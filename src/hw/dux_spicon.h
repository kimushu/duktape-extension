#ifndef DUX_SPICON_H_INCLUDED
#define DUX_SPICON_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)

/*
 * Structures
 */

typedef struct dux_spicon_data
{
	/* [ ... func obj(writedata) obj(readbuffer) this aux ] */
	void (*transfer)(duk_context *ctx, struct dux_spicon_data *data,
			duk_size_t read_skip, duk_uint_t filler);

	duk_ret_t (*update_bitrate)(duk_context *ctx, struct dux_spicon_data *data);
	duk_ret_t (*get_slaveSelect)(duk_context *ctx, struct dux_spicon_data *data);

	duk_uint_t bitrate;
	duk_uint8_t mode;
	duk_uint8_t lsbFirst;
}
dux_spicon_data;

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
