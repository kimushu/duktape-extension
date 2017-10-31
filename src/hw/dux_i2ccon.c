#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_I2CCON_DATA[]  = DUX_IPK("icD");
DUK_LOCAL const char DUX_IPK_I2CCON_FUNCS[] = DUX_IPK("icF");

/*
 * Entry of I2CConnection constructor
 */
DUK_LOCAL duk_ret_t i2ccon_constructor(duk_context *ctx)
{
	/* [ buf ptr(dux_i2ccon_functions) ] */
	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}
	duk_require_buffer(ctx, 0, NULL);
	duk_require_pointer(ctx, 1);

	duk_push_this(ctx);
	/* [ buf ptr this ] */
	duk_swap(ctx, 0, 2);
	/* [ this ptr buf ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_I2CCON_DATA);
	duk_put_prop_string(ctx, 0, DUX_IPK_I2CCON_FUNCS);
	return 0; /* return this */
}

DUK_LOCAL const dux_i2ccon_functions *i2ccon_proto_common(duk_context *ctx, void **pdata)
{
	duk_idx_t idx;
	const dux_i2ccon_functions *funcs;

	idx = duk_get_top(ctx);
	duk_push_this(ctx);
	duk_get_prop_string(ctx, idx, DUX_IPK_I2CCON_DATA);
	duk_get_prop_string(ctx, idx, DUX_IPK_I2CCON_FUNCS);
	*pdata = duk_get_buffer_data(ctx, idx + 1, NULL);
	funcs = (const dux_i2ccon_functions *)duk_get_pointer(ctx, idx + 2);
	duk_pop_3(ctx);
	return funcs;
}

/*
 * Entry of I2CConnection.prototype.read()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_read(duk_context *ctx)
{
	/* [ uint func ] */
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);

	duk_push_undefined(ctx);
	/* [ uint func undefined ] */
	duk_insert(ctx, 0);
	/* [ undefined uint func ] */
	return (*funcs->transfer)(ctx, data);
}

/*
 * Entry of I2CConnection.prototype.transfer()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_transfer(duk_context *ctx)
{
	/* [ obj uint func ] */
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);

	return (*funcs->transfer)(ctx, data);
}

/*
 * Entry of I2CConnection.prototype.write()
 */
DUK_LOCAL duk_ret_t i2ccon_proto_write(duk_context *ctx)
{
	/* [ obj func ] */
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);

	duk_push_uint(ctx, 0);
	/* [ obj func 0 ] */
	duk_swap(ctx, 1, 2);
	/* [ obj 0 func ] */
	return (*funcs->transfer)(ctx, data);
}

/*
 * Getter for I2CConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t i2ccon_proto_bitrate_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);
	return (*funcs->bitrate_getter)(ctx, data);
}

/*
 * Setter of I2CConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t i2ccon_proto_bitrate_setter(duk_context *ctx)
{
	/* [ uint ] */
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);
	return (*funcs->bitrate_setter)(ctx, data);
}

/*
 * Getter of I2CConnection.prototype.slaveAddress
 */
DUK_LOCAL duk_ret_t i2ccon_proto_slaveAddress_getter(duk_context *ctx)
{
	void *data;
	const dux_i2ccon_functions *funcs = i2ccon_proto_common(ctx, &data);
	return (*funcs->slaveAddress_getter)(ctx, data);
}

/*
 * List of prototype methods
 */
DUK_LOCAL const duk_function_list_entry i2ccon_proto_funcs[] = {
	{ "read", i2ccon_proto_read, 2 },
	{ "transfer", i2ccon_proto_transfer, 3 },
	{ "write", i2ccon_proto_write, 2 },
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
	/* [ ... Hardware ] */
	dux_push_named_c_constructor(
			ctx, "I2CConnection", i2ccon_constructor, 2,
			NULL, i2ccon_proto_funcs, NULL, i2ccon_proto_props);
	/* [ ... Hardware constructor ] */
	duk_put_prop_string(ctx, -2, "I2CConnection");
	/* [ ... Hardware ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */
