/*
 * ECMA objects:
 *    class PeridotSPI {
 *      constructor({sclk: <uint> pin [,mosi: <uint> pin] [,miso: <uint> pin]}) {
 *      }
 *
 *      static connect({ss_m: <uint> pin, sclk: <uint> pin
 *                      [,mosi: <uint> pin] [,miso: <uint> pin]},
 *                     <uint> bitrate = SPI_DEFAULT_BITRATE) {
 *        return <SPIConnection>;
 *      }
 *
 *      connect(<uint> ss_n_pin,
 *              <uint> bitrate = SPI_DEFAULT_BITRATE) {
 *        return <SPIConnection>;
 *      }
 *    }
 *    global.Peridot.SPI = PeridotSPI;
 *
 * Internal data structure:
 *    PeridotSPI.[[DUX_IPK_PERIDOT_SPI_POOLS]] = new Array(
 *      thrpool1, ..., thrpoolN
 *    );
 *    thrpoolX.[[DUX_IPK_PERIDOT_SPI_DRIVER]] = <pointer> driver;
 *    (new PeridotSPI).[[DUX_IPK_PERIDOT_SPI_DRIVER]] = <pointer> driver;
 *    (new PeridotSPI).[[DUX_IPK_PERIDOT_SPI_PINS]] = <uint> pins;
 */
#if defined(DUX_USE_BOARD_PERIDOT)
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)
#include "../dux_internal.h"
#include <system.h>
#include <peridot_spi_master.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>

/*
 * Constants
 */

#define SPI_DEFAULT_BITRATE	12500000

DUK_LOCAL const char DUX_IPK_PERIDOT_SPI_POOLS[] = DUX_IPK("bSPIPools");
DUK_LOCAL const char DUX_IPK_PERIDOT_SPI_PINS[] = DUX_IPK("bSPIPins");
DUK_LOCAL const char DUX_IPK_PERIDOT_SPI_DRIVER[] = DUX_IPK("bSPIDrv");

enum
{
	SPI_BLKIDX_DATA = 0,
	SPI_BLKIDX_CLKDIV,
	SPI_BLKIDX_WRITEDATA,
	SPI_BLKIDX_READBUF,
	SPI_BLKIDX_READSKIP,
	SPI_BLKIDX_FLAGS,
	SPI_BLKIDX_CALLBACK,
	SPI_BLKIDX_THIS,
	SPI_NUM_BLOCKS,
};

/*
 * Structures
 */

typedef union spi_pins_t
{
	duk_uint_t uint;
	struct
	{
		duk_int8_t ss_n;
		duk_int8_t sclk;
		duk_int8_t mosi;
		duk_int8_t miso;
	};
}
spi_pins_t;

typedef struct dux_spicon_data_peridot
{
	dux_spicon_data common;
	spi_pins_t pins;
	peridot_spi_master_state *driver;
	alt_u32 clkdiv;
}
dux_spicon_data_peridot;

/*
 * Read pin configurations from ECMA object to spi_pins_t
 */
DUK_LOCAL duk_ret_t spi_get_pins(duk_context *ctx, duk_idx_t index, spi_pins_t *pins)
{
	pins->uint = 0;
	pins->ss_n = dux_get_peridot_pin_by_key(ctx, index, "ss_n", "cs_n", NULL);
	if (pins->ss_n < DUK_RET_ERROR)
	{
		return pins->ss_n;
	}
	pins->sclk = dux_get_peridot_pin_by_key(ctx, index, "sclk", "sck", NULL);
	if (pins->sclk < 0)
	{
		return pins->sclk;
	}
	pins->mosi = dux_get_peridot_pin_by_key(ctx, index, "mosi", NULL);
	if (pins->mosi < DUK_RET_ERROR)
	{
		return pins->mosi;
	}
	pins->miso = dux_get_peridot_pin_by_key(ctx, index, "miso", NULL);
	if (pins->miso < DUK_RET_ERROR)
	{
		return pins->miso;
	}
	return 0;
}

/*
 * Thread pool worker for SPI transfer
 */
DUK_LOCAL duk_int_t spicon_worker(const dux_thrpool_block *blocks, duk_size_t num_blocks)
{
	dux_spicon_data_peridot *data;
	int result;

	if (num_blocks != SPI_NUM_BLOCKS)
	{
		return -EBADF;
	}

	data = (dux_spicon_data_peridot *)blocks[SPI_BLKIDX_DATA].pointer;

	result = peridot_spi_master_configure_pins(
			data->driver,
			data->pins.sclk,
			data->pins.mosi,
			data->pins.miso,
			0);
	if (result < 0)
	{
		return result;
	}

	result = peridot_spi_master_transfer(
			data->driver,
			data->pins.ss_n,
			blocks[SPI_BLKIDX_CLKDIV].uint,
			blocks[SPI_BLKIDX_WRITEDATA].length,
			blocks[SPI_BLKIDX_WRITEDATA].pointer,
			blocks[SPI_BLKIDX_READSKIP].uint,
			blocks[SPI_BLKIDX_READBUF].length,
			blocks[SPI_BLKIDX_READBUF].pointer,
			blocks[SPI_BLKIDX_FLAGS].uint);

	return result;
}

/*
 * Thread pool completer for SPI transfer
 */
DUK_LOCAL duk_ret_t spicon_completer(duk_context *ctx)
{
	duk_idx_t nargs;

	/* [ job int ] */
	duk_int_t ret = duk_require_int(ctx, 1);
	duk_get_prop_index(ctx, 0, SPI_BLKIDX_CALLBACK);
	/* [ job int func ] */
	duk_require_callable(ctx, 2);

	if (ret == 0)
	{
		/* Transfer succeeded */
		duk_push_true(ctx);
		/* [ job int func true ] */
		duk_get_prop_index(ctx, 0, SPI_BLKIDX_READBUF);
		/* [ job int func true buf:4 ] */
		if (duk_is_null_or_undefined(ctx, 4))
		{
			duk_pop(ctx);
			/* [ job int func true ] */
			nargs = 1;
		}
		else
		{
			duk_push_buffer_object(ctx, 4,
					0, duk_get_length(ctx, 4),
					DUK_BUFOBJ_ARRAYBUFFER);
			/* [ job int func true buf:4 bufobj(ArrayBuffer):5 ] */
			duk_remove(ctx, 4);
			/* [ job int func true bufobj(ArrayBuffer):4 ] */
			nargs = 2;
		}
	}
	else
	{
		/* Transfer failed */
		duk_push_false(ctx);
		/* [ job int func false ] */
		duk_push_error_object(ctx, DUK_ERR_API_ERROR, "SPI transfer failed (%d)", ret);
		/* [ job int func false err ] */
		nargs = 2;
	}

	duk_call(ctx, nargs);
	/* [ job int retval ] */

	return 0;
}

/*
 * Implementation of SPIConnection.prototype.{read,transfer,write}
 */
DUK_LOCAL void spicon_transfer(duk_context *ctx, dux_spicon_data_peridot *data,
		duk_size_t read_skip, duk_uint_t filler)
{
	duk_idx_t job_idx;

	/* [ ... func:-5 writedata:-4 readbuf:-3 this:-2 thrpool:-1 ] */
	duk_swap_top(ctx, -5);
	/* [ ... thrpool:-5 writedata:-4 readbuf:-3 this:-2 func:-1 ] */
	duk_push_array(ctx);
	/* [ ... thrpool:-6 writedata:-5 readbuf:-4 this:-3 func:-2 arr:-1 ] */
	job_idx = duk_normalize_index(ctx, -5);
	duk_swap_top(ctx, job_idx);
	/* [ ... thrpool:-6 arr:-5(job_idx) readbuf:-4 this:-3 func:-2 writedata:-1 ] */
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_WRITEDATA);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_CALLBACK);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_THIS);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_READBUF);
	/* [ ... thrpool:-2 arr:-1(job_idx) ] */
	duk_push_pointer(ctx, data);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_DATA);
	duk_push_uint(ctx, data->clkdiv);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_CLKDIV);
	duk_push_uint(ctx, read_skip);
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_READSKIP);
	duk_push_uint(ctx,
			((filler << PERIDOT_SPI_MASTER_FILLER_OFST) &
				PERIDOT_SPI_MASTER_FILLER_MSK) |
			((data->common.mode << PERIDOT_SPI_MASTER_MODE_OFST) &
				PERIDOT_SPI_MASTER_MODE_MSK) |
			(data->common.lsbFirst ? PERIDOT_SPI_MASTER_LSBFIRST : 0));
	duk_put_prop_index(ctx, job_idx, SPI_BLKIDX_FLAGS);
	/* [ ... thrpool:-2 arr:-1 ] */
	dux_thrpool_queue(ctx, -2, spicon_worker, spicon_completer);
	/* [ ... thrpool:-1 ] */
	duk_pop(ctx);
	/* [ ... ] */
}

/*
 * Implementation for bitrate change
 */
DUK_LOCAL duk_ret_t spicon_update_bitrate(duk_context *ctx, dux_spicon_data_peridot *data)
{
	duk_ret_t result;
	alt_u32 clkdiv;

	result = peridot_spi_master_get_clkdiv(data->driver,
				data->common.bitrate, &clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->clkdiv = clkdiv;
	return 0;
}

/*
 * Common implementation of PeridotSPI.(prototype.)connect
 */
DUK_LOCAL duk_ret_t spi_connect_body(duk_context *ctx, spi_pins_t *pins)
{
	dux_spicon_data_peridot *data;
	duk_uint_t bitrate;
	peridot_spi_master_state *driver;

	/* [ any uint(bitrate)/undefined this ] */
	if (duk_is_null_or_undefined(ctx, 1))
	{
		bitrate = SPI_DEFAULT_BITRATE;
	}
	else
	{
		bitrate = duk_require_uint(ctx, 1);
	}

	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_POOLS);
	/* [ any uint/undefined this arr ] */
	duk_swap(ctx, 0, 3);
	/* [ arr uint/undefined this any ] */
	duk_set_top(ctx, 1);
	/* [ arr ] */

	dux_push_spicon_constructor(ctx);
	/* [ arr constructor ] */
	duk_push_fixed_buffer(ctx, sizeof(*data));
	/* [ arr constructor buf ] */
	data = (dux_spicon_data_peridot *)duk_get_buffer(ctx, -1, NULL);

	data->common.transfer =
		(void (*)(duk_context *, dux_spicon_data *, duk_size_t, duk_uint_t))spicon_transfer;
	data->common.update_bitrate =
		(duk_ret_t (*)(duk_context *, dux_spicon_data *))spicon_update_bitrate;
	data->common.bitrate = bitrate;
	data->pins.uint = pins->uint;
	data->driver = NULL;

	duk_enum(ctx, 0, DUK_ENUM_ARRAY_INDICES_ONLY);
	/* [ arr constructor buf enum ] */
	while (duk_next(ctx, 3, 1))
	{
		/* [ arr constructor buf enum key:4 thrpool:5 ] */
		duk_get_prop_string(ctx, 5, DUX_IPK_PERIDOT_SPI_DRIVER);
		/* [ arr constructor buf enum key:4 thrpool:5 pointer:6 ] */
		driver = (peridot_spi_master_state *)duk_get_pointer(ctx, 6);
		duk_pop(ctx);
		/* [ arr constructor buf enum key:4 thrpool:5 ] */

		if ((driver) &&
			peridot_spi_master_configure_pins(driver,
					pins->sclk, pins->mosi, pins->miso, 1) == 0)
		{
			data->driver = driver;
			duk_swap(ctx, 3, 5);
			/* [ arr constructor buf thrpool key:4 enum:5 ] */
			duk_set_top(ctx, 4);
			/* [ arr constructor buf thrpool ] */
			duk_new(ctx, 2);
			/* [ arr obj ] */
			return 1; /* return obj */
		}

		duk_pop_2(ctx);
		/* [ arr constructor buf enum ] */
	}

	duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR,
			"no driver supports (sclk=%u,mosi=%d,miso=%d)",
			pins->sclk, pins->mosi, pins->miso);
	/* unreachable */
	return 0;
}

/*
 * Entry of PeridotSPI constructor
 */
DUK_LOCAL duk_ret_t spi_constructor(duk_context *ctx)
{
	spi_pins_t pins;
	duk_ret_t result;

	/* [ obj ] */
	result = spi_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	pins.ss_n = -1;

	duk_push_current_function(ctx);
	duk_push_this(ctx);
	/* [ obj constructor this ] */

	duk_push_uint(ctx, pins.uint);
	duk_put_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_PINS);

	duk_get_prop_string(ctx, 1, DUX_IPK_PERIDOT_SPI_POOLS);
	duk_put_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_POOLS);

	return 0; /* return this */
}

/*
 * Entry of PeridotSPI.connect()
 */
DUK_LOCAL duk_ret_t spi_connect(duk_context *ctx)
{
	spi_pins_t pins;
	duk_ret_t result;

	/* [ obj uint/undefined ] */
	result = spi_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	/* [ obj uint/undefined ] */
	duk_push_this(ctx);
	/* [ obj uint/undefined this ] */

	return spi_connect_body(ctx, &pins);
}

/*
 * Entry of PeridotSPI.prototype.connect()
 */
DUK_LOCAL duk_ret_t spi_proto_connect(duk_context *ctx)
{
	spi_pins_t pins;

	/* [ obj uint/undefined ] */
	duk_push_this(ctx);
	/* [ obj uint/undefined this ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_PINS);
	/* [ uint uint/undefined this uint ] */
	pins.uint = duk_require_uint(ctx, 3);
	duk_pop(ctx);
	/* [ uint uint/undefined this ] */
	pins.ss_n = dux_get_peridot_pin(ctx, 0);

	return spi_connect_body(ctx, &pins);
}

/*
 * Getter of PeridotSPI.prototype.pins
 */
DUK_LOCAL duk_ret_t spi_proto_pins_getter(duk_context *ctx)
{
	spi_pins_t pins;

	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	duk_push_object(ctx);
	/* [ this obj ] */
	duk_get_prop_string(ctx, 0, DUX_IPK_PERIDOT_SPI_PINS);
	/* [ this obj uint ] */
	pins.uint = duk_require_uint(ctx, 2);
	if (pins.ss_n >= 0)
	{
		duk_push_uint(ctx, pins.ss_n);
		duk_put_prop_string(ctx, 1, "ss_n");
	}
	duk_push_uint(ctx, pins.sclk);
	duk_put_prop_string(ctx, 1, "sclk");
	if (pins.mosi >= 0)
	{
		duk_push_uint(ctx, pins.mosi);
		duk_put_prop_string(ctx, 1, "mosi");
	}
	if (pins.miso >= 0)
	{
		duk_push_uint(ctx, pins.miso);
		duk_put_prop_string(ctx, 1, "miso");
	}
	return 1; /* return obj */
}

/*
 * List of class methods
 */
DUK_LOCAL duk_function_list_entry spi_funcs[] = {
	{ "connect", spi_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype methods
 */
DUK_LOCAL duk_function_list_entry spi_proto_funcs[] = {
	{ "connect", spi_proto_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL dux_property_list_entry spi_proto_props[] = {
	{ "pins", spi_proto_pins_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize PeridotSPI submodule
 */
DUK_INTERNAL duk_errcode_t dux_peridot_spi_init(duk_context *ctx)
{
	extern peridot_spi_master_state *const peridot_spi_drivers[];
	duk_idx_t index;

	/* [ ... obj ] */
	dux_push_named_c_constructor(
			ctx, "PeridotSPI", spi_constructor, 1,
			spi_funcs, spi_proto_funcs, NULL, spi_proto_props);
	/* [ ... obj constructor ] */
	duk_push_array(ctx);
	/* [ ... obj constructor arr ] */
	for (index = 0;; ++index)
	{
		peridot_spi_master_state *driver;
		driver = peridot_spi_drivers[index];
		if (!driver)
		{
			break;
		}
		dux_push_thrpool(ctx, 1, 1);
		duk_push_pointer(ctx, driver);
		/* [ ... obj constructor arr thrpool pointer ] */
		duk_put_prop_string(ctx, -2, DUX_IPK_PERIDOT_SPI_DRIVER);
		/* [ ... obj constructor arr thrpool ] */
		duk_put_prop_index(ctx, -2, index);
		/* [ ... obj constructor arr ] */
	}

	duk_put_prop_string(ctx, -2, DUX_IPK_PERIDOT_SPI_POOLS);
	/* [ ... obj constructor ] */

	if (index == 0)
	{
		// No driver
		duk_pop(ctx);
		/* [ ... obj ] */
		return DUK_ERR_UNSUPPORTED_ERROR;
	}

	duk_put_prop_string(ctx, -2, "SPI");
	/* [ ... obj ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */
#endif  /* DUX_USE_BOARD_PERIDOT */
