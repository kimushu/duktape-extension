/**
 * @file delay.c
 * @brief Promisified delay function (compatible with npm/delay 1.3.1)
 */

#if defined(DUX_ENABLE_PACKAGE_DELAY) && !defined(DUX_OPT_NO_PROMISE)

#include "../dux_internal.h"

DUK_LOCAL duk_ret_t delay_generate(duk_context *ctx)
{
    /* [ delay value ] */
    dux_promise_new(ctx);
    /* [ delay value promise resolve reject ] */
    if (duk_get_current_magic(ctx)) {
        // resolve
        duk_insert(ctx, 0);
        /* [ reject delay value promise resolve ] */
        duk_insert(ctx, 1);
        /* [ reject resolve delay value promise ] */
    } else {
        // reject
        duk_dup_top(ctx);
        duk_insert(ctx, 0);
        duk_insert(ctx, 1);
        /* [ reject reject delay value promise resolve ] */
        duk_pop(ctx);
        /* [ reject reject delay value promise ] */
    }
    duk_insert(ctx, 0);
    /* [ promise reject callback delay value ] */
    duk_get_global_string(ctx, "setTimeout");
    /* [ promise reject callback delay value func ] */
    duk_insert(ctx, 2);
    /* [ promise reject func callback delay value ] */
    if (duk_pcall(ctx, 3) != DUK_EXEC_SUCCESS) {
        /* [ promise reject err ] */
        duk_pcall(ctx, 1);
        /* [ promise retval ] */
        duk_pop(ctx);
        /* [ promise ] */
    } else {
        /* [ promise reject retval(timer) ] */
        duk_pop_2(ctx);
        /* [ promise ] */
    }
    return 1;   /* return promise */
}

DUK_LOCAL duk_ret_t package_delay_entry(duk_context *ctx)
{
    /* [ require module exports ] */
    duk_push_c_function(ctx, delay_generate, 2);
    duk_set_magic(ctx, -1, 1);
    /* [ require module exports func(resolve) ] */
    duk_push_c_function(ctx, delay_generate, 2);
    duk_set_magic(ctx, -1, 0);
    /* [ require module exports func(resolve) func(reject) ] */
    duk_put_prop_string(ctx, 3, "reject");
    /* [ require module exports func(resolve) ] */
    duk_put_prop_string(ctx, 1, "exports");
    /* [ require module exports ] */
    return DUK_ERR_NONE;
}

/*
 * Initialize delay package
 */
DUK_INTERNAL duk_errcode_t dux_package_delay(duk_context *ctx)
{
    return dux_modules_register(ctx, "delay", package_delay_entry);
}

#endif  /* DUX_ENABLE_PACKAGE_DELAY && !DUX_OPT_NO_PROMISE */
