#ifndef DUX_I2CCON_H_INCLUDED
#define DUX_I2CCON_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)

/*
 * Structures
 */

typedef struct
{
	duk_ret_t (*transfer)(duk_context *ctx, void *data);
	duk_ret_t (*slaveAddress_getter)(duk_context *ctx, void *data);
	duk_ret_t (*bitrate_getter)(duk_context *ctx, void *data);
	duk_ret_t (*bitrate_setter)(duk_context *ctx, void *data);
}
dux_i2ccon_functions;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_i2ccon_init(duk_context *ctx);
#define DUX_INIT_I2CCON     dux_i2ccon_init,
#define DUX_TICK_I2CCON

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */

#define DUX_INIT_I2CCON
#define DUX_TICK_I2CCON

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_I2C */
#endif  /* !DUX_I2CCON_H_INCLUDED */
