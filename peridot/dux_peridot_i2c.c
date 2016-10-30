#include "dux_i2ccon.h"
#include "dux_common.h"
#include "dux_peridot.h"
#include "dux_thrpool.h"
#include "system.h"
#include "peridot_i2c_master.h"
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>

extern void top_level_error(duk_context *ctx);

#define I2C_DEFAULT_BITRATE	100000

static const char *const DUX_I2C_PINS = "\xff" "pins";
static const char *const DUX_I2C_POOLS = "dux_i2c_peridot.pools";
static const char *const DUX_I2C_DRIVER = "\xff" "dux_i2c_peridot.driver";
static const char *const DUX_I2C_POOL = "\xff" "pool";

typedef union i2c_pins_t
{
	duk_uint_t uint;
	struct
	{
		duk_int8_t scl;
		duk_int8_t sda;
	};
}
i2c_pins_t;

typedef struct dux_i2ccon_peridot_t
{
	dux_i2ccon_t common;
	i2c_pins_t pins;
	peridot_i2c_master_state *driver;
	alt_u32 clkdiv;
}
dux_i2ccon_peridot_t;

/*
 * Push pool array
 */
static void i2c_push_pools(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_I2C_POOLS))
	{
		/* [ ... stath undefined ] */
		duk_pop(ctx);
		duk_push_array(ctx);
		duk_dup_top(ctx);
		/* [ ... stash arr arr ] */
		duk_put_prop_string(ctx, -3, DUX_I2C_POOLS);
	}
	/* [ ... stash arr ] */
	duk_remove(ctx, -2);
	/* [ ... arr ] */
}

/*
 * Read pin configurations from ECMA object to i2c_pins_t
 */
static duk_ret_t i2c_get_pins(duk_context *ctx, duk_idx_t index, i2c_pins_t *pins)
{
	pins->uint = 0;
	pins->scl = dux_get_peridot_pin(ctx, index, "scl");
	pins->sda = dux_get_peridot_pin(ctx, index, "sda");
	if ((pins->scl < 0) || (pins->sda < 0))
	{
		return DUK_RET_RANGE_ERROR;
	}
	return 0;
}

/*
 * C function entry of PeridotI2C.constructor
 * (<obj> pins)
 */
static duk_ret_t i2c_constructor(duk_context *ctx)
{
	i2c_pins_t pins;
	duk_ret_t result;
	result = i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}

	duk_push_this(ctx);
	duk_push_uint(ctx, pins.uint);
	duk_put_prop_string(ctx, -2, DUX_I2C_PINS);

	return 0;
}

/*
 * Thread pool worker for I2C transfer
 */
duk_int_t i2ccon_worker(const dux_thrpool_block_t *blocks, duk_size_t num_blocks)
{
	dux_i2ccon_peridot_t *data;
	int result;

	if (num_blocks != 5)
	{
		return -EBADF;
	}

	data = (dux_i2ccon_peridot_t *)blocks[0].pointer;

	result = peridot_i2c_master_configure_pins(
			data->driver,
			data->pins.scl,
			data->pins.sda,
			0);
	if (result < 0)
	{
		return result;
	}

	result = peridot_i2c_master_transfer(
			data->driver,
			data->common.slaveAddress,
			data->clkdiv,
			blocks[1].length, blocks[1].pointer,
			blocks[2].length, blocks[2].pointer);

	return result;
}

/*
 * Thread pool completer for I2C transfer
 */
duk_ret_t i2ccon_completer(duk_context *ctx)
{
	/* [ job int ] */
	duk_int_t ret = duk_require_int(ctx, 1);
	duk_get_prop_index(ctx, 0, 3);
	/* [ job int func ] */

	if (ret == 0)
	{
		// Success
		if (!duk_is_callable(ctx, -1))
		{
			return 0;
		}
		duk_push_true(ctx);
		/* [ job int func true ] */
		duk_get_prop_index(ctx, 0, 2);	/* readbuf */
		/* [ job int func true buf ] */
		duk_push_buffer_object(ctx, -1, 0, duk_get_length(ctx, -1), DUK_BUFOBJ_ARRAYBUFFER);
		/* [ job int func true buf bufobj(ArrayBuffer) ] */
		duk_remove(ctx, -2);
		/* [ job int func true bufobj(ArrayBuffer) ] */
		duk_call(ctx, 2);
		/* [ job int retval ] */
	}
	else
	{
		// Failed
		duk_push_false(ctx);
		/* [ job int func false ] */
		duk_push_error_object(ctx, DUK_ERR_API_ERROR, "I2C transfer failed");
		/* [ job int func false err ] */
		if (duk_is_callable(ctx, -3))
		{
			duk_call(ctx, 2);
		}
		else
		{
			top_level_error(ctx);
		}
	}

	return 0;
}

/*
 * Native implementation for I2CConnection.prototype.{read,transfer,write}
 */
duk_ret_t i2ccon_transfer(duk_context *ctx, dux_i2ccon_peridot_t *data, duk_uint_t readlen)
{
	/* [ ... val(writedata) func ] */
	duk_push_array(ctx);
	duk_swap_top(ctx, -3);
	/* [ ... arr func val(writedata) ] */
	duk_put_prop_index(ctx, -3, 1);	/* val(writedata) */
	/* [ ... arr func ] */
	duk_put_prop_index(ctx, -2, 3);	/* func(callback) */
	/* [ ... arr ] */
	if (readlen > 0)
	{
		duk_push_fixed_buffer(ctx, readlen);
	}
	else
	{
		duk_push_undefined(ctx);
	}
	duk_put_prop_index(ctx, -2, 2);	/* buf(readdata) */
	/* [ ... arr ] */
	duk_push_external_buffer(ctx);
	duk_config_buffer(ctx, -1, data, sizeof(*data));
	duk_put_prop_index(ctx, -2, 0);	/* buf(data) */
	/* [ ... arr ] */
	duk_push_this(ctx);
	duk_put_prop_index(ctx, -2, 4);	/* this */
	/* [ ... arr ] */

	duk_push_this(ctx);
	/* [ ... arr i2ccon ] */
	duk_get_prop_string(ctx, -1, DUX_I2C_POOL);
	/* [ ... arr i2ccon thrpool ] */
	duk_swap_top(ctx, -3);
	/* [ ... thrpool i2ccon arr ] */
	dux_thrpool_queue(ctx, -3, i2ccon_worker, i2ccon_completer);

	return 0;	/* return undefined; */
}

/*
 * Native implementation for PeridotI2C.connect/PeridotI2C.prototype.connect
 */
static duk_ret_t i2c_connect_body(duk_context *ctx, i2c_pins_t *pins)
{
	/* [ uint(slaveAddr) uint(bitrate)/undef ] */
	dux_i2ccon_peridot_t *data;
	duk_ret_t result;
	duk_uint_t slaveAddress;
	duk_uint_t bitrate;
	peridot_i2c_master_state *driver;

	slaveAddress = duk_require_uint(ctx, 0);
	if (slaveAddress > 0x7f)
	{
		return DUK_RET_RANGE_ERROR;
	}

	if (duk_is_null_or_undefined(ctx, 1))
	{
		bitrate = I2C_DEFAULT_BITRATE;
	}
	else
	{
		bitrate = duk_require_uint(ctx, 1);
	}

	duk_set_top(ctx, 0);
	/* [  ] */

	dux_push_i2ccon_class(ctx);
	duk_push_fixed_buffer(ctx, sizeof(*data));
	data = (dux_i2ccon_peridot_t *)duk_get_buffer(ctx, -1, NULL);
	duk_new(ctx, 1);
	/* [ i2ccon ] */

	data->common.transfer =
		(duk_ret_t (*)(duk_context *, dux_i2ccon_t *, duk_uint_t))i2ccon_transfer;
	data->common.slaveAddress = slaveAddress;
	data->common.bitrate = bitrate;
	data->pins.uint = pins->uint;
	data->driver = NULL;

	i2c_push_pools(ctx);
	/* [ i2ccon arr ] */
	duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
	/* [ i2ccon arr enum ] */
	while (duk_next(ctx, -1, 1))
	{
		/* [ i2ccon arr enum key thrpool ] */
		duk_get_prop_string(ctx, -1, DUX_I2C_DRIVER);
		driver = (peridot_i2c_master_state *)duk_get_pointer(ctx, -1);
		duk_pop(ctx);

		if ((driver) &&
			peridot_i2c_master_configure_pins(driver, pins->scl, pins->sda, 1) == 0)
		{
			data->driver = driver;
			duk_put_prop_string(ctx, 0, DUX_I2C_POOL);
			/* [ i2ccon arr enum key ] */
			duk_pop(ctx);
			/* [ i2ccon arr enum ] */
			break;
		}

		duk_pop_2(ctx);
		/* [ i2ccon arr enum ] */
	}
	duk_pop_2(ctx);
	/* [ i2ccon ] */

	if (!data->driver)
	{
		duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR,
				"no driver supports (scl=%u,sda=%u)", pins->scl, pins->sda);
		// never returns
	}

	result = peridot_i2c_master_get_clkdiv(data->driver,
				data->common.bitrate, &data->clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	/* [ i2ccon ] */
	return 1;
}

/*
 * C function entry of PeridotI2C.connect
 * (<obj> pins, <uint> slaveAddr, [<uint> bitrate = 100000])
 */
static duk_ret_t i2c_connect(duk_context *ctx)
{
	i2c_pins_t pins;
	duk_ret_t result;

	result = i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}

	duk_remove(ctx, 0);
	return i2c_connect_body(ctx, &pins);
}

/*
 * C function entry of PeridotI2C.connect
 * (<uint> slaveAddr, [<uint> bitrate = 100000])
 */
static duk_ret_t i2c_proto_connect(duk_context *ctx)
{
	i2c_pins_t pins;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_I2C_PINS);
	pins.uint = duk_require_uint(ctx, -1);
	duk_pop(ctx);

	return i2c_connect_body(ctx, &pins);
}

static duk_function_list_entry i2c_funcs[] = {
	{ "connect", i2c_connect, 1 },
	{ NULL, NULL, 0 }
};

static duk_function_list_entry i2c_proto_funcs[] = {
	{ "connect", i2c_proto_connect, 3 },
	{ NULL, NULL, 0 }
};

/*
 * Initialize I2C instances
 */
void dux_peridot_i2c_init(duk_context *ctx, ...)
{
	/* [ ... ] */
	va_list args;
	duk_uarridx_t idx;

	dux_i2ccon_init(ctx);

	duk_push_heap_stash(ctx);
	duk_push_array(ctx);
	/* [ ... stash arr ] */

	va_start(args, ctx);
	for (idx = 0;; ++idx)
	{
		peridot_i2c_master_state *driver;
		driver = va_arg(args, peridot_i2c_master_state *);
		if (!driver)
		{
			break;
		}
		dux_push_thrpool(ctx, 1, 1);
		duk_push_pointer(ctx, driver);
		duk_put_prop_string(ctx, -2, DUX_I2C_DRIVER);
		/* [ ... stash arr thrpool ] */
		duk_put_prop_index(ctx, -2, idx);
		/* [ ... stash arr ] */
	}
	va_end(args);

	duk_put_prop_string(ctx, -2, DUX_I2C_POOLS);
	duk_pop(ctx);
	/* [ ... ] */

	if (idx == 0)
	{
		// No driver
		return;
	}

	duk_get_global_string(ctx, "Peridot");
	/* [ ... obj(Peridot) ] */

	dux_push_named_c_constructor(
			ctx, "PeridotI2C", i2c_constructor, 1,
			i2c_funcs, i2c_proto_funcs);
	/* [ ... obj(Peridot) func(PeridotI2C) ] */

	duk_put_prop_string(ctx, -2, "I2C");
	duk_pop(ctx);
	/* [ ... ] */
}

