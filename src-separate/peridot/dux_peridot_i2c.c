/*
 * ECMA classes:
 *   new PeridotI2C(
 *     {
 *       scl: <uint>,
 *       sda: <uint>
 *     }
 *   );
 *   global.Peridot.I2C = PeridotI2C;
 *
 * ECMA methods:
 *   PeridotI2C.connect(pins, slaveAddress[, bitrate])
 *   PeridotI2C.prototype.connect(slaveAddress[, bitrate])
 */
#if defined(DUX_USE_BOARD_PERIDOT)
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)
#include "../dux_internal.h"
#include <system.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>

/*
 * Constants
 */

#define I2C_DEFAULT_BITRATE	100000

static const char *const DUX_I2C_PINS = "\xff" "pins";
static const char *const DUX_I2C_POOLS = "dux_i2c_peridot.pools";
static const char *const DUX_I2C_DRIVER = "\xff" "dux_i2c_peridot.driver";
static const char *const DUX_I2C_POOL = "\xff" "pool";

enum
{
	I2C_BLKIDX_DATA = 0,
	I2C_BLKIDX_CLKDIV,
	I2C_BLKIDX_WRITEDATA,
	I2C_BLKIDX_READDATA,
	I2C_BLKIDX_CALLBACK,
	I2C_BLKIDX_THIS,
	I2C_NUM_BLOCKS,
};

/*
 * Structures
 */

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

typedef struct dux_i2ccon_data_peridot
{
	dux_i2ccon_data common;
	i2c_pins_t pins;
	peridot_i2c_master_state *driver;
	alt_u32 clkdiv;
}
dux_i2ccon_data_peridot;

/*
 * Push pool array
 */
static void i2c_push_pools(duk_context *ctx)
{
	/* [ ... ] */
	duk_get_global_string(ctx, "Peridot");
	/* [ ... obj ] */
	if (!duk_get_prop_string(ctx, -1, DUX_IPK_PERIDOT_I2C_POOLS))
	{
		/* [ ... stath undefined ] */
		duk_pop(ctx);
		duk_push_array(ctx);
		duk_dup_top(ctx);
		/* [ ... stash arr arr ] */
		duk_put_prop_string(ctx, -3, DUX_IPK_PERIDOT_I2C_POOLS);
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
 * Thread pool worker for I2C transfer
 */
duk_int_t i2ccon_worker(const dux_thrpool_block_t *blocks, duk_size_t num_blocks)
{
	dux_i2ccon_data_peridot *data;
	int result;

	if (num_blocks != I2C_NUM_BLOCKS)
	{
		return -EBADF;
	}

	data = (dux_i2ccon_data_peridot *)blocks[I2C_BLKIDX_DATA].pointer;

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
			blocks[I2C_BLKIDX_CLKDIV].uint,
			blocks[I2C_BLKIDX_WRITEDATA].length,
			blocks[I2C_BLKIDX_WRITEDATA].pointer,
			blocks[I2C_BLKIDX_READDATA].length,
			blocks[I2C_BLKIDX_READDATA].pointer);

	return result;
}

/*
 * Thread pool completer for I2C transfer
 */
duk_ret_t i2ccon_completer(duk_context *ctx)
{
	/* [ job int ] */
	duk_int_t ret = duk_require_int(ctx, 1);
	duk_get_prop_index(ctx, 0, I2C_BLKIDX_CALLBACK);
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
		duk_push_error_object(ctx, DUK_ERR_API_ERROR, "I2C transfer failed (%d)", ret);
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
duk_ret_t i2ccon_transfer(duk_context *ctx, dux_i2ccon_data_peridot *data, duk_uint_t readlen)
{
	/* [ ... val(writedata) func ] */
	duk_push_array(ctx);
	duk_swap_top(ctx, -3);
	/* [ ... arr func val(writedata) ] */
	duk_put_prop_index(ctx, -3, I2C_BLKIDX_WRITEDATA);
	/* [ ... arr func ] */
	duk_put_prop_index(ctx, -2, I2C_BLKIDX_CALLBACK);
	/* [ ... arr ] */
	if (readlen > 0)
	{
		duk_push_fixed_buffer(ctx, readlen);
	}
	else
	{
		duk_push_undefined(ctx);
	}
	duk_put_prop_index(ctx, -2, I2C_BLKIDX_READDATA);
	/* [ ... arr ] */
	duk_push_external_buffer(ctx);
	duk_config_buffer(ctx, -1, data, sizeof(*data));
	duk_put_prop_index(ctx, -2, I2C_BLKIDX_DATA);
	/* [ ... arr ] */
	duk_push_uint(ctx, data->clkdiv);
	duk_put_prop_index(ctx, -2, I2C_BLKIDX_CLKDIV);
	/* [ ... arr ] */
	duk_push_this(ctx);
	duk_put_prop_index(ctx, -2, I2C_BLKIDX_THIS);
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
 * Native implementation for bitrate change
 */
static duk_ret_t i2ccon_update_bitrate(duk_context *ctx, dux_i2ccon_data_peridot *data)
{
	duk_ret_t result;
	alt_u32 clkdiv;

	result = peridot_i2c_master_get_clkdiv(data->driver,
				data->common.bitrate, &clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->clkdiv = clkdiv;
	return 0;
}

/*
 * Common implementation of PeridotI2C.connect/PeridotI2C.prototype.connect
 */
DUK_LOCAL duk_ret_t i2c_connect_body(duk_context *ctx, i2c_pins_t *pins)
{
	dux_i2ccon_data_peridot *data;
	duk_ret_t result;
	duk_uint_t slaveAddress;
	duk_uint_t bitrate;
	peridot_i2c_master_state *driver;

	/* [ uint(slaveAddr) uint(bitrate)/undefined this ] */
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

	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_I2C_POOLS);
	/* [ uint uint/undefined this arr ] */
	duk_swap(ctx, 0, 3);
	/* [ arr uint/undefined this uint ] */
	duk_set_top(ctx, 1);
	/* [ arr ] */

	dux_push_i2ccon_class(ctx);
	/* [ arr constructor ] */
	duk_push_fixed_buffer(ctx, sizeof(*data));
	/* [ arr constructor buf ] */
	data = (dux_i2ccon_data_peridot *)duk_get_buffer(ctx, -1, NULL);
	duk_new(ctx, 1);
	/* [ arr i2ccon ] */

	data->common.transfer =
		(duk_ret_t (*)(duk_context *, dux_i2ccon_t *, duk_uint_t))i2ccon_transfer;
	data->common.update_bitrate =
		(duk_ret_t (*)(duk_context *, dux_i2ccon_t *))i2ccon_update_bitrate;
	data->common.slaveAddress = slaveAddress;
	data->common.bitrate = bitrate;
	data->pins.uint = pins->uint;
	data->driver = NULL;

	duk_enum(ctx, 0, DUK_ENUM_ARRAY_INDICES_ONLY);
	/* [ arr i2ccon enum ] */
	while (duk_next(ctx, 2, 1))
	{
		/* [ arr i2ccon enum key thrpool:4 ] */
		duk_get_prop_string(ctx, 4, DUX_I2C_DRIVER);
		/* [ arr i2ccon enum key thrpool:4 pointer:5 ] */
		driver = (peridot_i2c_master_state *)duk_get_pointer(ctx, 5);
		duk_pop(ctx);
		/* [ arr i2ccon enum key thrpool:4 ] */

		if ((driver) &&
			peridot_i2c_master_configure_pins(driver, pins->scl, pins->sda, 1) == 0)
		{
			data->driver = driver;
			duk_put_prop_string(ctx, 0, DUX_IPK_PERIDOT_I2C_POOL);
			/* [ arr i2ccon enum key ] */
			duk_pop(ctx);
			/* [ arr i2ccon enum ] */
			break;
		}

		duk_pop_2(ctx);
		/* [ arr i2ccon enum ] */
	}
	duk_pop(ctx);
	/* [ arr i2ccon ] */

	if (!data->driver)
	{
		duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR,
				"no driver supports (scl=%u,sda=%u)", pins->scl, pins->sda);
		/* unreachable */
		return 0;
	}

	result = (*data->common.update_bitrate)(ctx, data);
	if (result != 0)
	{
		return result;
	}

	/* [ arr i2ccon ] */
	return 1; /* return i2ccon */
}

/*
 * Entry of PeridotI2C constructor
 */
DUK_LOCAL duk_ret_t i2c_constructor(duk_context *ctx)
{
	i2c_pins_t pins;
	duk_ret_t result;

	/* [ obj ] */
	result = i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}

	duk_push_current_function(ctx);
	duk_push_this(ctx);
	/* [ obj constructor this ] */
	duk_push_uint(ctx, pins.uint);
	duk_put_prop_string(ctx, 2, DUX_IPK_PERIDOT_I2C_PINS);
	duk_get_prop_string(ctx, 1, DUX_IPK_PERIDOT_I2C_POOLS);
	duk_put_prop_string(ctx, 2, DUX_IPK_PERIDOT_I2C_POOLS);

	return 0; /* return this */
}

/*
 * Entry of PeridotI2C.connect()
 */
DUK_LOCAL duk_ret_t i2c_connect(duk_context *ctx)
{
	i2c_pins_t pins;
	duk_ret_t result;

	/* [ obj uint uint/undefined ] */
	result = i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	duk_remove(ctx, 0);
	/* [ uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ uint uint/undefined this ] */

	return i2c_connect_body(ctx, &pins);
}

/*
 * Entry of PeridotI2C.prototype.connect()
 */
DUK_LOCAL duk_ret_t i2c_proto_connect(duk_context *ctx)
{
	i2c_pins_t pins;

	/* [ uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ uint uint/undefined this ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PERIDOT_I2C_PINS);
	/* [ uint uint/undefined this uint ] */
	pins.uint = duk_require_uint(ctx, 2);
	duk_pop(ctx);
	/* [ uint uint/undefined this ] */

	return i2c_connect_body(ctx, &pins);
}

/*
 * Getter of PeridotI2C.prototype.pins
 */
DUK_LOCAL duk_ret_t i2c_proto_pins_getter(duk_context *ctx)
{
	i2c_pins_t pins;

	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	duk_push_object(ctx);
	/* [ this obj ] */
	duk_get_prop_string(ctx, 0, DUX_IPK_PERIDOT_I2C_PINS);
	/* [ this obj uint ] */
	pins.uint = duk_require_uint(ctx, 2);
	duk_push_uint(ctx, pins.scl);
	duk_put_prop_string(ctx, 1, "scl");
	duk_push_uint(ctx, pins.sda);
	duk_put_prop_string(ctx, 1, "sda");
	return 1; /* return obj */
}

/*
 * List of class methods
 */
DUK_LOCAL duk_function_list_entry i2c_funcs[] = {
	{ "connect", i2c_connect, 3 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype methods
 */
DUK_LOCAL duk_function_list_entry i2c_proto_funcs[] = {
	{ "connect", i2c_proto_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL dux_property_list_entry i2c_proto_props[] = {
	{ "pins", i2c_proto_pins_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize PeridotI2C submodule
 */
DUK_INTERNAL duk_errcode_t dux_peridot_i2c_init(duk_context *ctx)
{
	extern peridot_i2c_master_state *const peridot_i2c_drivers[];
	duk_idx_t index;

	/* [ ... obj ] */
	dux_push_named_c_constructor(
			ctx, "PeridotI2C", i2c_constructor, 1,
			i2c_funcs, i2c_proto_funcs, NULL, NULL);
	/* [ ... obj constructor ] */
	duk_push_array(ctx);
	/* [ ... obj constructor arr ] */
	for (index = 0;; ++index)
	{
		peridot_i2c_master_state *driver;
		driver = peridot_i2c_drivers[index];
		if (!driver)
		{
			break;
		}
		dux_push_thrpool(ctx, 1, 1);
		duk_push_pointer(ctx, driver);
		/* [ ... obj constructor arr thrpool pointer ] */
		duk_put_prop_string(ctx, -2, DUX_IPK_PERIDOT_I2C_DRIVER);
		/* [ ... obj constructor arr thrpool ] */
		duk_put_prop_index(ctx, -2, index);
		/* [ ... obj constructor arr ] */
	}

	duk_put_prop_string(ctx, -2, DUX_IPK_PERIDOT_I2C_POOLS);
	/* [ ... obj constructor ] */

	if (index == 0)
	{
		// No driver
		duk_pop(ctx);
		/* [ ... obj ] */
		return DUK_ERR_UNSUPPORTED_ERROR;
	}

	duk_put_prop_string(ctx, -2, "I2C");
	/* [ ... obj ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */
#endif  /* DUX_USE_BOARD_PERIDOT */
