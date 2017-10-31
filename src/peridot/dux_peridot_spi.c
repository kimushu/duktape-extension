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

#define PERIDOT_SPI_DEFAULT_BITRATE 1000000

DUK_LOCAL const char DUX_IPK_PERIDOT_SPI_SPICON[] = DUX_IPK("bSPICon");
DUK_LOCAL const char DUX_IPK_PERIDOT_SPI_PINS[] = DUX_IPK("bSPIPins");

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

typedef struct
{
	peridot_spi_pins_t pins;
	peridot_spi_master_pfc_map *map;
	alt_u32 clkdiv;
	alt_u32 flags;
}
peridot_spicon_data_t;

typedef struct
{
	peridot_spicon_data_t data;
	duk_uint_t writeLength;
	const void *writeData;
	duk_uint_t readSkip;
	duk_uint_t readLength;
	void *readData;
	duk_uint_t flags;
}
peridot_spicon_req_t;

/*
 * Read pin configurations from ECMA object to spi_pins_t
 */
DUK_LOCAL duk_ret_t peridot_spi_get_pins(duk_context *ctx, duk_idx_t index, spi_pins_t *pins)
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

DUK_LOCAL void peridot_spicon_finalize(duk_context *ctx, peridot_spicon_req_t *req)
{
	duk_free(ctx, (void *)req->writeData);
	duk_free(ctx, req->readData);
}

/*
 * Worker for SPI transfer
 */
DUK_LOCAL duk_int_t peridot_spicon_work_cb(peridot_spicon_req_t *req)
{
	int result;

	result = peridot_spi_master_configure_pins(
			req->data.map,
			req->data.pins.sclk,
			req->data.pins.mosi,
			req->data.pins.miso,
			0);
	if (result < 0)
	{
		return result;
	}

	result = peridot_spi_master_transfer(
			req->data.map->sp,
			req->data.pins.ss_n,
			req->data.clkdiv,
			req->writeLength,
			req->writeData,
			req->readSkip,
			req->readLength,
			req->readData,
			req->flags);

	return result;
}

/*
 * After worker for SPI transfer
 */
DUK_LOCAL duk_ret_t peridot_spicon_after_work_cb(duk_context *ctx, peridot_spicon_req_t *req)
{
	/* [ int callback ] */
	void *buf;
	duk_int_t result = duk_get_int_default(ctx, 0, -1);

	if (result != 0)
	{
		/* Transfer failed */
		duk_push_error_object(ctx, DUK_ERR_ERROR, "SPI transfer failed (result=%d)", result);
		/* [ int callback err ] */
		return duk_pcall(ctx, 1);
	}

	buf = duk_push_fixed_buffer(ctx, req->readLength);
	/* [ int callback buf ] */
	memcpy(buf, req->readData, req->readLength);
	duk_push_buffer_object(ctx, 2, 0, req->readLength, DUK_BUFOBJ_NODEJS_BUFFER);
	/* [ int callback buf bufobj:3 ] */
	duk_push_undefined(ctx);
	/* [ int callback buf bufobj:3 undefined:4 ] */
	duk_replace(ctx, 2);
	/* [ int callback undefined bufobj:3 ] */
	return duk_pcall(ctx, 2);
}

/*
 * Implementation of SPIConnection.prototype.{read,transfer,write}
 */
DUK_LOCAL void peridot_spicon_transferRaw(duk_context *ctx, dux_spicon_data_peridot *data, peridot_spicon_data_t *data)
{
	/* [ obj(writeData) int(readLen) uint(filler) func:3 ] */
	duk_uint_t filler;
	peridot_spicon_req_t *req;
	req = (peridot_spicon_req_t *)dux_work_alloc(ctx, sizeof(*req),
			(dux_work_finalizer)peridot_spicon_finalize);
	memcpy(&req->data, data, sizeof(*data));

	req->writeData = dux_alloc_as_byte_buffer(ctx, 0, &req->writeLength);
	if (duk_is_boolean(ctx, 1))
	{
		// Full-duplex (Write and read)
		req->readSkip = req->writeLength;
		req->readLength = req->writeLength;
	}
	else
	{
		// Half-duplex (Write, then read)
		req->readSkip = req->writeLength;
		req->readLength = duk_require_uint(ctx, 1);
	}
	if (req->readLength > 0)
	{
		req->readData = duk_alloc(ctx, req->readLength);
		if (!req->readData)
		{
			return duk_generic_error(ctx, "Cannot allocate read buffer (length=%u)", req->readLength);
		}
	}
	filler = duk_require_uint(ctx, 2);
	req->flags =
			((filler << PERIDOT_SPI_MASTER_FILLER_OFST) &
				PERIDOT_SPI_MASTER_FILLER_MSK) | data->flags;

	dux_promise_new_with_node_callback(ctx, 3);
	/* [ obj(writeData) uint(readLen) uint(filler) func:3 promise|undefined:4 ] */
	duk_swap(ctx, 3, 4);
	/* [ obj(writeData) uint(readLen) uint(filler) promise|undefined:3 func:4 ] */
	dux_queue_work(ctx, (dux_work_t *)req, (dux_work_cb)peridot_spicon_work_cb,
			(dux_after_work_cb)peridot_spicon_after_work_cb, 1);
	/* [ obj(writeData) uint(readLen) uint(filler) promise|undefined:3 ] */
	return 1;
}

/*
 * Getter of bitrate property
 */
DUK_LOCAL duk_ret_t peridot_spicon_bitrate_getter(duk_context *ctx, peridot_spicon_data_t *data)
{
	duk_push_int(ctx, data->bitrate);
	return 1;
}

/*
 * Setter of bitrate property
 */
DUK_LOCAL duk_ret_t peridot_spicon_bitrate_setter(duk_context *ctx, peridot_spicon_data_t *data)
{
	/* [ int ] */
	duk_ret_t result;
	duk_int_t bitrate;
	alt_u32 clkdiv;

	bitrate = duk_require_int(ctx, 0);
	result = peridot_spi_master_get_clkdiv(data->map->sp, bitrate, &clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->bitrate = bitrate;
	data->clkdiv = clkdiv;
	return 0;
}

/*
 * Getter of lsbFirst property
 */
DUK_LOCAL duk_ret_t peridot_spicon_lsbFirst_getter(duk_context *ctx, peridot_spicon_data_t *data)
{
	duk_push_boolean(ctx, (data->flags & PERIDOT_SPI_MASTER_LSBFIRST));
	return 1;
}

/*
 * Setter of lsbFirst property
 */
DUK_LOCAL duk_ret_t peridot_spicon_lsbFirst_setter(duk_context *ctx, peridot_spicon_data_t *data)
{
	/* [ boolean ] */
	if (duk_require_boolean(ctx, 0))
	{
		data->flags |= PERIDOT_SPI_MASTER_LSBFIRST;
	}
	else
	{
		data->flags &= ~PERIDOT_SPI_MASTER_LSBFIRST;
	}
	return 0;
}

/*
 * Getter of mode property
 */
DUK_LOCAL duk_ret_t peridot_spicon_mode_getter(duk_context *ctx, peridot_spicon_data_t *data)
{
	duk_push_uint(ctx, (data->mode & PERIDOT_SPI_MASTER_MODE_MSK) >> PERIDOT_SPI_MASTER_MODE_OFST);
	return 1;
}

/*
 * Setter of mode property
 */
DUK_LOCAL duk_ret_t peridot_spicon_mode_setter(duk_context *ctx, peridot_spicon_data_t *data)
{
	/* [ uint ] */
	duk_uint_t mode = duk_require_uint(ctx, 0);
	data->flags &= ~PERIDOT_SPI_MASTER_MODE_MSK;
	data->flags |= (mode << PERIDOT_SPI_MASTER_MODE_OFST) & PERIDOT_SPI_MASTER_MODE_MSK;
	return 0;
}

/*
 * Getter of slaveSelect property
 */
DUK_LOCAL duk_ret_t peridot_spicon_slaveSelect_getter(duk_context *ctx, peridot_spicon_data_t *data)
{
	duk_push_uint(ctx, data->pins.ss_n);
	return 1;
}

/*
 * Function table for PERIDOT based SPIConnection
 */
DUK_LOCAL const dux_spicon_functions peridot_spicon_functions = {
	.transferRaw = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_transferRaw,
	.bitrate_getter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_bitrate_getter,
	.bitrate_setter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_bitrate_setter,
	.lsbFirst_getter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_lsbFirst_getter,
	.lsbFirst_setter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_lsbFirst_setter,
	.mode_getter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_mode_getter,
	.mode_setter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_mode_setter,
	.slaveSelect_getter = (duk_ret_t (*)(duk_context *, void *))peridot_spicon_slaveSelect_getter,
};

/*
 * Entry of PeridotSPI constructor
 */
DUK_LOCAL duk_ret_t peridot_spi_constructor(duk_context *ctx)
{
	peridot_spi_pins_t pins;
	duk_ret_t result;

	/* [ obj ] */
	result = peridot_spi_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	pins.ss_n = -1;

	duk_push_this(ctx);
	/* [ obj this ] */
	duk_push_uint(ctx, pins.uint);
	/* [ obj this uint ] */
	duk_put_prop_string(ctx, 1, DUX_IPK_PERIDOT_SPI_PINS);
	/* [ obj this ] */
	return 0; /* return this */
}

/*
 * Body of PeridotSPI.connect() / PeridotSPI.prototype.connect()
 */
DUK_LOCAL duk_ret_t peridot_spi_connect_body(duk_context *ctx, peridot_spi_pins_t *pins)
{
	/* [ con_constructor uint/undefined ] */
	extern peridot_spi_master_pfc_map *const peridot_spi_drivers[];
	peridot_spicon_data_t *data;
	duk_int_t bitrate;
	int map_idx;
	int result;

	// Get bitrate
	if (duk_is_null_or_undefined(ctx, 1))
	{
		bitrate = PERIDOT_SPI_DEFAULT_BITRATE;
	}
	else
	{
		bitrate = duk_require_uint(ctx, 1);
	}

	duk_pop(ctx);
	/* [ con_constructor ] */

	data = (peridot_spicon_data_t *)duk_push_fixed_buffer(ctx, sizeof(*data));
	/* [ con_constructor buf ] */
	data->bitrate = bitrate;
	data->pins.uint = pins->uint;

	// Lookup driver
	for (map_idx = 0;; ++map_idx)
	{
		peridot_spi_master_pfc_map *map;
		map = peridot_spi_drivers[map_idx];
		if (!map)
		{
			// No more drivers
			return duk_generic_error(ctx, "No SPI driver (SS_N=%d, SCLK=%d, MOSI=%d, MISO=%d)",
				pins->ss_n, pins->sclk, pins->mosi, pins->miso);
		}
		
		if (peridot_spi_master_configure_pins(map, pins->sclk, pins->mosi, pins->miso, 1) == 0)
		{
			// Found
			data->map = map;
			break;
		}
	}

	result = peridot_spi_master_get_clkdiv(data->map, data->bitrate, &data->clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	duk_push_pointer(ctx, (void *)&peridot_spicon_functions);
	/* [ con_constructor buf ] */
	duk_new(ctx, 2);
	/* [ instance ] */
	return 1;
}

/*
 * Entry of PeridotSPI.connect()
 */
DUK_LOCAL duk_ret_t peridot_spi_connect(duk_context *ctx)
{
	spi_pins_t pins;
	duk_ret_t result;

	/* [ obj uint/undefined ] */
	result = peridot_spi_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	/* [ obj uint/undefined ] */
	duk_push_this(ctx);
	/* [ obj uint/undefined constructor ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_SPICON);
	/* [ obj uint/undefined constructor con_constructor:3 ] */
	duk_replace(ctx, 0);
	/* [ con_constructor uint/undefined constructor ] */
	duk_pop(ctx);
	/* [ con_constructor uint/undefined ] */
	return peridot_spi_connect_body(ctx, &pins);
}

/*
 * Entry of PeridotSPI.prototype.connect()
 */
DUK_LOCAL duk_ret_t peridot_spi_proto_connect(duk_context *ctx)
{
	spi_pins_t pins;

	/* [ obj uint/undefined ] */
	duk_push_this(ctx);
	/* [ obj uint/undefined this ] */
	dux_push_constructor(ctx, 2);
	/* [ obj uint/undefined this constructor:3 ] */
	duk_get_prop_string(ctx, 3, DUX_IPK_PERIDOT_SPI_SPICON);
	/* [ obj uint/undefined this constructor:3 con_constructor:4 ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_SPI_PINS);
	/* [ obj uint/undefined this constructor:3 con_constructor:4 pins:5 ] */
	pins.uint = duk_require_uint(ctx, 5);
	pins.ss_n = dux_get_peridot_pin(ctx, 0);
	duk_pop(ctx);
	/* [ obj uint/undefined this constructor:3 con_constructor:4 ] */
	duk_replace(ctx, 0);
	/* [ con_constructor uint/undefined this constructor:3 ] */
	duk_pop_2(ctx);
	/* [ con_constructor uint/undefined ] */
	return peridot_spi_connect_body(ctx, &pins);
}

/*
 * Getter of PeridotSPI.prototype.pins
 */
DUK_LOCAL duk_ret_t peridot_spi_proto_pins_getter(duk_context *ctx)
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
DUK_LOCAL const duk_function_list_entry peridot_spi_funcs[] = {
	{ "connect", peridot_spi_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype methods
 */
DUK_LOCAL const duk_function_list_entry peridot_spi_proto_funcs[] = {
	{ "connect", peridot_spi_proto_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL const dux_property_list_entry peridot_spi_proto_props[] = {
	{ "pins", peridot_spi_proto_pins_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize PeridotSPI submodule
 */
DUK_INTERNAL duk_errcode_t dux_peridot_spi_init(duk_context *ctx)
{
	/* [ require module exports ] */
	dux_push_named_c_constructor(
			ctx, "PeridotSPI", peridot_spi_constructor, 1,
			peridot_spi_funcs, peridot_spi_proto_funcs,
			NULL, peridot_spi_proto_props);
	/* [ require module exports constructor:3 ] */
	duk_dup(ctx, 0);
	duk_push_string(ctx, "hardware");
	duk_call(ctx, 1);
	/* [ require module exports constructor:3 hardware:4 ] */
	duk_get_prop_string(ctx, 4, "SPIConnection");
	/* [ require module exports constructor:3 hardware:4 SPIConnection:5 ] */
	duk_put_prop_string(ctx, 3, DUX_IPK_PERIDOT_SPI_SPICON);
	/* [ require module exports constructor:3 hardware:4 ] */
	duk_pop(ctx);
	/* [ require module exports constructor:3 ] */
	duk_put_prop_string(ctx, 2, "SPI");
	/* [ require module exports ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */
#endif  /* DUX_USE_BOARD_PERIDOT */
