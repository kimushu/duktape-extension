#ifndef DUX_SPICON_H_INCLUDED
#define DUX_SPICON_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)

/*
 * Structures
 */

typedef struct
{
	duk_ret_t (*transferRaw)(duk_context *ctx, void *data);
	duk_ret_t (*bitrate_getter)(duk_context *ctx, void *data);
	duk_ret_t (*bitrate_setter)(duk_context *ctx, void *data);
	duk_ret_t (*lsbFirst_getter)(duk_context *ctx, void *data);
	duk_ret_t (*lsbFirst_setter)(duk_context *ctx, void *data);
	duk_ret_t (*mode_getter)(duk_context *ctx, void *data);
	duk_ret_t (*mode_setter)(duk_context *ctx, void *data);
	duk_ret_t (*slaveSelect_getter)(duk_context *ctx, void *data);
}
dux_spicon_functions;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_spicon_init(duk_context *ctx);
#define DUX_INIT_SPICON     dux_spicon_init,
#define DUX_TICK_SPICON

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */

#define DUX_INIT_SPICON
#define DUX_TICK_SPICON

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_SPI */
#endif  /* !DUX_SPICON_H_INCLUDED */
