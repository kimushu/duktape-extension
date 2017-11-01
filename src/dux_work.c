#include "dux_internal.h"
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <errno.h>

typedef union
{
    duk_uint64_t _packer;
    struct
    {
        duk_uint8_t queued;
        duk_uint8_t abort;
        duk_uint8_t done;
        duk_uint8_t after_nargs;
        dux_work_finalizer finalizer;
        duk_int_t result;
        pthread_t thread;
        dux_work_cb work_cb;
        dux_after_work_cb after_work_cb;
    };
}
dux_work_priv_t;

DUK_LOCAL const char DUX_IPK_WORK[] = DUX_IPK("Work");
DUK_LOCAL int DUX_IDX_WORK_THREAD  = 0;
DUK_LOCAL int DUX_IDX_WORK_REQUEST = 1;

/**
 * @func work_worker
 * @brief Worker thread entry (detached from Duktape contexts!)
 */
DUK_LOCAL void *work_worker(dux_work_priv_t *req_priv)
{
    req_priv->result = (*req_priv->work_cb)((dux_work_t *)(req_priv + 1));
    req_priv->done = 1;
    return NULL;
}

/**
 * @func work_free
 * @brief Free memory for work request
 */
DUK_LOCAL void work_free(duk_context *ctx, dux_work_priv_t *req_priv)
{
    if (req_priv->finalizer)
    {
        (*req_priv->finalizer)(ctx, (dux_work_t *)(req_priv + 1));
    }
    duk_free(ctx, req_priv);
}

/**
 * @func work_finalizer
 * @brief Work queue finalizer
 */
DUK_LOCAL duk_ret_t work_finalizer(duk_context *ctx)
{
    /* [ obj ] */
    dux_work_priv_t *req_priv;

    // Set abort flag of all workers
    duk_enum(ctx, 0, DUK_ENUM_OWN_PROPERTIES_ONLY);
    /* [ obj enum ] */
    while (duk_next(ctx, 1, 1))
    {
        /* [ obj enum key arr:3 ] */
        duk_get_prop_index(ctx, 3, DUX_IDX_WORK_REQUEST);
        /* [ obj enum key arr:3 ptr:4 ] */
        req_priv = (dux_work_priv_t *)duk_get_pointer(ctx, 4);
        if (req_priv)
        {
            req_priv->abort = 1;
        }
        duk_pop_3(ctx);
        /* [ obj enum ] */
    }
    /* [ obj enum ] */
    duk_pop(ctx);
    /* [ obj ] */

    // Join threads
    duk_enum(ctx, 0, DUK_ENUM_OWN_PROPERTIES_ONLY);
    /* [ obj enum ] */
    while (duk_next(ctx, 1, 1))
    {
        /* [ obj enum key arr:3 ] */
        duk_get_prop_index(ctx, 3, DUX_IDX_WORK_REQUEST);
        /* [ obj enum key arr:3 ptr:4 ] */
        req_priv = (dux_work_priv_t *)duk_get_pointer(ctx, 4);
        duk_del_prop_index(ctx, 3, DUX_IDX_WORK_THREAD);
        duk_del_prop_index(ctx, 3, DUX_IDX_WORK_REQUEST);
        if (req_priv && req_priv->work_cb)
        {
            pthread_join(req_priv->thread, NULL);
            work_free(ctx, req_priv);
        }
        duk_pop_3(ctx);
        /* [ obj enum ] */
    }
    /* [ obj enum ] */

    return 0;
}

/**
 * @func dux_work_aborting
 * @brief Determine if work is requested to abort
 */
DUK_INTERNAL duk_bool_t dux_work_aborting(dux_work_t *req)
{
    if (req)
    {
        dux_work_priv_t *req_priv = ((dux_work_priv_t *)req) - 1;
        return req_priv->abort;
    }
    return 1;
}

/**
 * @func queue_work_safe
 * @brief Body of work queueing
 */
DUK_LOCAL duk_ret_t queue_work_safe(duk_context *ctx, dux_work_priv_t *req_priv)
{
    /* [ ... arg1 ... argN ] */
    duk_context *after_ctx;

    duk_push_heap_stash(ctx);
    /* [ ... arg1 ... argN stash ] */
    duk_get_prop_string(ctx, -1, DUX_IPK_WORK);
    /* [ ... arg1 ... argN stash obj ] */
    duk_push_pointer(ctx, req_priv);
    /* [ ... arg1 ... argN stash obj ptr ] */
    duk_push_array(ctx);
    /* [ ... arg1 ... argN stash obj ptr arr ] */
    duk_push_thread(ctx);
    after_ctx = duk_require_context(ctx, -1);
    duk_put_prop_index(ctx, -2, DUX_IDX_WORK_THREAD);
    /* [ ... arg1 ... argN stash obj ptr arr ] */
    duk_push_pointer(ctx, req_priv);
    duk_put_prop_index(ctx, -2, DUX_IDX_WORK_REQUEST);
    /* [ ... arg1 ... argN stash obj ptr arr ] */
    duk_put_prop(ctx, -3);
    req_priv->queued = 1;
    /* [ ... arg1 ... argN stash obj ] */
    duk_pop_2(ctx);
    /* [ ... arg1 ... argN ] */
    duk_push_undefined(after_ctx);
    duk_xmove_top(after_ctx, ctx, req_priv->after_nargs);
    /* [ ... ] (ctx) */
    /* [ undefined arg1 ... argN ] (after_ctx) */
    if (pthread_create(&req_priv->thread, NULL, (void *(*)(void *))work_worker, req_priv) != 0)
    {
        req_priv->work_cb = NULL;
        return duk_generic_error(ctx, "Cannot create worker thread (errno=%d)", errno);
    }
    return 0;
}

/**
 * @func dux_queue_work
 * @brief Queue a new work request
 */
DUK_INTERNAL void dux_queue_work(duk_context *ctx, const dux_work_t *req, duk_size_t req_size, dux_work_cb work_cb, dux_after_work_cb after_work_cb, duk_idx_t after_nargs, dux_work_finalizer finalizer)
{
    /* [ ... arg1 ... argN ] */
    dux_work_priv_t *req_priv;

    req_priv = (dux_work_priv_t *)duk_alloc(ctx, sizeof(*req_priv) + req_size);
    if (!req_priv)
    {
        (void)duk_generic_error(ctx, "Cannot allocate memory for work queue");
        return;
    }
    memset(req_priv, 0, sizeof(*req_priv));
    req_priv->finalizer = finalizer;
    req_priv->work_cb = work_cb;
    req_priv->after_work_cb = after_work_cb;
    req_priv->after_nargs = after_nargs;
    memcpy(req_priv + 1, req, req_size);
    if (duk_safe_call(ctx, (duk_safe_call_function)queue_work_safe, req_priv, after_nargs, 1) != DUK_EXEC_SUCCESS)
    {
        // Queueing failed
        /* [ ... err ] */
        if (!req_priv->queued) {
            duk_free(ctx, req_priv);
            (void)duk_throw(ctx);
        }
        return;
    }

    // Succeeded
    /* [ ... undefined ] */
    duk_pop(ctx);
    /* [ ... ] */
}

/**
 * @func dux_work_init
 * @brief Initialize work queue
 */
DUK_INTERNAL duk_errcode_t dux_work_init(duk_context *ctx)
{
    /* [ ... ] */
    duk_push_heap_stash(ctx);
    /* [ ... stash ] */
    duk_push_object(ctx);
    /* [ ... stash obj ] */
    duk_push_c_function(ctx, work_finalizer, 1);
    duk_set_finalizer(ctx, -2);
    /* [ ... stash obj ] */
    duk_put_prop_string(ctx, -2, DUX_IPK_WORK);
    /* [ ... stash ] */
    duk_pop(ctx);
    /* [ ... ] */
    return DUK_ERR_NONE;
}

/**
 * @func dux_work_tick
 * @brief Tick handler for work queue
 */
DUK_INTERNAL duk_int_t dux_work_tick(duk_context *ctx)
{
    /* [ ... ] */
    duk_int_t result;

    duk_push_heap_stash(ctx);
    /* [ ... stash ] */
    if (!duk_get_prop_string(ctx, -1, DUX_IPK_WORK))
    {
        /* [ ... stash undefined ] */
        duk_pop_2(ctx);
        /* [ ... ] */
        return DUX_TICK_RET_JOBLESS;
    }
    /* [ ... stash obj ] */

    result = DUX_TICK_RET_JOBLESS;

    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    /* [ ... stash obj enum ] */
    while (duk_next(ctx, -1, 1))
    {
        /* [ ... stash obj enum key arr ] */
        dux_work_priv_t *req_priv;
        duk_context *after_ctx;

        duk_get_prop_index(ctx, -1, DUX_IDX_WORK_REQUEST);
        /* [ ... stash obj enum key arr ptr ] */
        req_priv = (dux_work_priv_t *)duk_get_pointer(ctx, -1);
        if ((!req_priv) || (!req_priv->work_cb))
        {
            duk_pop_2(ctx);
            /* [ ... stash obj enum key ] */
            goto cleanup;
        }

        result = DUX_TICK_RET_CONTINUE;
        if (!req_priv->done)
        {
            // Still running
            duk_pop_3(ctx);
            /* [ ... stash obj enum ] */
            continue;
        }

        // Finished
        pthread_join(req_priv->thread, NULL);
        duk_get_prop_index(ctx, -2, DUX_IDX_WORK_THREAD);
        /* [ ... stash obj enum key arr ptr thr ] */
        after_ctx = duk_get_context(ctx, -1);
        if (after_ctx && req_priv->after_work_cb)
        {
            duk_push_int(after_ctx, req_priv->result);
            duk_replace(after_ctx, 0);
            /* after_ctx: [ int arg1 ... argN ] */
            if (duk_safe_call(after_ctx,
                    (duk_safe_call_function)req_priv->after_work_cb,
                    (dux_work_t *)(req_priv + 1),
                    req_priv->after_nargs + 1, 1) != DUK_EXEC_SUCCESS)
            {
                /* after_ctx: [ ... err ] */
                dux_report_error(after_ctx);
            }
            duk_set_top(after_ctx, 0);
            /* after_ctx: [  ] */
        }
        duk_pop_3(ctx);
        /* [ ... stash obj enum key ] */

cleanup:
        duk_del_prop(ctx, -3);
        /* [ ... stash obj enum ] */
        work_free(ctx, req_priv);
    }
    /* [ ... stash obj enum ] */

    // Give other threads the time to process queued work
    sched_yield();

    duk_pop_3(ctx);
    /* [ ... ] */
    return result;
}
