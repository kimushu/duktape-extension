/*
 * ECMA objects:
 *    class ParallelIO {
 *      // With accessor functions
 *      constructor(<uint> width, <uint> offset, <uint> polarity,
 *                  <pointer> manip, <pointer> param) {
 *      }
 *
 *      // Output functions (Chainable)
 *      assert()           { return this; }
 *      clear()            { return this.low(); }
 *      high()             { return this; }
 *      low()              { return this; }
 *      negate()           { return this; }
 *      off()              { return this.negate(); }
 *      on()               { return this.assert(); }
 *      set()              { return this.high(); }
 *      toggle()           { return this; }
 *
 *      // Direction functions (Chainable)
 *      disableInput()     { return this; }
 *      disableOutput()    { return this; }
 *      enableInput()      { return this; }
 *      enableOutput()     { return this; }
 *
 *      // Polarity functions (Chainable)
 *      setActiveHigh()    { return this; }
 *      setActiveLow()     { return this; }
 *
 *      // Lock functions (Chainable)
 *      lock()             { return this; }
 *      unlock()           { return this; }
 *
 *      // Other functions
 *      slice(<uint> begin, <uint> end = LAST) {
 *        return <ParallelIO>;
 *      }
 *
 *      // Pin value properties
 *      get isAsserted()   { return <bool>; }
 *      get isCleared()    { return this.isLow; }
 *      get isHigh()       { return <bool>; }
 *      get isLow()        { return <bool>; }
 *      get isNegated()    { return <bool>; }
 *      get isOff()        { return this.isNegated; }
 *      get isOn()         { return this.isAsserted; }
 *      get isSet()        { return this.isHigh; }
 *      get value()        { return <uint>; }
 *      set value(<uint> val) {}
 *
 *      // Direction properties
 *      get canInput()     { return <bool>; }
 *      get canOutput()    { return <bool>; }
 *
 *      // Polarity properties
 *      get isActiveHigh() { return <bool>; }
 *      get isActiveLow()  { return <bool>; }
 *
 *      // Lock properties
 *      get isLocked()     { return <bool>; }
 *
 *      // Other properties
 *      get width()        { return <uint>; }
 *    );
 *    global.ParallelIO = ParallelIO;
 *
 * Internal data structure:
 *    (new ParallelIO).[[DUX_IPK_PARAIO_DATA]] = <PlainBuffer> obj;
 *    (new ParallelIO).[[DUX_IPK_PARAIO_LINK]] = <PlainBuffer> obj;
 */
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_PARALLELIO)
#include "../dux_internal.h"

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_PARAIO_DATA[] = DUX_IPK("piData");
DUK_LOCAL const char DUX_IPK_PARAIO_LINK[] = DUX_IPK("piLink");

/*
 * Structures
 */

struct dux_paraio_data;
typedef struct dux_paraio_data
{
	struct dux_paraio_data *link;
	duk_int_t width;        /* 1-32 */
	duk_int_t offset;       /* 0-31 */
	duk_uint_t bits;        /* ((1<<width)-1)<<offset */

	/* Linked */
	const dux_paraio_manip *manip;
	void *param;
	duk_uint_t cfg_in;
	duk_uint_t cfg_out;
	duk_uint_t cfg_pol;     /* 0=ActiveHigh,1=ActiveLow */
	duk_uint_t cfg_lock;
}
dux_paraio_data;

/*
 * Get data pointer
 */
DUK_LOCAL dux_paraio_data *paraio_push_this_and_get_data(duk_context *ctx)
{
	dux_paraio_data *data;

	/* [ ... ] */
	duk_push_this(ctx);
	/* [ ... this ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_PARAIO_DATA);
	/* [ ... this buf ] */
	data = (dux_paraio_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop(ctx);
	/* [ ... this ] */
	return data;
}

/*
 * Constructor of ParallelIO class
 */
DUK_LOCAL duk_ret_t paraio_constructor(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data;

	/* [ uint uint uint pointer pointer:4 ] */
	duk_push_this(ctx);
	duk_push_fixed_buffer(ctx, sizeof(dux_paraio_data));
	/* [ uint uint uint pointer pointer:4 this:5 buf:6 ] */
	data = (dux_paraio_data *)duk_require_buffer(ctx, 6, NULL);

	data->width = duk_require_uint(ctx, 0);
	if ((data->width == 0) || (data->width > 32))
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->offset = duk_require_uint(ctx, 1);
	if ((data->offset >= 32) || ((data->offset + data->width) > 32))
	{
		return DUK_RET_RANGE_ERROR;
	}

	data->link = data;
	data->bits = ((1 << data->width) - 1) << data->offset;
	data->manip = (const dux_paraio_manip *)duk_require_pointer(ctx, 3);
	data->param = (void *)duk_require_pointer(ctx, 4);
	data->cfg_pol = duk_require_uint(ctx, 2);
	data->cfg_lock = 0;

	result = (*data->manip->read_config)(ctx, data->param, data->bits,
				&data->cfg_in, &data->cfg_out);
	if (result != 0)
	{
		return result;
	}

	duk_put_prop_string(ctx, 5, DUX_IPK_PARAIO_DATA);
	/* [ ... this:5 ] */

	return 0; /* return this */
}

/*
 * Entry of ParallelIO.prototype.assert()
 */
DUK_LOCAL duk_ret_t paraio_proto_assert(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->write_output)(ctx, data->param,
				~data->cfg_pol & bits, data->cfg_pol & bits, 0);
	if (result != 0)
	{
		return result;
	}
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.low()
 */
DUK_LOCAL duk_ret_t paraio_proto_low(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->write_output)(ctx, data->param,
				0, bits, 0);
	if (result != 0)
	{
		return result;
	}
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.high()
 */
DUK_LOCAL duk_ret_t paraio_proto_high(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->write_output)(ctx, data->param,
				bits, 0, 0);
	if (result != 0)
	{
		return result;
	}
	return 1; /* return this */

}

/*
 * Entry of ParallelIO.prototype.negate()
 */
DUK_LOCAL duk_ret_t paraio_proto_negate(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->write_output)(ctx, data->param,
				data->cfg_pol & bits, ~data->cfg_pol & bits, 0);
	if (result != 0)
	{
		return result;
	}
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.toggle()
 */
DUK_LOCAL duk_ret_t paraio_proto_toggle(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->write_output)(ctx, data->param,
				0, 0, bits);
	if (result != 0)
	{
		return result;
	}
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.disableInput()
 */
DUK_LOCAL duk_ret_t paraio_proto_disableInput(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if (data->link->cfg_lock & bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!data->manip->config_input)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->config_input)(ctx, data->param,
				bits, 0);
	if (result != 0)
	{
		return result;
	}
	data->link->cfg_in &= ~bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.disableOutput()
 */
DUK_LOCAL duk_ret_t paraio_proto_disableOutput(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if (data->link->cfg_lock & bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!data->manip->config_output)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->config_output)(ctx, data->param,
				bits, 0);
	if (result != 0)
	{
		return result;
	}
	data->link->cfg_out &= ~bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.enableInput()
 */
DUK_LOCAL duk_ret_t paraio_proto_enableInput(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if (data->link->cfg_lock & bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!data->manip->config_input)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->config_input)(ctx, data->param,
				bits, bits);
	if (result != 0)
	{
		return result;
	}
	data->link->cfg_in |= bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.enableOutput()
 */
DUK_LOCAL duk_ret_t paraio_proto_enableOutput(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;

	/* [ this ] */
	if (data->link->cfg_lock & bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!data->manip->config_output)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->config_output)(ctx, data->param,
				bits, bits);
	if (result != 0)
	{
		return result;
	}
	data->link->cfg_out |= bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.setActiveHigh()
 */
DUK_LOCAL duk_ret_t paraio_proto_setActiveHigh(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);

	/* [ this ] */
	if (data->link->cfg_lock & data->bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	data->link->cfg_pol &= ~data->bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.setActiveLow()
 */
DUK_LOCAL duk_ret_t paraio_proto_setActiveLow(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);

	/* [ this ] */
	if (data->link->cfg_lock & data->bits)
	{
		return DUK_RET_TYPE_ERROR;
	}
	data->link->cfg_pol |= data->bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.lock()
 */
DUK_LOCAL duk_ret_t paraio_proto_lock(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);

	/* [ this ] */
	data->link->cfg_lock |= data->bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.unlock()
 */
DUK_LOCAL duk_ret_t paraio_proto_unlock(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);

	/* [ this ] */
	data->link->cfg_lock &= ~data->bits;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.slice()
 */
DUK_LOCAL duk_ret_t paraio_proto_slice(duk_context *ctx)
{
	dux_paraio_data *data;
	dux_paraio_data *new_data;
	duk_int_t begin, end;
	duk_ret_t result;

	/* [ uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ uint uint/undefined this ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PARAIO_DATA);
	/* [ uint uint/undefined this buf ] */
	data = (dux_paraio_data *)duk_require_buffer(ctx, 3, NULL);

	begin = duk_require_int(ctx, 0);
	if (begin < 0)
	{
		begin += data->width;
	}
	if ((begin < 0) || (begin >= data->width))
	{
		return DUK_RET_RANGE_ERROR;
	}
	if (duk_is_null_or_undefined(ctx, 1))
	{
		end = data->width;
	}
	else
	{
		end = duk_require_int(ctx, 1);
		if (end < 0)
		{
			end += data->width;
		}
	}
	if (!(begin < end))
	{
		return DUK_RET_RANGE_ERROR;
	}

	if (data->link != data)
	{
		duk_pop(ctx);
		duk_get_prop_string(ctx, 2, DUX_IPK_PARAIO_LINK);
		/* [ uint uint/undefined this buf ] */
	}
	/* [ any any this buf ] */
	duk_push_object(ctx);
	/* [ any any this buf obj:4 ] */
	duk_swap(ctx, 3, 4);
	/* [ any any this obj buf:4 ] */
	duk_get_prototype(ctx, 2);
	duk_set_prototype(ctx, 3);
	/* [ any any this obj buf:4 ] */
	duk_put_prop_string(ctx, 3, DUX_IPK_PARAIO_LINK);
	/* [ any any this obj ] */
	duk_push_fixed_buffer(ctx, sizeof(*new_data));
	/* [ any any this obj buf:4 ] */
	new_data = (dux_paraio_data *)duk_require_buffer(ctx, 4, NULL);
	memcpy(new_data, data, sizeof(*new_data));
	new_data->link = data->link;
	new_data->width = end - begin;
	new_data->offset += begin;
	new_data->bits = ((1 << new_data->width) - 1) << new_data->offset;
	if (data->manip->slice)
	{
		result = (*data->manip->slice)(ctx, data->param, new_data->bits,
					&new_data->manip, &new_data->param);
		if (result != 0)
		{
			return result;
		}
	}
	duk_put_prop_string(ctx, 3, DUX_IPK_PARAIO_DATA);
	/* [ any any this obj ] */
	return 1; /* return obj */
}

/*
 * Common functions for ParallelIO.prototype.isXXX
 */
DUK_LOCAL duk_ret_t paraio_proto_is_all_zero(duk_context *ctx, duk_uint_t value, duk_uint_t bits)
{
	value &= bits;
	if (value == 0)
	{
		duk_push_true(ctx);
	}
	else if (value == bits)
	{
		duk_push_false(ctx);
	}
	else
	{
		return 0; /* return undefined */
	}
	return 1; /* return bool */
}

/*
 * Common functions for ParallelIO.prototype.isXXX
 */
DUK_LOCAL duk_ret_t paraio_proto_is_all_one(duk_context *ctx, duk_uint_t value, duk_uint_t bits)
{
	value &= bits;
	if (value == bits)
	{
		duk_push_true(ctx);
	}
	else if (value == 0)
	{
		duk_push_false(ctx);
	}
	else
	{
		return 0; /* return undefined */
	}
	return 1; /* return bool */
}

/*
 * Getter of ParallelIO.prototype.isAsserted
 */
DUK_LOCAL duk_ret_t paraio_proto_isAsserted_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!data->manip->read_input) ||
		((data->link->cfg_in & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->read_input)(ctx, data->param, bits, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_one(ctx, val ^ data->link->cfg_pol, bits);
}

/*
 * Getter of ParallelIO.prototype.isHigh
 */
DUK_LOCAL duk_ret_t paraio_proto_isHigh_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!data->manip->read_input) ||
		((data->link->cfg_in & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->read_input)(ctx, data->param, bits, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_one(ctx, val, bits);
}

/*
 * Getter of ParallelIO.prototype.isLow
 */
DUK_LOCAL duk_ret_t paraio_proto_isLow_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!data->manip->read_input) ||
		((data->link->cfg_in & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->read_input)(ctx, data->param, bits, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_zero(ctx, val, bits);
}

/*
 * Getter of ParallelIO.prototype.isNegated
 */
DUK_LOCAL duk_ret_t paraio_proto_isNegated_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!data->manip->read_input) ||
		((data->link->cfg_in & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->read_input)(ctx, data->param, bits, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_zero(ctx, val ^ data->link->cfg_pol, bits);
}

/*
 * Getter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t paraio_proto_value_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!data->manip->read_input) ||
		((data->link->cfg_in & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*data->manip->read_input)(ctx, data->param, bits, &val);
	if (result != 0)
	{
		return result;
	}
	duk_push_uint(ctx, (val & bits) >> data->offset);
	/* [ this uint ] */
	return 1; /* return uint */
}

/*
 * Setter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t paraio_proto_value_setter(duk_context *ctx)
{
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	duk_uint_t bits = data->bits;
	duk_uint_t val;

	/* [ uint this ] */
	if ((!data->manip->write_output) ||
		((data->link->cfg_out & bits) != bits))
	{
		return DUK_RET_TYPE_ERROR;
	}
	val = duk_require_uint(ctx, 0);
	if (val >= (1 << data->width))
	{
		return DUK_RET_RANGE_ERROR;
	}
	val <<= data->offset;
	result = (*data->manip->write_output)(ctx, data->param,
				val & bits, ~val & bits, 0);
	if (result != 0)
	{
		return result;
	}
	return 0;
}

/*
 * Getter of ParallelIO.prototype.canInput
 */
DUK_LOCAL duk_ret_t paraio_proto_canInput_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	return paraio_proto_is_all_one(ctx, data->link->cfg_in, data->bits);
}

/*
 * Getter of ParallelIO.prototype.canOutput
 */
DUK_LOCAL duk_ret_t paraio_proto_canOutput_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	return paraio_proto_is_all_one(ctx, data->link->cfg_out, data->bits);
}

/*
 * Getter of ParallelIO.prototype.isActiveHigh
 */
DUK_LOCAL duk_ret_t paraio_proto_isActiveHigh_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	return paraio_proto_is_all_zero(ctx, data->link->cfg_pol, data->bits);
}

/*
 * Getter of ParallelIO.prototype.isActiveLow
 */
DUK_LOCAL duk_ret_t paraio_proto_isActiveLow_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	return paraio_proto_is_all_one(ctx, data->link->cfg_pol, data->bits);
}

/*
 * Getter of ParallelIO.prototype.isLocked
 */
DUK_LOCAL duk_ret_t paraio_proto_isLocked_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	return paraio_proto_is_all_one(ctx, data->link->cfg_lock, data->bits);
}

/*
 * Getter of ParallelIO.prototype.width
 */
DUK_LOCAL duk_ret_t paraio_proto_width_getter(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	/* [ this ] */
	duk_push_uint(ctx, data->width);
	/* [ this uint ] */
	return 1; /* return uint */
}

/*
 * List of ParallelIO's instance methods
 */
DUK_LOCAL const duk_function_list_entry paraio_proto_funcs[] = {
	/* Output */
	{ "assert", paraio_proto_assert, 0 },
	{ "clear", paraio_proto_low, 0 },
	{ "high", paraio_proto_high, 0 },
	{ "low", paraio_proto_low, 0 },
	{ "negate", paraio_proto_negate, 0 },
	{ "off", paraio_proto_negate, 0 },
	{ "on", paraio_proto_assert, 0 },
	{ "set", paraio_proto_high, 0 },
	{ "toggle", paraio_proto_toggle, 0 },
	/* Direction */
	{ "disableInput", paraio_proto_disableInput, 0 },
	{ "disableOutput", paraio_proto_disableOutput, 0 },
	{ "enableInput", paraio_proto_enableInput, 0 },
	{ "enableOutput", paraio_proto_enableOutput, 0 },
	/* Polarity */
	{ "setActiveHigh", paraio_proto_setActiveHigh, 0 },
	{ "setActiveLow", paraio_proto_setActiveLow, 0 },
	/* Lock */
	{ "lock", paraio_proto_lock, 0 },
	{ "unlock", paraio_proto_unlock, 0 },
	/* Other */
	{ "slice", paraio_proto_slice, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of ParallelIO's instance properties
 */
DUK_LOCAL const dux_property_list_entry paraio_proto_props[] = {
	/* Pin value */
	{ "isAsserted", paraio_proto_isAsserted_getter, NULL },
	{ "isCleared", paraio_proto_isLow_getter, NULL },
	{ "isHigh", paraio_proto_isHigh_getter, NULL },
	{ "isLow", paraio_proto_isLow_getter, NULL },
	{ "isNegated", paraio_proto_isNegated_getter, NULL },
	{ "isOff", paraio_proto_isNegated_getter, NULL },
	{ "isOn", paraio_proto_isAsserted_getter, NULL },
	{ "isSet", paraio_proto_isHigh_getter, NULL },
	{ "value", paraio_proto_value_getter, paraio_proto_value_setter },
	/* Direction */
	{ "canInput", paraio_proto_canInput_getter, NULL },
	{ "canOutput", paraio_proto_canOutput_getter, NULL },
	/* Polarity */
	{ "isActiveHigh", paraio_proto_isActiveHigh_getter, NULL },
	{ "isActiveLow", paraio_proto_isActiveLow_getter, NULL },
	/* Lock */
	{ "isLocked", paraio_proto_isLocked_getter, NULL },
	/* Others */
	{ "width", paraio_proto_width_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize ParallelIO object
 */
DUK_INTERNAL duk_errcode_t dux_paraio_init(duk_context *ctx)
{
	/* [ ... ] */
	dux_push_named_c_constructor(
			ctx, "ParallelIO", paraio_constructor, 5,
			NULL, paraio_proto_funcs, NULL, paraio_proto_props);
	/* [ ... constructor ] */
	duk_put_global_string(ctx, "ParallelIO");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Manipulator functions
 */

DUK_LOCAL duk_ret_t paraio_manip_read_input(duk_context *ctx, void *param,
                                            duk_uint_t bits, duk_uint_t *result)
{
	*result = dux_hardware_read(param) & bits;
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_write_output(duk_context *ctx, void *param,
                                              duk_uint_t set, duk_uint_t clear,
                                              duk_uint_t toggle)
{
	dux_hardware_write(param,
			((dux_hardware_read(param) | set) & ~clear) ^ toggle);
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_config_enabled(duk_context *ctx, void *param,
                                                duk_uint_t bits, duk_uint_t enabled)
{
	if ((bits & enabled) != bits)
	{
		return DUK_RET_API_ERROR;
	}
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_read_config_ro(duk_context *ctx, void *param,
                                                duk_uint_t bits,
                                                duk_uint_t *input, duk_uint_t *output)
{
	*input = bits;
	*output = 0;
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_read_config_rw(duk_context *ctx, void *param,
                                                duk_uint_t bits,
                                                duk_uint_t *input, duk_uint_t *output)
{
	*input = bits;
	*output = bits;
	return 0;
}

/*
 * Manipulator definitions
 */

DUK_INTERNAL const dux_paraio_manip dux_paraio_manip_ro =
{
	.read_input = paraio_manip_read_input,
	.config_input = paraio_manip_config_enabled,
	.read_config = paraio_manip_read_config_ro,
};

DUK_INTERNAL const dux_paraio_manip dux_paraio_manip_rw =
{
	.read_input = paraio_manip_read_input,
	.write_output = paraio_manip_write_output,
	.config_input = paraio_manip_config_enabled,
	.config_output = paraio_manip_config_enabled,
	.read_config = paraio_manip_read_config_rw,
};

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_PARALLELIO */
