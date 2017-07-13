#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_PARALLELIO)
#include "../dux_internal.h"

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_PARAIO_DATA[] = DUX_IPK("piData");
DUK_LOCAL const char DUX_IPK_PARAIO_ROOT[] = DUX_IPK("piRoot");

/*
 * Structures
 */

struct dux_paraio_root;
typedef struct dux_paraio_data
{
	struct dux_paraio_root *root;
	duk_uint8_t offset;     /* 0-31 */
	duk_uint8_t width;      /* 1-32 */
	duk_uint_t bit_mask;    /* ((1<<width)-1)<<offset */
}
dux_paraio_data;

typedef struct dux_paraio_root {
	dux_paraio_data head;
	const dux_paraio_manip *manip;
	void *param;
	duk_uint_t cfg_in;
	duk_uint_t cfg_out;
	duk_uint_t cfg_pol;     /* 0=ActiveHigh, 1=ActiveLow */
	duk_uint_t cfg_lock;
}
dux_paraio_root;

/*
 * Get data pointer
 */
DUK_LOCAL dux_paraio_data *paraio_get_data(duk_context *ctx, duk_idx_t this_idx)
{
	dux_paraio_data *data;

	/* [ ... this ... ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_PARAIO_DATA);
	/* [ ... this ... buf ] */
	data = (dux_paraio_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop(ctx);
	/* [ ... this ... ] */
	return data;
}

/*
 * Get data pointer
 */
DUK_LOCAL dux_paraio_data *paraio_push_this_and_get_data(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_this(ctx);
	/* [ ... this ] */
	return paraio_get_data(ctx, -1);
}

/*
 * Constructor for new bit array
 */
DUK_LOCAL duk_ret_t paraio_construct_new(duk_context *ctx,
	duk_int_t offset, duk_int_t width, duk_uint_t polarity,
	const dux_paraio_manip *manip, void *param, dux_paraio_data **pdata)
{
	dux_paraio_root *root;
	dux_paraio_data *data;

	if ((offset < 0) || (width < 0) || (offset + width) > 32) {
		return DUK_RET_RANGE_ERROR;
	}

	duk_set_top(ctx, 0);
	duk_push_this(ctx);
	/* [ this ] */
	duk_push_fixed_buffer(ctx, sizeof(dux_paraio_root));
	/* [ this buf ] */
	root = (dux_paraio_root *)duk_require_buffer(ctx, 1, NULL);
	duk_dup(ctx, 1);
	duk_put_prop_string(ctx, 0, DUX_IPK_PARAIO_ROOT);
	duk_put_prop_string(ctx, 0, DUX_IPK_PARAIO_DATA);
	/* [ this ] */

	*pdata = data = &root->head;
	data->root     = root;
	data->width    = width;
	data->offset   = offset;
	data->bit_mask = ((1u << width) - 1) << offset;
	root->manip    = manip;
	root->param    = param;
	root->cfg_pol  = polarity;
	root->cfg_lock = 0;

	return (*root->manip->read_config)(ctx, root->param, data->bit_mask,
				&root->cfg_in, &root->cfg_out);
}

/*
 * Constructor for linked (sliced) array
 */
DUK_LOCAL duk_ret_t paraio_construct_sliced(duk_context *ctx,
	duk_int_t offset, duk_int_t width, dux_paraio_data **pdata)
{
	dux_paraio_root *root;
	dux_paraio_data *data;

	/* [ buf(root) ... ] */
	root = (dux_paraio_root *)duk_require_buffer(ctx, -1, NULL);
	duk_set_top(ctx, 1);
	/* [ buf(root) ] */
	duk_push_this(ctx);
	duk_swap(ctx, 0, 1);
	/* [ this buf(root) ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_PARAIO_ROOT);
	/* [ this ] */
	duk_push_fixed_buffer(ctx, sizeof(dux_paraio_data));
	/* [ this buf ] */
	*pdata = data = (dux_paraio_data *)duk_require_buffer(ctx, 1, NULL);
	duk_put_prop_string(ctx, 0, DUX_IPK_PARAIO_DATA);
	/* [ this ] */

	if ((offset < root->head.offset) ||
		(width < 0) ||
		((offset + offset) > (root->head.offset + root->head.width))) {
		return DUK_RET_RANGE_ERROR;
	}

	data->root     = root;
	data->offset   = offset;
	data->width    = width;
	data->bit_mask = ((1u << width) - 1) << offset;
	return 0;
}

/*
 * Getter for single bit slicing
 */
DUK_LOCAL duk_ret_t paraio_sliced_getter(duk_context *ctx)
{
	/* [ key ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	/* [ key this ] */
	duk_uarridx_t arr_idx = duk_to_uint(ctx, 0);
	if (arr_idx >= data->width) {
		/* out of range */
		return 0;
	}

	duk_get_prop_string(ctx, 1, "constructor");
	/* [ key this constructor ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_PARAIO_ROOT);
	duk_push_int(ctx, arr_idx + data->offset);
	duk_push_int(ctx, 1);
	/* [ key this constructor buf int int ] */
	duk_new(ctx, 3);
	/* [ key this retval ] */
	duk_dup(ctx, 0);
	duk_dup(ctx, 2);
	/* [ key this retval key retval ] */
	duk_def_prop(ctx, 1, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_ENUMERABLE | DUK_DEFPROP_FORCE);
	/* [ key this retval ] */
	return 1;
}

/*
 * Constructor of ParallelIO class
 */
DUK_LOCAL duk_ret_t paraio_constructor(duk_context *ctx)
{
	duk_ret_t result;
	duk_idx_t index;
	dux_paraio_data *data;

	if (duk_is_pointer(ctx, 3)) {
		/* [ int int uint ptr ptr ] for new array */
		result = paraio_construct_new(
			ctx,
			duk_require_int(ctx, 0),
			duk_require_int(ctx, 1),
			duk_require_uint(ctx, 2),
			(const dux_paraio_manip *)duk_require_pointer(ctx, 3),
			duk_require_pointer(ctx, 4),
			&data
		);
	} else {
		/* [ buf int int undefined undefined ] for sliced array */
		result = paraio_construct_sliced(
			ctx,
			duk_require_int(ctx, 1),
			duk_require_int(ctx, 2),
			&data
		);
	}

	if (result < 0) {
		return result;
	}

	/* [ this ] */
	if (data->width == 1) {
		/* Set circular reference */
		duk_push_string(ctx, "0");
		duk_dup(ctx, 0);
		duk_def_prop(ctx, 0, DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_HAVE_VALUE);
		return 0;
	}

	/* Set getter for single bit slicing */
	duk_push_c_function(ctx, paraio_sliced_getter, 1);
	/* [ this getter ] */
	for (index = 0; index < data->width; ++index) {
		duk_push_sprintf(ctx, "%d", index);
		duk_dup(ctx, 1);
		duk_def_prop(ctx, 0, DUK_DEFPROP_HAVE_GETTER);
	}
	/* [ this getter ] */
	return 0;
}

/*
 * Entry of ParallelIO.prototype.assert()
 */
DUK_LOCAL duk_ret_t paraio_proto_assert(duk_context *ctx)
{
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->write_output)(ctx, root->param,
				~root->cfg_pol & bit_mask, root->cfg_pol & bit_mask, 0);
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
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->write_output)(ctx, root->param,
				0, bit_mask, 0);
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
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->write_output)(ctx, root->param,
				bit_mask, 0, 0);
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
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->write_output)(ctx, root->param,
				root->cfg_pol & bit_mask, ~root->cfg_pol & bit_mask, 0);
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
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->write_output)(ctx, root->param,
				0, 0, bit_mask);
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
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if (root->cfg_lock & bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!root->manip->config_input)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->config_input)(ctx, root->param,
				bit_mask, 0);
	if (result != 0)
	{
		return result;
	}
	root->cfg_in &= ~bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.disableOutput()
 */
DUK_LOCAL duk_ret_t paraio_proto_disableOutput(duk_context *ctx)
{
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if (root->cfg_lock & bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!root->manip->config_output)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->config_output)(ctx, root->param,
				bit_mask, 0);
	if (result != 0)
	{
		return result;
	}
	root->cfg_out &= ~bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.enableInput()
 */
DUK_LOCAL duk_ret_t paraio_proto_enableInput(duk_context *ctx)
{
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if (root->cfg_lock & bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!root->manip->config_input)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->config_input)(ctx, root->param,
				bit_mask, bit_mask);
	if (result != 0)
	{
		return result;
	}
	root->cfg_in |= bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.enableOutput()
 */
DUK_LOCAL duk_ret_t paraio_proto_enableOutput(duk_context *ctx)
{
	/* [  ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;

	/* [ this ] */
	if (root->cfg_lock & bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (!root->manip->config_output)
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->config_output)(ctx, root->param,
				bit_mask, bit_mask);
	if (result != 0)
	{
		return result;
	}
	root->cfg_out |= bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.setActiveHigh()
 */
DUK_LOCAL duk_ret_t paraio_proto_setActiveHigh(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;

	/* [ this ] */
	if (root->cfg_lock & data->bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	root->cfg_pol &= ~data->bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.setActiveLow()
 */
DUK_LOCAL duk_ret_t paraio_proto_setActiveLow(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;

	/* [ this ] */
	if (root->cfg_lock & data->bit_mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	root->cfg_pol |= data->bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.lock()
 */
DUK_LOCAL duk_ret_t paraio_proto_lock(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;

	/* [ this ] */
	root->cfg_lock |= data->bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.unlock()
 */
DUK_LOCAL duk_ret_t paraio_proto_unlock(duk_context *ctx)
{
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;

	/* [ this ] */
	root->cfg_lock &= ~data->bit_mask;
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.slice()
 */
DUK_LOCAL duk_ret_t paraio_proto_slice(duk_context *ctx)
{
	/* [ int int/undefined ] */
	dux_paraio_data *data;
	/* [ int int/undefined this ] */
	duk_int_t begin, end;
	begin = duk_require_int(ctx, 0);
	if (duk_is_null_or_undefined(ctx, 1)) {
		end = DUK_INT_MAX;
	} else {
		end = duk_require_int(ctx, 1);
	}

	duk_set_top(ctx, 0);
	/* [  ] */
	data = paraio_push_this_and_get_data(ctx);
	/* [ this ] */
	if ((end <= begin) || (begin < 0) || (begin >= data->width)) {
		/* return empty array */
		duk_push_array(ctx);
		return 1;
	}

	/* adjust end */
	if (end > data->width) {
		end = data->width;
	}

	duk_get_prop_string(ctx, 0, "constructor");
	/* [ this constructor ] */
	duk_get_prop_string(ctx, 0, DUX_IPK_PARAIO_ROOT);
	duk_push_int(ctx, begin + data->offset);
	duk_push_int(ctx, end - begin);
	/* [ this constructor buf int int ] */
	duk_new(ctx, 3);
	/* [ this retval ] */
	return 1;
}

/*
 * Common functions for ParallelIO.prototype.isXXX
 */
DUK_LOCAL duk_ret_t paraio_proto_is_all_zero(duk_context *ctx, duk_uint_t value, duk_uint_t bit_mask)
{
	value &= bit_mask;
	if (value == 0)
	{
		duk_push_true(ctx);
	}
	else if (value == bit_mask)
	{
		duk_push_false(ctx);
	}
	else
	{
		duk_push_null(ctx);
	}
	return 1; /* return bool/null */
}

/*
 * Common functions for ParallelIO.prototype.isXXX
 */
DUK_LOCAL duk_ret_t paraio_proto_is_all_one(duk_context *ctx, duk_uint_t value, duk_uint_t bit_mask)
{
	value &= bit_mask;
	if (value == bit_mask)
	{
		duk_push_true(ctx);
	}
	else if (value == 0)
	{
		duk_push_false(ctx);
	}
	else
	{
		duk_push_null(ctx);
	}
	return 1; /* return bool/null */
}

/*
 * Getter of ParallelIO.prototype.isAsserted
 */
DUK_LOCAL duk_ret_t paraio_proto_isAsserted_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!root->manip->read_input) ||
		((root->cfg_in & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->read_input)(ctx, root->param, bit_mask, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_one(ctx, val ^ root->cfg_pol, bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isHigh
 */
DUK_LOCAL duk_ret_t paraio_proto_isHigh_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!root->manip->read_input) ||
		((root->cfg_in & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->read_input)(ctx, root->param, bit_mask, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_one(ctx, val, bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isLow
 */
DUK_LOCAL duk_ret_t paraio_proto_isLow_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!root->manip->read_input) ||
		((root->cfg_in & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->read_input)(ctx, root->param, bit_mask, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_zero(ctx, val, bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isNegated
 */
DUK_LOCAL duk_ret_t paraio_proto_isNegated_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!root->manip->read_input) ||
		((root->cfg_in & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->read_input)(ctx, root->param, bit_mask, &val);
	if (result != 0)
	{
		return result;
	}
	return paraio_proto_is_all_zero(ctx, val ^ root->cfg_pol, bit_mask);
}

/*
 * Getter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t paraio_proto_value_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_ret_t result;
	duk_uint_t val;
	/* [ this ] */
	if ((!root->manip->read_input) ||
		((root->cfg_in & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	result = (*root->manip->read_input)(ctx, root->param, bit_mask, &val);
	if (result != 0)
	{
		return result;
	}
	duk_push_uint(ctx, (val & bit_mask) >> data->offset);
	/* [ this uint ] */
	return 1; /* return uint */
}

/*
 * Setter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t paraio_proto_value_setter(duk_context *ctx)
{
	/* [ uint ] */
	duk_ret_t result;
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	duk_uint_t bit_mask = data->bit_mask;
	duk_uint_t val;

	/* [ uint this ] */
	if ((!root->manip->write_output) ||
		((root->cfg_out & bit_mask) != bit_mask))
	{
		return DUK_RET_TYPE_ERROR;
	}
	val = duk_require_uint(ctx, 0);
	if (val >= (1 << data->width))
	{
		return DUK_RET_RANGE_ERROR;
	}
	val <<= data->offset;
	result = (*root->manip->write_output)(ctx, root->param,
				val & bit_mask, ~val & bit_mask, 0);
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
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	return paraio_proto_is_all_one(ctx, root->cfg_in, data->bit_mask);
}

/*
 * Getter of ParallelIO.prototype.canOutput
 */
DUK_LOCAL duk_ret_t paraio_proto_canOutput_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	return paraio_proto_is_all_one(ctx, root->cfg_out, data->bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isActiveHigh
 */
DUK_LOCAL duk_ret_t paraio_proto_isActiveHigh_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	return paraio_proto_is_all_zero(ctx, root->cfg_pol, data->bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isActiveLow
 */
DUK_LOCAL duk_ret_t paraio_proto_isActiveLow_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	return paraio_proto_is_all_one(ctx, root->cfg_pol, data->bit_mask);
}

/*
 * Getter of ParallelIO.prototype.isLocked
 */
DUK_LOCAL duk_ret_t paraio_proto_isLocked_getter(duk_context *ctx)
{
	/* [  ] */
	dux_paraio_data *data = paraio_push_this_and_get_data(ctx);
	dux_paraio_root *root = data->root;
	return paraio_proto_is_all_one(ctx, root->cfg_lock, data->bit_mask);
}

/*
 * Getter of ParallelIO.prototype.width
 */
DUK_LOCAL duk_ret_t paraio_proto_width_getter(duk_context *ctx)
{
	/* [  ] */
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
	/* [ ... Hardware ] */
	dux_push_named_c_constructor(
			ctx, "ParallelIO", paraio_constructor, 5,
			NULL, paraio_proto_funcs, NULL, paraio_proto_props);
	/* [ ... Hardware constructor ] */
	duk_put_prop_string(ctx, -2, "ParallelIO");
	/* [ ... Hardware ] */
	return DUK_ERR_NONE;
}

/*
 * Manipulator functions
 */

DUK_LOCAL duk_ret_t paraio_manip_read_input(duk_context *ctx, void *param,
                                            duk_uint_t bit_mask, duk_uint_t *result)
{
	*result = dux_hardware_read(param) & bit_mask;
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
                                                duk_uint_t bit_mask, duk_uint_t enabled)
{
	if ((bit_mask & enabled) != bit_mask)
	{
		return DUK_RET_ERROR;
	}
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_read_config_ro(duk_context *ctx, void *param,
                                                duk_uint_t bit_mask,
                                                duk_uint_t *input, duk_uint_t *output)
{
	*input = bit_mask;
	*output = 0;
	return 0;
}

DUK_LOCAL duk_ret_t paraio_manip_read_config_rw(duk_context *ctx, void *param,
                                                duk_uint_t bit_mask,
                                                duk_uint_t *input, duk_uint_t *output)
{
	*input = bit_mask;
	*output = bit_mask;
	return 0;
}

/*
 * Manipulator tables
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
