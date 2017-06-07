#ifndef DUX_PARAIO_H_INCLUDED
#define DUX_PARAIO_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_PARALLELIO)

/*
 * Type definitions
 */

typedef struct dux_paraio_manip
{
	duk_ret_t (*read_input)(duk_context *ctx, void *param, duk_uint_t bits, duk_uint_t *result);
	duk_ret_t (*write_output)(duk_context *ctx, void *param, duk_uint_t set, duk_uint_t clear, duk_uint_t toggle);
	duk_ret_t (*config_input)(duk_context *ctx, void *param, duk_uint_t bits, duk_uint_t enabled);
	duk_ret_t (*config_output)(duk_context *ctx, void *param, duk_uint_t bits, duk_uint_t enabled);
	duk_ret_t (*read_config)(duk_context *ctx, void *param, duk_uint_t bits, duk_uint_t *input, duk_uint_t *output);
	duk_ret_t (*slice)(duk_context *ctx, void *param, duk_uint_t bits, struct dux_paraio_manip const **new_manip, void **new_param);
}
dux_paraio_manip;

DUK_INTERNAL_DECL const dux_paraio_manip dux_paraio_manip_ro;
DUK_INTERNAL_DECL const dux_paraio_manip dux_paraio_manip_rw;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_paraio_init(duk_context *ctx);
#define DUX_INIT_PARAIO     dux_paraio_init,
#define DUX_TICK_PARAIO

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_PARALLELIO */

#define DUX_INIT_PARAIO
#define DUX_TICK_PARAIO

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_PARALLELIO */
#endif  /* !DUX_PARAIO_H_INCLUDED */
