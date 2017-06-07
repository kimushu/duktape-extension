#if !defined(DUX_OPT_NO_HARDWARE_MODULES)
#include "../dux_internal.h"

/*
 * Entry of Hardware module
 */
DUK_LOCAL duk_errcode_t hardware_entry(duk_context *ctx)
{
    /* [ require module exports ] */
    return dux_invoke_initializers(ctx,
        DUX_INIT_PARAIO
        DUX_INIT_I2CCON
        DUX_INIT_SPICON
        NULL
    );
}

/*
 * Initialize Hardware module
 */
DUK_INTERNAL duk_errcode_t dux_hardware_init(duk_context *ctx)
{
    dux_modules_register(ctx, "hardware", hardware_entry);
}

/*
 * Tick for Hardware modules
 */
DUK_INTERNAL duk_int_t dux_hardware_tick(duk_context *ctx)
{
    return dux_invoke_tick_handlers(ctx,
        DUX_TICK_PARAIO
        DUX_TICK_I2CCON
        DUX_TICK_SPICON
        NULL
    );
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES */
