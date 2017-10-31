#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_SPICON_DATA[]  = DUX_IPK("scD");
DUK_LOCAL const char DUX_IPK_SPICON_FUNCS[] = DUX_IPK("scF");

#define SPICON_DEFAULT_FILLER   (0xff)

/*
 * Get filler value from stack
 */
DUK_LOCAL duk_uint_t spicon_get_filler(duk_context *ctx, duk_idx_t idx)
{
	/* [ ... val ... ] */
	if (duk_is_null_or_undefined(ctx, idx))
	{
		return SPICON_DEFAULT_FILLER;
	}
	return dux_require_int_range(ctx, idx, 0, 255);
}

/*
 * Entry of SPIConnection constructor
 */
DUK_LOCAL duk_ret_t spicon_constructor(duk_context *ctx)
{
	/* [ buf ptr(dux_spicon_functions) ] */
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
	duk_put_prop_string(ctx, 0, DUX_IPK_SPICON_DATA);
	duk_put_prop_string(ctx, 0, DUX_IPK_SPICON_FUNCS);
	return 0; /* return this */
}

DUK_LOCAL const dux_spicon_functions *spicon_proto_common(duk_context *ctx, void **pdata)
{
	duk_idx_t idx;
	const dux_spicon_functions *funcs;

	idx = duk_get_top(ctx);
	duk_push_this(ctx);
	duk_get_prop_string(ctx, idx, DUX_IPK_SPICON_DATA);
	duk_get_prop_string(ctx, idx, DUX_IPK_SPICON_FUNCS);
	*pdata = duk_get_buffer_data(ctx, idx + 1, NULL);
	funcs = (const dux_spicon_functions *)duk_get_pointer(ctx, idx + 2);
	duk_pop_3(ctx);
	return funcs;
}

/*
 * Entry of SPIConnection.prototype.exchange()
 */
DUK_LOCAL duk_ret_t spicon_proto_exchange(duk_context *ctx)
{
	/* [ obj(writeData) func ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	duk_push_uint(ctx, 0);
	/* [ obj(writeData) func uint(filler) ] */
	duk_push_true(ctx);
	/* [ obj(writeData) func uint(filler) true(readLen):3 ] */
	duk_swap(ctx, 1, 3);
	/* [ obj(writeData) true(readLen) uint(filler) func:3 ] */
	return (*funcs->transferRaw)(ctx, data);
}

/*
 * Entry of SPIConnection.prototype.read()
 */
DUK_LOCAL duk_ret_t spicon_proto_read(duk_context *ctx)
{
	/* [ uint(readLen) int(filler) func ]  (with filler) */
	/* [ uint(readLen) func undefined ] (without filler) */
	duk_uint_t filler;
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);

	duk_require_uint(ctx, 0);

	if (duk_is_callable(ctx, 1))
	{
		// Without filler
		filler = SPICON_DEFAULT_FILLER;
		duk_pop(ctx);
		duk_push_int(ctx, filler);
		duk_swap(ctx, 1, 2);
	}
	else
	{
		// With filler
		filler = spicon_get_filler(ctx, 1);
		/* [ uint(readLen) int(filler) func ] */
		duk_push_int(ctx, filler);
		duk_replace(ctx, 1);
	}
	/* [ uint(readLen) int(filler) func ] */
	duk_push_undefined(ctx);
	duk_insert(ctx, 0);
	/* [ undefined uint(readLen) int(filler) func ] */
	return (*funcs->transferRaw)(ctx, data);
}

/*
 * Entry of SPIConnection.prototype.transfer()
 */
DUK_LOCAL duk_ret_t spicon_proto_transfer(duk_context *ctx)
{
	/* [ obj(writeData) uint(readLen) int(filler) func:3 ]  (with filler) */
	/* [ obj(writeData) uint(readLen) func undefined:3 ] (without filler) */
	duk_uint_t filler;
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);

	duk_require_uint(ctx, 1);

	if (duk_is_callable(ctx, 2))
	{
		// Without filler
		filler = SPICON_DEFAULT_FILLER;
		duk_pop(ctx);
		duk_push_uint(ctx, filler);
		/* [ obj(writeData) uint(readLen) ufunc int(filler):3 ] */
		duk_swap(ctx, 2, 3);
	}
	else
	{
		// With filler
		filler = spicon_get_filler(ctx, 2);
		/* [ obj(writeData) uint(readLen) uint(filler) func:3 ] */
		duk_push_uint(ctx, filler);
		duk_replace(ctx, 2);
	}
	/* [ obj(writeData) uint(readLen) uint(filler) func:3 ] */
	return (*funcs->transferRaw)(ctx, data);
}

/*
 * Entry of SPIConnection.prototype.write()
 */
DUK_LOCAL duk_ret_t spicon_proto_write(duk_context *ctx)
{
	/* [ obj(writeData) func ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	duk_push_uint(ctx, 0);
	/* [ obj(writeData) func uint(filler) ] */
	duk_push_uint(ctx, 0);
	/* [ obj(writeData) func uint(filler) uint(readLen):3 ] */
	duk_swap(ctx, 1, 3);
	/* [ obj(writeData) uint(readLen) uint(filler) func:3 ] */
	return (*funcs->transferRaw)(ctx, data);
}

/*
 * Getter for SPIConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t spicon_proto_bitrate_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->bitrate_getter)(ctx, data);
}

/*
 * Setter of SPIConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t spicon_proto_bitrate_setter(duk_context *ctx)
{
	/* [ uint ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->bitrate_setter)(ctx, data);
}

/*
 * Getter of SPIConnection.prototype.lsbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_lsbFirst_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->lsbFirst_getter)(ctx, data);
}

/*
 * Setter of SPIConnection.prototype.lsbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_lsbFirst_setter(duk_context *ctx)
{
	/* [ boolean ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->lsbFirst_setter)(ctx, data);
}

/*
 * Getter of SPIConnection.prototype.mode
 */
DUK_LOCAL duk_ret_t spicon_proto_mode_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->mode_getter)(ctx, data);
}

/*
 * Setter of SPIConnection.prototype.mode
 */
DUK_LOCAL duk_ret_t spicon_proto_mode_setter(duk_context *ctx)
{
	/* [ uint ] */
	void *data;
	dux_require_int_range(ctx, 0, 0, 3);
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->mode_setter)(ctx, data);
}

/*
 * Getter of SPIConnection.prototype.msbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_msbFirst_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	duk_ret_t result;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	result = (*funcs->lsbFirst_getter)(ctx, data);
	if (result == 1)
	{
		duk_push_boolean(ctx, !duk_require_boolean(ctx, -1));
	}
	return result;
}

/*
 * Setter of SPIConnection.prototype.msbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_msbFirst_setter(duk_context *ctx)
{
	/* [ boolean ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	duk_push_boolean(ctx, !duk_require_boolean(ctx, 0));
	duk_replace(ctx, 0);
	return (*funcs->lsbFirst_setter)(ctx, data);
}

/*
 * Getter of SPIConnection.prototype.slaveSelect
 */
DUK_LOCAL duk_ret_t spicon_proto_slaveSelect_getter(duk_context *ctx)
{
	/* [  ] */
	void *data;
	const dux_spicon_functions *funcs = spicon_proto_common(ctx, &data);
	return (*funcs->slaveSelect_getter)(ctx, data);
}

/*
 * List of prototype methods
 */
DUK_LOCAL const duk_function_list_entry spicon_proto_funcs[] = {
	{ "exchange", spicon_proto_exchange, 2 },
	{ "read", spicon_proto_read, 3 },
	{ "transfer", spicon_proto_transfer, 4 },
	{ "write", spicon_proto_write, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL const dux_property_list_entry spicon_proto_props[] = {
	{ "bitrate", spicon_proto_bitrate_getter, spicon_proto_bitrate_setter },
	{ "lsbFirst", spicon_proto_lsbFirst_getter, spicon_proto_lsbFirst_setter },
	{ "mode", spicon_proto_mode_getter, spicon_proto_mode_setter },
	{ "msbFirst", spicon_proto_msbFirst_getter, spicon_proto_msbFirst_setter },
	{ "slaveSelect", spicon_proto_slaveSelect_getter, NULL },
	{ NULL, NULL, NULL },
};

/*
 * Initialize SPIConnection class
 */
DUK_INTERNAL duk_errcode_t dux_spicon_init(duk_context *ctx)
{
	/* [ ... Hardware ] */
	dux_push_named_c_constructor(
			ctx, "SPIConnection", spicon_constructor, 2,
			NULL, spicon_proto_funcs, NULL, spicon_proto_props);
	/* [ ... Hardware constructor ] */
	duk_put_prop_string(ctx, -2, "SPIConnection");
	/* [ ... Hardware ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */
