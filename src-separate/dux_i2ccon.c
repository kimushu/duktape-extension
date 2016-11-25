#include "dux_i2ccon.h"
#include "dux_common.h"

static const char *const DUX_I2CCON_CLASS = "\xff" "dux_i2ccon.class";
static const char *const DUX_I2CCON_BUF = "\xff" "dux_i2ccon.buf";

/*
 * C function entry of I2CConnection.constructor
 * (PlainBuffer data)
 */
static duk_ret_t i2ccon_constructor(duk_context *ctx)
{
	/* [ buf ] */

	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

	duk_push_this(ctx);
	duk_insert(ctx, 0);
	/* [ i2ccon buf ] */
	duk_require_buffer(ctx, 1, NULL);
	duk_put_prop_string(ctx, 0, DUX_I2CCON_BUF);
	return 0;
}

/*
 * Native implementation of I2CConnection read/write functions
 */
static duk_ret_t i2ccon_proto_common(duk_context *ctx,
		duk_idx_t widx, duk_idx_t ridx, duk_idx_t cidx)
{
	/* [ ... writedata ... readlen ... callback ... ] */
	/*       ^widx >=0     ^ridx >=0   ^cidx >=0      */
	dux_i2ccon_t *data;
	duk_uint_t readlen;

	duk_push_this(ctx);
	/* [ ... this ] */
	duk_get_prop_string(ctx, -1, DUX_I2CCON_BUF);
	data = (dux_i2ccon_t *)duk_require_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	/* [ ... ] */

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
 * C function entry of I2CConnection.prototype.read
 * (<uint> readlen, <function/null> callback)
 */
static duk_ret_t i2ccon_proto_read(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, -1, 0, 1);
}

/*
 * C function entry of I2CConnection.prototype.transfer
 * (<obj> writedata, <uint> readlen, <function/null> callback)
 */
static duk_ret_t i2ccon_proto_transfer(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, 0, 1, 2);
}

/*
 * C function entry of I2CConnection.prototype.write
 * (<obj> writedata, <function/null> callback)
 */
static duk_ret_t i2ccon_proto_write(duk_context *ctx)
{
	return i2ccon_proto_common(ctx, 0, -1, 1);
}

/*
 * C function entry of getter for I2CConnection.prototype.bitrate
 * ()
 */
static duk_ret_t i2ccon_proto_bitrate_getter(duk_context *ctx)
{
	dux_i2ccon_t *data;

	duk_push_this(ctx);
	/* [ ... this ] */
	duk_get_prop_string(ctx, -1, DUX_I2CCON_BUF);
	/* [ ... this buf ] */
	data = (dux_i2ccon_t *)duk_require_buffer(ctx, -1, NULL);
	duk_push_uint(ctx, data->bitrate);
	/* [ ... this buf uint ] */
	return 1;	/* return uint; */
}

/*
 * C function entry of setter for I2CConnection.prototype.bitrate
 * (<uint> bitrate)
 */
static duk_ret_t i2ccon_proto_bitrate_setter(duk_context *ctx)
{
	dux_i2ccon_t *data;
	duk_uint_t old_bitrate;
	duk_int_t new_bitrate;
	duk_ret_t result;

	duk_push_this(ctx);
	/* [ ... this ] */
	duk_get_prop_string(ctx, -1, DUX_I2CCON_BUF);
	/* [ ... this buf ] */
	data = (dux_i2ccon_t *)duk_require_buffer(ctx, -1, NULL);
	old_bitrate = data->bitrate;
	new_bitrate = duk_require_int(ctx, 0);
	if (new_bitrate < 0)
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
	return 0;	/* return undefined; */
}

/*
 * C function entry of getter for I2CConnection.prototype.slaveAddress
 * ()
 */
static duk_ret_t i2ccon_proto_slaveAddress_getter(duk_context *ctx)
{
	dux_i2ccon_t *data;

	duk_push_this(ctx);
	/* [ ... this ] */
	duk_get_prop_string(ctx, -1, DUX_I2CCON_BUF);
	/* [ ... this buf ] */
	data = (dux_i2ccon_t *)duk_require_buffer(ctx, -1, NULL);
	duk_push_uint(ctx, data->slaveAddress);
	/* [ ... this buf uint ] */
	return 1;	/* return uint; */
}

static const duk_function_list_entry i2ccon_proto_funcs[] = {
	{ "read", i2ccon_proto_read, 2 },
	{ "transfer", i2ccon_proto_transfer, 3 },
	{ "write", i2ccon_proto_write, 2 },
	{ NULL, NULL, 0 }
};

static const dux_property_list_entry i2ccon_proto_props[] = {
	{ "bitrate", i2ccon_proto_bitrate_getter, i2ccon_proto_bitrate_setter },
	{ "slaveAddress", i2ccon_proto_slaveAddress_getter, NULL },
	{ NULL, NULL, NULL },
};

/*
 * Initialize I2CConnection class
 */
void dux_i2ccon_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	dux_push_named_c_constructor(
			ctx, "I2CConnection", i2ccon_constructor, 1,
			NULL, i2ccon_proto_funcs, NULL, i2ccon_proto_props);
	/* [ ... stash constructor ] */
	duk_put_prop_string(ctx, -2, DUX_I2CCON_CLASS);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
}
/*
 * Push I2CConnection class (for board dependent implementations)
 */
void dux_push_i2ccon_class(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_I2CCON_CLASS);
	/* [ ... stash constructor ] */
	duk_remove(ctx, -2);
	/* [ ... constructor ] */
}

