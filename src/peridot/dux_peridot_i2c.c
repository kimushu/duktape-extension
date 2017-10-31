#if defined(DUX_USE_BOARD_PERIDOT)
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)
#include "../dux_internal.h"
#include <system.h>
#include <peridot_i2c_master.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>

/*
 * Constants
 */

#define PERIDOT_I2C_DEFAULT_BITRATE 100000

DUK_LOCAL const char DUX_IPK_PERIDOT_I2C_I2CCON[] = DUX_IPK("bI2CCon");
DUK_LOCAL const char DUX_IPK_PERIDOT_I2C_PINS[] = DUX_IPK("bI2CPins");

/*
 * Structures
 */

typedef union
{
	duk_uint_t uint;
	struct
	{
		duk_int8_t scl;
		duk_int8_t sda;
	};
}
peridot_i2c_pins_t;

typedef struct
{
	duk_int_t slaveAddress;
	duk_int_t bitrate;
	peridot_i2c_pins_t pins;
	peridot_i2c_master_state *driver;
	alt_u32 clkdiv;
}
peridot_i2ccon_data_t;

typedef struct {
	peridot_i2ccon_data_t data;
	duk_uint_t writeLength;
	const void *writeData;
	duk_uint_t readLength;
	void *readData;
}
peridot_i2ccon_req_t;

/*
 * Read pin configurations from ECMA object to peridot_i2c_pins_t
 */
DUK_LOCAL duk_ret_t peridot_i2c_get_pins(duk_context *ctx, duk_idx_t index, peridot_i2c_pins_t *pins)
{
	pins->uint = 0;
	pins->scl = dux_get_peridot_pin_by_key(ctx, index, "scl", NULL);
	pins->sda = dux_get_peridot_pin_by_key(ctx, index, "sda", NULL);
	if ((pins->scl < 0) || (pins->sda < 0))
	{
		return DUK_RET_RANGE_ERROR;
	}
	return 0;
}

DUK_LOCAL void peridot_i2ccon_finalize(duk_context *ctx, peridot_i2ccon_req_t *req)
{
	duk_free(ctx, (void *)req->writeData);
	duk_free(ctx, req->readData);
}

/*
 * Worker for I2C transfer
 */
DUK_LOCAL duk_int_t peridot_i2ccon_work_cb(peridot_i2ccon_req_t *req)
{
	int result;

	result = peridot_i2c_master_configure_pins(
			req->data.driver,
			req->data.pins.scl,
			req->data.pins.sda,
			0);
	if (result < 0)
	{
		return result;
	}

	result = peridot_i2c_master_transfer(
			req->data.driver,
			req->data.slaveAddress,
			req->data.clkdiv,
			req->writeLength,
			req->writeData,
			req->readLength,
			req->readData);

	return result;
}

/*
 * After worker for I2C transfer
 */
DUK_LOCAL duk_ret_t peridot_i2ccon_after_work_cb(duk_context *ctx, peridot_i2ccon_req_t *req)
{
	/* [ int callback ] */
	void *buf;
	duk_int_t result = duk_get_int_default(ctx, 0, -1);

	if (result != 0)
	{
		/* Transfer failed */
		duk_push_error_object(ctx, DUK_ERR_ERROR, "I2C transfer failed (result=%d)", result);
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
 * Implementation of I2CConnection.prototype.transfer
 */
DUK_LOCAL duk_ret_t peridot_i2ccon_transfer(duk_context *ctx, peridot_i2ccon_data_t *data)
{
	/* [ obj uint func ] */
	peridot_i2ccon_req_t *req;
	req = (peridot_i2ccon_req_t *)dux_work_alloc(ctx, sizeof(*req),
			(dux_work_finalizer)peridot_i2ccon_finalize);
	memcpy(&req->data, data, sizeof(*data));

	req->writeData = dux_alloc_as_byte_buffer(ctx, 0, &req->writeLength);
	req->readLength = duk_require_uint(ctx, 1);
	if (req->readLength > 0)
	{
		req->readData = duk_alloc(ctx, req->readLength);
		if (!req->readData)
		{
			return duk_generic_error(ctx, "Cannot allocate read buffer (length=%u)", req->readLength);
		}
	}

	dux_promise_new_with_node_callback(ctx, 2);
	/* [ obj uint func promise|undefined ] */
	duk_swap(ctx, 2, 3);
	/* [ obj uint promise|undefined func ] */
	dux_queue_work(ctx, (dux_work_t *)req, (dux_work_cb)peridot_i2ccon_work_cb,
			(dux_after_work_cb)peridot_i2ccon_after_work_cb, 1);
	/* [ obj uint promise|undefined ] */
	return 1;
}

/*
 * Getter of slaveAddress property
 */
DUK_LOCAL duk_ret_t peridot_i2ccon_slaveAddress_getter(duk_context *ctx, peridot_i2ccon_data_t *data)
{
	duk_push_int(ctx, data->slaveAddress);
	return 1;
}

/*
 * Getter of bitrate property
 */
DUK_LOCAL duk_ret_t peridot_i2ccon_bitrate_getter(duk_context *ctx, peridot_i2ccon_data_t *data)
{
	duk_push_int(ctx, data->bitrate);
	return 1;
}

/*
 * Setter of bitrate property
 */
DUK_LOCAL duk_ret_t peridot_i2ccon_bitrate_setter(duk_context *ctx, peridot_i2ccon_data_t *data)
{
	/* [ int ] */
	duk_ret_t result;
	duk_int_t bitrate;
	alt_u32 clkdiv;

	bitrate = duk_require_int(ctx, 0);
	result = peridot_i2c_master_get_clkdiv(data->driver, bitrate, &clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->bitrate = bitrate;
	data->clkdiv = clkdiv;
	return 0;
}

/*
 * Function table for PERIDOT based I2CConnection
 */
DUK_LOCAL const dux_i2ccon_functions peridot_i2ccon_functions = {
	.transfer = (duk_ret_t (*)(duk_context *, void *))peridot_i2ccon_transfer,
	.slaveAddress_getter = (duk_ret_t (*)(duk_context *, void *))peridot_i2ccon_slaveAddress_getter,
	.bitrate_getter = (duk_ret_t (*)(duk_context *, void *))peridot_i2ccon_bitrate_getter,
	.bitrate_setter = (duk_ret_t (*)(duk_context *, void *))peridot_i2ccon_bitrate_setter,
};

/*
 * Entry of PeridotI2C constructor
 */
DUK_LOCAL duk_ret_t peridot_i2c_constructor(duk_context *ctx)
{
	peridot_i2c_pins_t pins;
	duk_ret_t result;

	/* [ obj ] */
	result = peridot_i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}

	duk_push_this(ctx);
	/* [ obj this ] */
	duk_push_uint(ctx, pins.uint);
	/* [ obj this uint ] */
	duk_put_prop_string(ctx, 1, DUX_IPK_PERIDOT_I2C_PINS);
	/* [ obj this ] */
	return 0; /* return this */
}

/*
 * Body of PeridotI2C.connect() / PeridotI2C.prototype.connect()
 */
DUK_LOCAL duk_ret_t peridot_i2c_connect_body(duk_context *ctx, peridot_i2c_pins_t *pins)
{
	/* [ con_constructor uint uint/undefined ] */
	extern peridot_i2c_master_state *const peridot_i2c_drivers[];
	peridot_i2ccon_data_t *data;
	duk_int_t slaveAddress;
	duk_int_t bitrate;
	int drv_idx;
	int result;

	// Get slave address
	slaveAddress = duk_require_int(ctx, 1);
	if ((slaveAddress < 0) || (slaveAddress > 127))
	{
		return DUK_RET_RANGE_ERROR;
	}

	// Get bitrate
	if (duk_is_null_or_undefined(ctx, 2))
	{
		bitrate = PERIDOT_I2C_DEFAULT_BITRATE;
	} else {
		bitrate = duk_require_int(ctx, 2);
	}

	duk_pop_2(ctx);
	/* [ con_constructor ] */

	data = (peridot_i2ccon_data_t *)duk_push_fixed_buffer(ctx, sizeof(*data));
	/* [ con_constructor buf ] */
	data->slaveAddress = slaveAddress;
	data->bitrate = bitrate;
	data->pins.uint = pins->uint;

	// Lookup driver
	for (drv_idx = 0;; ++drv_idx)
	{
		peridot_i2c_master_state *driver = peridot_i2c_drivers[drv_idx];
		if (!driver) {
			// No more drivers
			return duk_generic_error(ctx, "No I2C driver (SCL=%d, SDA=%d)", pins->scl, pins->sda);
		}

		if (peridot_i2c_master_configure_pins(driver, pins->scl, pins->sda, 1) == 0)
		{
			// Found
			data->driver = driver;
			break;
		}
	}

	result = peridot_i2c_master_get_clkdiv(data->driver, data->bitrate, &data->clkdiv);
	if (result != 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	duk_push_pointer(ctx, (void *)&peridot_i2ccon_functions);
	/* [ con_constructor buf ptr ] */
	duk_new(ctx, 2);
	/* [ instance ] */
	return 1;
}

/*
 * Entry of PeridotI2C.connect()
 */
DUK_LOCAL duk_ret_t peridot_i2c_connect(duk_context *ctx)
{
	peridot_i2c_pins_t pins;
	duk_ret_t result;

	/* [ obj uint uint/undefined ] */
	result = peridot_i2c_get_pins(ctx, 0, &pins);
	if (result != 0)
	{
		return result;
	}
	/* [ obj uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ obj uint uint/undefined constructor:3 ] */
	duk_get_prop_string(ctx, 3, DUX_IPK_PERIDOT_I2C_I2CCON);
	/* [ obj uint uint/undefined constructor:3 con_constructor:4 ] */
	duk_replace(ctx, 0);
	/* [ con_constructor uint uint/undefined constructor:3 ] */
	duk_pop(ctx);
	/* [ con_constructor uint uint/undefined ] */
	return peridot_i2c_connect_body(ctx, &pins);
}

/*
 * Entry of PeridotI2C.prototype.connect()
 */
DUK_LOCAL duk_ret_t peridot_i2c_proto_connect(duk_context *ctx)
{
	peridot_i2c_pins_t pins;

	/* [ uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ uint uint/undefined this ] */
	dux_push_constructor(ctx, 2);
	/* [ uint uint/undefined this constructor:3 ] */
	duk_get_prop_string(ctx, 3, DUX_IPK_PERIDOT_I2C_I2CCON);
	/* [ uint uint/undefined this constructor:3 con_constructor:4 ] */
	duk_insert(ctx, 0);
	/* [ con_constructor uint uint/undefined this:3 constructor:4 ] */
	duk_get_prop_string(ctx, 3, DUX_IPK_PERIDOT_I2C_PINS);
	/* [ con_constructor uint uint/undefined this:3 constructor:4 pins:5 ] */
	pins.uint = duk_require_uint(ctx, 5);
	duk_pop_3(ctx);
	/* [ con_constructor uint uint/undefined ] */
	return peridot_i2c_connect_body(ctx, &pins);
}

/*
 * Getter of PeridotI2C.prototype.pins
 */
DUK_LOCAL duk_ret_t peridot_i2c_proto_pins_getter(duk_context *ctx)
{
	peridot_i2c_pins_t pins;

	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	duk_get_prop_string(ctx, 0, DUX_IPK_PERIDOT_I2C_PINS);
	/* [ this uint ] */
	pins.uint = duk_require_uint(ctx, 1);
	duk_push_object(ctx);
	/* [ this uint obj ] */
	duk_push_uint(ctx, pins.scl);
	duk_put_prop_string(ctx, 2, "scl");
	duk_push_uint(ctx, pins.sda);
	duk_put_prop_string(ctx, 2, "sda");
	return 1; /* return obj */
}

/*
 * List of class methods
 */
DUK_LOCAL duk_function_list_entry peridot_i2c_funcs[] = {
	{ "connect", peridot_i2c_connect, 3 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype methods
 */
DUK_LOCAL duk_function_list_entry peridot_i2c_proto_funcs[] = {
	{ "connect", peridot_i2c_proto_connect, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL dux_property_list_entry peridot_i2c_proto_props[] = {
	{ "pins", peridot_i2c_proto_pins_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize PeridotI2C submodule
 */
DUK_INTERNAL duk_errcode_t dux_peridot_i2c_init(duk_context *ctx)
{
	/* [ require module exports ] */
	dux_push_named_c_constructor(
			ctx, "PeridotI2C", peridot_i2c_constructor, 1,
			peridot_i2c_funcs, peridot_i2c_proto_funcs,
			NULL, peridot_i2c_proto_props);
	/* [ require module exports constructor:3 ] */
	duk_dup(ctx, 0);
	duk_push_string(ctx, "hardware");
	duk_call(ctx, 1);
	/* [ require module exports constructor:3 hardware:4 ] */
	duk_get_prop_string(ctx, 4, "I2CConnection");
	/* [ require module exports constructor:3 hardware:4 I2CConnection:5 ] */
	duk_put_prop_string(ctx, 3, DUX_IPK_PERIDOT_I2C_I2CCON);
	/* [ require module exports constructor:3 hardware:4 ] */
	duk_pop(ctx);
	/* [ require module exports constructor:3 ] */
	duk_put_prop_string(ctx, 2, "I2C");
	/* [ require module exports ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */
#endif  /* DUX_USE_BOARD_PERIDOT */
