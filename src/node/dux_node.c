#if !defined(DUX_OPT_NO_NODEJS_MODULES)
#include "../dux_internal.h"

DUK_INTERNAL duk_errcode_t dux_node_init(duk_context *ctx)
{
    return dux_invoke_initializers(ctx,
        DUX_INIT_EVENTS
        DUX_INIT_CONSOLE
        DUX_INIT_PROCESS
        DUX_INIT_TIMER
        DUX_INIT_UTIL
        DUX_INIT_PATH
        NULL
    );
}

DUK_INTERNAL duk_int_t dux_node_tick(duk_context *ctx)
{
    return dux_invoke_tick_handlers(ctx,
        DUX_TICK_EVENTS
        DUX_TICK_CONSOLE
        DUX_TICK_PROCESS
        DUX_TICK_TIMER
        DUX_TICK_UTIL
        DUX_TICK_PATH
        NULL
    );
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES */
