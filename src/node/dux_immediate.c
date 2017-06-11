#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_IMMEDIATE)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_IMMEDIATE[] = DUX_IPK("Immediate");
DUK_LOCAL const char DUX_IPK_IMMEDIATE_CB[] = DUX_IPK("imCb");

/**
 * Entry of setImmediate()
 */
DUK_LOCAL duk_ret_t immed_set_immediate(duk_context *ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);

    if (nargs == 0) {
        return DUK_ERR_TYPE_ERROR;
    }
    /* [ func (...) ] */
    duk_require_callable(ctx, 0);
    if (nargs > 1) {
        /* [ func ... ] */
        dux_bind_arguments(ctx, nargs - 1);
        /* [ bound_func ] */
    }

    duk_push_object(ctx);
    duk_swap(ctx, 0, 1);
    /* [ obj func ] */
    duk_put_prop_string(ctx, 0, DUX_IPK_IMMEDIATE_CB);
    /* [ immed ] */
    duk_push_heap_stash(ctx);
    /* [ immed stash ] */
    if (!duk_get_prop_string(ctx, 1, DUX_IPK_IMMEDIATE)) {
        /* [ immed stash undefined ] */
        duk_pop(ctx);
        duk_push_array(ctx);
        /* [ immed stash arr ] */
        duk_dup(ctx, 2);
        duk_put_prop_string(ctx, 1, DUX_IPK_IMMEDIATE);
    }
    /* [ immed stash arr ] */
    duk_dup(ctx, 0);
    duk_put_prop_index(ctx, 2, duk_get_length(ctx, 2));
    /* [ immed stash arr ] */
    duk_pop_2(ctx);
    /* [ immed ] */
    return 1;
}

/**
 * Entry of clearImmediate()
 */
DUK_LOCAL duk_ret_t immed_clear_immediate(duk_context *ctx)
{
    /* [ immed ] */
    if (!duk_is_object(ctx, 0)) {
        return DUK_RET_TYPE_ERROR;
    }
    if (!duk_has_prop_string(ctx, 0, DUX_IPK_IMMEDIATE_CB)) {
        /* Invalid Immediate object */
        return DUK_RET_RANGE_ERROR;
    }
    duk_del_prop_string(ctx, 0, DUX_IPK_IMMEDIATE_CB);
    return 0;   /* return undefined */
}

/*
 * Initialize "immediate" timer
 */
DUK_INTERNAL duk_errcode_t dux_immediate_init(duk_context *ctx)
{
    duk_push_c_function(ctx, immed_set_immediate, DUK_VARARGS);
    duk_put_global_string(ctx, "setImmediate");
    duk_push_c_function(ctx, immed_clear_immediate, 1);
    duk_put_global_string(ctx, "clearImmediate");
    return DUK_ERR_NONE;
}

DUK_INTERNAL duk_int_t dux_immediate_tick(duk_context *ctx)
{
	duk_int_t result = DUX_TICK_RET_JOBLESS;

    /* [ ... ] */
    duk_push_heap_stash(ctx);
    /* [ ... stash ] */
    if (duk_get_prop_string(ctx, -1, DUX_IPK_IMMEDIATE)) {
        /* [ ... stash arr ] */
        duk_del_prop_string(ctx, -2, DUX_IPK_IMMEDIATE);
        duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
        /* [ ... stash arr enum ] */
        while (duk_next(ctx, -1, 1)) {
            /* [ ... stash arr enum key value ] */
            if (duk_get_prop_string(ctx, -1, DUX_IPK_IMMEDIATE_CB)) {
                /* [ ... stash arr enum key value func ] */
                duk_del_prop_string(ctx, -2, DUX_IPK_IMMEDIATE_CB);
                result = DUX_TICK_RET_CONTINUE;
                if (duk_pcall(ctx, 0) != 0) {
                    /* [ ... stash arr enum key value error ] */
                    dux_report_error(ctx);
                }
            }
            duk_pop_3(ctx);
            /* [ ... stash arr enum ] */
        }
        duk_pop(ctx);
        /* [ ... stash arr ] */
    }
    duk_pop_2(ctx);
    /* [ ... ] */
    return result;
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_IMMEDIATE */
