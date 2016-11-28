/*
 * ECMA classes:
 *    new I2CConnection(
 *      <PlainBuffer> data
 *    );
 *
 * ECMA methods:
 *    I2CConnection.prototype.read(readlen, callback)
 *    I2CConnection.prototype.write(writedata, callback)
 *    I2CConnection.prototype.writeAndRead(writedata, readlen, callback)
 *
 * ECMA properties:
 *    I2CConnection.prototype.bitrate
 *    I2CConnection.prototype.slaveAddress
 *
 * Native functions:
 *    void dux_push_i2ccon_constructor(duk_context *ctx);
 */
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_I2CCON[]      = DUX_IPK("I2CCon");
DUK_LOCAL const char DUX_IPK_I2CCON_DATA[] = DUX_IPK("icData");

/*
 * Get data pointer
 */
DUK_LOCAL dux_i2ccon_data *i2ccon_get_data(duk_context *ctx)
{
	dux_i2ccon_data *data;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_IPK_I2CCON_DATA);
	data = (dux_i2ccon_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	return data;
}

/*
 * Entry of I2CConnection constructor
 */
DUK_LOCAL duk_ret_t i2ccon_constructor(duk_context *ctx)
{
	/* [ buf ] */

	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

	duk_require_buffer(ctx, 0, NULL);

	duk_push_this(ctx);
	/* [ buf this ] */
	duk_swap(ctx, 0, 1);
	/* [ this buf ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_I2CCON_DATA);
	/* [ this ] */
	return 0; /* return this */
}

/*
 * Common implementation of I2CConnection read/write functions
 */
DUK_LOCAL duk_ret_t i2ccon_proto_common(duk_context *ctx,
		duk_idx_t widx, duk_idx_t ridx, duk_idx_t cidx)
{
	/* [ ... writedata ... readlen ... callback ... ] */
	/*       ^widx >=0     ^ridx >=0   ^cidx          */
	dux_i2ccon_data *data = i2ccon_get_data(ctx);
	duk_uint_t readlen;

	readlen = 0;
	if (ridx >= 0)
	{
		readlen = duk_require_uint(ctx, ridx);
	}

	if (widx >= 0)
	{
		if (duk_is_array(ctx, widx))
		{
			// Array => Convert to fixed Buffer
			duk_size_t size;
			duk_uarridx_t aidx;
			unsigned char *buffer;

			size = duk_get_length(ctx, widx);
			duk_push_fixed_buffer(ctx, size);
			buffer = (unsigned char *)duk_get_buffer(ctx, -1, NULL);

			for (aidx = 0; aidx < size; ++aidx)
			{
				duk_uint_t byte;
				duk_get_prop_index(ctx, widx, aidx);
				byte = duk_require_uint(ctx, -1);
				duk_pop(ctx);
				if (byte > 0xff)
				{
					return DUK_RET_TYPE_ERROR;
				}
				*buffer++ = (unsigned char)byte;
			}
		}
		else if (duk_is_string(ctx, widx) || duk_is_buffer(ctx, widx))
		{
			// string, Duktape.Buffer, Node.js Buffer, ArrayBuffer,
			// DataView, TypedArray
			// => No conversion needed
			duk_dup(ctx, widx);
		}
		else
		{
			return DUK_RET_TYPE_ERROR;
		}
	}
	else
	{
		duk_push_undefined(ctx);
	}

	if (duk_is_null_or_undefined(ctx, cidx))
	{
		// TODO: Promise support
		duk_push_undefined(ctx);
	}
	else
	{
		duk_require_function(ctx, cidx);
		duk_dup(ctx, cidx);
	}

	/* [ ... writedata func ] */
	return (*data->transfer)(ctx, data, readlen);
}

/*
 * Entry of I2CConnection.prototype.read()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_read(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, -1, 0, 1);
}

/*
 * Entry of I2CConnection.prototype.writeAndRead()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_writeAndRead(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, 0, 1, 2);
}

/*
 * Entry of I2CConnection.prototype.write()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_write(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, 0, -1, 1);
}

/*
 * Getter for I2CConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t i2ccon_proto_bitrate_getter(duk_context *ctx)
{
	dux_i2ccon_data *data = i2ccon_get_data(ctx);

	duk_push_uint(ctx, data->bitrate);
	/* [ uint ] */
	return 1; /* return uint */
}

/*
 * Setter of I2CConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t i2ccon_proto_bitrate_setter(duk_context *ctx)
{
	dux_i2ccon_data *data = i2ccon_get_data(ctx);
	duk_uint_t old_bitrate;
	duk_int_t new_bitrate;
	duk_ret_t result;

	/* [ uint ] */
	old_bitrate = data->bitrate;
	new_bitrate = duk_require_int(ctx, 0);
	if (new_bitrate <= 0)
	{
		return DUK_RET_RANGE_ERROR;
	}
	data->bitrate = new_bitrate;
	result = (*data->update_bitrate)(ctx, data);
	if (result != 0)
	{
		data->bitrate = old_bitrate;
		return result;
	}
	return 0; /* return undefined */
}

/*
 * Getter of I2CConnection.prototype.slaveAddress
 */
DUK_LOCAL duk_ret_t i2ccon_proto_slaveAddress_getter(duk_context *ctx)
{
	dux_i2ccon_data *data = i2ccon_get_data(ctx);
	duk_push_uint(ctx, data->slaveAddress);
	/* [ uint ] */
	return 1; /* return uint */
}

/*
 * List of prototype methods
 */
DUK_LOCAL const duk_function_list_entry i2ccon_proto_funcs[] = {
	{ "read", i2ccon_proto_read, 2 },
	{ "write", i2ccon_proto_write, 2 },
	{ "writeAndRead", i2ccon_proto_writeAndRead, 3 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL const dux_property_list_entry i2ccon_proto_props[] = {
	{ "bitrate", i2ccon_proto_bitrate_getter, i2ccon_proto_bitrate_setter },
	{ "slaveAddress", i2ccon_proto_slaveAddress_getter, NULL },
	{ NULL, NULL, NULL },
};

/*
 * Initialize I2CConnection class
 */
DUK_INTERNAL duk_errcode_t dux_i2ccon_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	dux_push_named_c_constructor(
			ctx, "I2CConnection", i2ccon_constructor, 1,
			NULL, i2ccon_proto_funcs, NULL, i2ccon_proto_props);
	/* [ ... stash constructor ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_I2CCON);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Push I2CConnection constructor (for board dependent implementations)
 */
DUK_INTERNAL void dux_push_i2ccon_constructor(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_I2CCON);
	/* [ ... stash constructor ] */
	duk_remove(ctx, -2);
	/* [ ... constructor ] */
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */
