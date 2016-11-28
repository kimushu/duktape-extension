#ifndef DUX_I2CCON_H_INCLUDED
#define DUX_I2CCON_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)

/*
 * Structures
 */

typedef struct dux_i2ccon_data
{
	duk_ret_t (*transfer)(duk_context *ctx, struct dux_i2ccon_data *data, duk_uint_t readlen);
	duk_ret_t (*update_bitrate)(duk_context *ctx, struct dux_i2ccon_data *data);
	duk_uint_t slaveAddress;
	duk_uint_t bitrate;
}
dux_i2ccon_data;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_i2ccon_init(duk_context *ctx);
#define dux_i2ccon_tick(ctx)    (DUX_TICK_RET_JOBLESS)

DUK_INTERNAL_DECL void dux_push_i2ccon_class(duk_context *ctx);

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */

#define dux_i2ccon_init(ctx)    (DUK_ERR_NONE)
#define dux_i2ccon_tick(ctx)    (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_I2C */
#endif  /* !DUX_I2CCON_H_INCLUDED */
