#ifndef DUX_I2CCON_H_
#define DUX_I2CCON_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dux_i2ccon_t
{
	duk_ret_t (*transfer)(duk_context *ctx, struct dux_i2ccon_t *data, duk_uint_t readlen);
	duk_ret_t (*update_bitrate)(duk_context *ctx, struct dux_i2ccon_t *data);
	duk_uint_t slaveAddress;
	duk_uint_t bitrate;
}
dux_i2ccon_t;

extern void dux_i2ccon_init(duk_context *ctx);
extern void dux_push_i2ccon_class(duk_context *ctx);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_I2CCON_H_ */
