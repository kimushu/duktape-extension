/*
 * ECMA objects:
 *    class SPIConnection {
 *      constructor(<PlainBuffer> data) {
 *      }
 *
 *      exchange(<ArrayBuffer/string> writedata,
 *               <Function> callback) {
 *        return;
 *      }
 *
 *      read(<uint> read_len [,<Function> callback [,<uint> filler]]) {
 *        return;
 *      }
 *
 *      transfer(<ArrayBuffer/string> writedata,
 *               <uint> read_len [,
 *               <Function> callback [,
 *               <uint> filler]]) {
 *        return;
 *      }
 *
 *      write(<ArrayBuffer/string> writedata,
 *            <Function> callback) {
 *        return;
 *      }
 *
 *      get bitrate()       { return <uint>; }
 *      set bitrate(<uint> value) {}
 *
 *      get lsbFirst()      { return <bool>; }
 *      set lsbFirst(<bool> value) {}
 *
 *      get mode()          { return <uint>; }
 *      set mode(<uint> value) {}
 *
 *      get msbFirst()      { return <bool>; }
 *      set msbFirst(<bool> value) {}
 *
 *      get slaveSelect()   { return <obj>; }
 *    }
 *
 * Native functions:
 *    void dux_push_spicon_constructor(duk_context *ctx);
 */
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_SPICON[]      = DUX_IPK("SPICon");
DUK_LOCAL const char DUX_IPK_SPICON_DATA[] = DUX_IPK("scData");
DUK_LOCAL const char DUX_IPK_SPICON_AUX[]  = DUX_IPK("scAux");

#define SPICON_DEFAULT_FILLER   (0xff)

/*
 * Get data pointer
 */
DUK_LOCAL dux_spicon_data *spicon_get_data(duk_context *ctx)
{
	dux_spicon_data *data;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_IPK_SPICON_DATA);
	data = (dux_spicon_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	return data;
}

/*
 * Get filler value from stack (stack value will be removed)
 */
DUK_LOCAL duk_uint_t spicon_get_filler(duk_context *ctx, duk_idx_t index)
{
	duk_int_t val;

	/* [ ... val ... ] */
	if (duk_is_null_or_undefined(ctx, index))
	{
		val = SPICON_DEFAULT_FILLER;
	}
	else
	{
		val = dux_require_int_range(ctx, index, 0, 255);
	}
	duk_remove(ctx, index);
	return (duk_uint_t)val;
}

/*
 * Entry of SPIConnection constructor
 */
DUK_LOCAL duk_ret_t spicon_constructor(duk_context *ctx)
{
	dux_spicon_data *data;
	duk_ret_t result;

	/* [ buf obj ] */
	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

	data = (dux_spicon_data *)duk_require_buffer(ctx, 0, NULL);

	duk_push_this(ctx);
	/* [ buf obj this ] */
	duk_swap(ctx, 0, 2);
	/* [ this obj buf ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_SPICON_DATA);
	/* [ this obj ] */
	duk_dup(ctx, 1);
	/* [ this obj obj ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_SPICON_AUX);
	/* [ this obj ] */
	duk_push_uint(ctx, data->bitrate);
	/* [ this obj uint ] */
	duk_insert(ctx, 0);
	/* [ uint this obj ] */
	result = (*data->update_bitrate)(ctx, data);
	if (result != 0)
	{
		return result; /* return error */
	}
	/* [ this this obj ] */
	return 0; /* return this */
}

/*
 * Common implementation of SPIConnection read/write functions
 */
DUK_LOCAL duk_ret_t spicon_proto_transfer_body(duk_context *ctx,
                                               duk_uint_t read_skip,
                                               duk_uint_t read_len,
                                               duk_uint_t filler)
{
	/* [ buf/undefined func/undefined ] */

	dux_spicon_data *data = spicon_get_data(ctx);

	dux_promise_get_cb_with_bool(ctx, 1);
	/* [ buf/undefined func promise/undefined ] */
	duk_swap(ctx, 0, 2);
	/* [ promise/undefined func buf/undefined ] */

	if (read_len > 0)
	{
		duk_push_fixed_buffer(ctx, read_len);
	}
	else
	{
		duk_push_undefined(ctx);
	}
	/* [ promise/undefined func buf/undefined buf/undefined:3 ] */

	duk_push_this(ctx);
	duk_get_prop_string(ctx, 4, DUX_IPK_SPICON_AUX);
	/* [ promise/undefined func buf/undefined buf/undefined:3 this:4 obj(aux):5 ] */

	(*data->transfer)(ctx, data, read_skip, filler);

	duk_set_top(ctx, 1);
	/* [ promise/undefined ] */
	return 1; /* return promise or undefined; */
}

/*
 * Entry of SPIConnection.prototype.exchange()
 */
DUK_LOCAL duk_ret_t spicon_proto_exchange(duk_context *ctx)
{
	duk_size_t len;

	/* [ obj func/undefined ] */
	dux_to_byte_buffer(ctx, 0, &len);
	/* [ buf func/undefined ] */

	return spicon_proto_transfer_body(ctx, 0, len, 0);
}

/*
 * Entry of SPIConnection.prototype.read()
 */
DUK_LOCAL duk_ret_t spicon_proto_read(duk_context *ctx)
{
	duk_uint_t read_len;
	duk_uint_t filler;

	/* [ uint func/undefined uint/undefined ] */
	filler = spicon_get_filler(ctx, 2);
	/* [ uint func/undefined ] */
	read_len = duk_require_uint(ctx, 0);
	duk_push_undefined(ctx);
	duk_replace(ctx, 0);
	/* [ undefined func/undefined ] */

	return spicon_proto_transfer_body(ctx, 0, read_len, filler);
}

/*
 * Entry of SPIConnection.prototype.transfer()
 */
DUK_LOCAL duk_ret_t spicon_proto_transfer(duk_context *ctx)
{
	duk_uint_t write_len;
	duk_uint_t read_len;
	duk_uint_t filler;

	/* [ obj uint func/undefined uint/undefined ] */
	filler = spicon_get_filler(ctx, 3);
	/* [ obj uint func/undefined ] */
	read_len = duk_require_uint(ctx, 1);
	duk_remove(ctx, 1);
	/* [ obj func/undefined ] */
	dux_to_byte_buffer(ctx, 0, &write_len);
	/* [ buf func/undefined ] */

	return spicon_proto_transfer_body(ctx, write_len, read_len, filler);
}

/*
 * Entry of SPIConnection.prototype.write()
 */
DUK_LOCAL duk_ret_t spicon_proto_write(duk_context *ctx)
{
	/* [ obj func/undefined ] */
	dux_to_byte_buffer(ctx, 0, NULL);
	/* [ buf func/undefined ] */

	return spicon_proto_transfer_body(ctx, 0, 0, 0);
}

/*
 * Getter for SPIConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t spicon_proto_bitrate_getter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);

	duk_push_uint(ctx, data->bitrate);
	/* [ uint ] */
	return 1; /* return uint */
}

/*
 * Setter of SPIConnection.prototype.bitrate
 */
DUK_LOCAL duk_ret_t spicon_proto_bitrate_setter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
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
	duk_push_this(ctx);
	/* [ uint this ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_SPICON_AUX);
	/* [ uint this obj ] */
	result = (*data->update_bitrate)(ctx, data);
	if (result != 0)
	{
		data->bitrate = old_bitrate;
		return result;
	}
	return 0; /* return undefined */
}

/*
 * Getter of SPIConnection.prototype.lsbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_lsbFirst_getter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	duk_push_boolean(ctx, data->lsbFirst);
	return 1; /* return bool; */
}

/*
 * Setter of SPIConnection.prototype.lsbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_lsbFirst_setter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	/* [ bool ] */
	data->lsbFirst = duk_require_boolean(ctx, 0);
	return 0; /* return undefined; */
}

/*
 * Getter of SPIConnection.prototype.mode
 */
DUK_LOCAL duk_ret_t spicon_proto_mode_getter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	duk_push_uint(ctx, data->mode);
	return 1; /* return uint; */
}

/*
 * Setter of SPIConnection.prototype.mode
 */
DUK_LOCAL duk_ret_t spicon_proto_mode_setter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);

	/* [ uint ] */
	data->mode = dux_require_int_range(ctx, 0, 0, 3);
	return 0; /* return undefined; */
}

/*
 * Getter of SPIConnection.prototype.msbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_msbFirst_getter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	duk_push_boolean(ctx, !data->lsbFirst);
	return 1; /* return bool; */
}

/*
 * Setter of SPIConnection.prototype.msbFirst
 */
DUK_LOCAL duk_ret_t spicon_proto_msbFirst_setter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	/* [ bool ] */
	data->lsbFirst = !duk_require_boolean(ctx, 0);
	return 0; /* return undefined; */
}

/*
 * Getter of SPIConnection.prototype.slaveSelect
 */
DUK_LOCAL duk_ret_t spicon_proto_slaveSelect_getter(duk_context *ctx)
{
	dux_spicon_data *data = spicon_get_data(ctx);
	return (*data->get_slaveSelect)(ctx, data);
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
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	dux_push_named_c_constructor(
			ctx, "SPIConnection", spicon_constructor, 2,
			NULL, spicon_proto_funcs, NULL, spicon_proto_props);
	/* [ ... stash constructor ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_SPICON);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Push SPIConnection constructor (for board dependent implementations)
 */
DUK_INTERNAL void dux_push_spicon_constructor(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_SPICON);
	/* [ ... stash constructor ] */
	duk_remove(ctx, -2);
	/* [ ... constructor ] */
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */
