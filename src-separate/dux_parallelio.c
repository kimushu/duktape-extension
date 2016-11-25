#include "dux_parallelio.h"

static const char *const DUX_PARALLELIO_DATA = "\xff" "data";

static duk_ret_t pio_constructor(duk_context *ctx)
{
	dux_parallelio_t data, *pdata;

	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

	data.width = duk_require_uint(ctx, 0);
	if ((data.width == 0) || (data.width > 32))
	{
		return DUK_RET_RANGE_ERROR;
	}
	data.offset = duk_require_uint(ctx, 1);
	if ((data.offset >= 32) || ((data.offset + data.width) > 32))
	{
		return DUK_RET_RANGE_ERROR;
	}
	data.mask = ((1 << data.width) - 1) << data.offset;
	data.val_ptr = (duk_uint_t *)duk_require_uint(ctx, 2);
	data.dir_ptr = (duk_uint_t *)duk_require_uint(ctx, 3);
	if (data.dir_ptr)
	{
		data.dir_pol = duk_require_uint(ctx, 4);
		data.dir_val = *data.dir_ptr ^ data.dir_pol;
	}
	else
	{
		data.dir_val = duk_require_uint(ctx, 4);
		data.dir_pol = 0;
	}
	data.pol_val = duk_require_uint(ctx, 5);

	pdata = (dux_parallelio_t *)duk_alloc(ctx, sizeof(*pdata));
	if (!pdata)
	{
		return DUK_RET_ALLOC_ERROR;
	}
	memcpy(pdata, &data, sizeof(*pdata));

	duk_push_this(ctx);
	duk_push_pointer(ctx, pdata);
	duk_put_prop_string(ctx, -2, DUX_PARALLELIO_DATA);

	return 0;
}

static dux_parallelio_t *pio_get_data(duk_context *ctx)
{
	dux_parallelio_t *data;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_PARALLELIO_DATA);
	data = (dux_parallelio_t *)duk_get_pointer(ctx, -1);
	duk_pop_2(ctx);
	return data;
}

static duk_ret_t pio_proto_assert(duk_context *ctx)
{
	dux_parallelio_t *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs) |
		((data->pol_val ^ data->mask) & outputs);
	duk_push_this(ctx);
	return 1;	/* return this; */
}

static duk_ret_t pio_proto_low(duk_context *ctx)
{
	dux_parallelio_t *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs);
	duk_push_this(ctx);
	return 1;	/* return this; */
}

static duk_ret_t pio_proto_high(duk_context *ctx)
{
	dux_parallelio_t *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs) | outputs;
	duk_push_this(ctx);
	return 1;	/* return this; */
}

static duk_ret_t pio_proto_negate(duk_context *ctx)
{
	dux_parallelio_t *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs) |
		((data->pol_val ^ ~data->mask) & outputs);
	duk_push_this(ctx);
	return 1;	/* return this; */
}

static duk_ret_t pio_proto_toggle(duk_context *ctx)
{
	dux_parallelio_t *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr ^= outputs;
	duk_push_this(ctx);
	return 1;	/* return this; */
}

static duk_function_list_entry pio_proto_funcs[] = {

	/* Output */
	{ "assert", pio_proto_assert, 0 },
	{ "clear", pio_proto_low, 0 },
	{ "high", pio_proto_high, 0 },
	{ "low", pio_proto_low, 0 },
	{ "negate", pio_proto_negate, 0 },
	{ "off", pio_proto_negate, 0 },
	{ "on", pio_proto_assert, 0 },
	{ "set", pio_proto_high, 0 },
	{ "toggle", pio_proto_toggle, 0 },
	{ NULL, NULL, 0 }
};

static void duk_def_prop_getter(duk_context *ctx, duk_idx_t obj_index, const char *key, duk_c_function func)
{
	obj_index = duk_normalize_index(ctx, obj_index);
	duk_push_string(ctx, key);
	duk_push_c_function(ctx, func, 0);
	duk_def_prop(ctx, obj_index, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_SET_ENUMERABLE);
}

/*
 * Initialize ParallelIO object
 */
void dux_parallelio_init(duk_context *ctx)
{
	duk_push_c_function(ctx, pio_constructor, 6);

	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, pio_proto_funcs);
	// duk_def_prop_getter(ctx, -1, "is_asserted", pio_proto_is_asserted);
	// duk_def_prop_getter(ctx, -1, "is_cleared", pio_proto_is_low);
	// duk_def_prop_getter(ctx, -1, "is_high", pio_proto_is_high);
	// duk_def_prop_getter(ctx, -1, "is_low", pio_proto_is_low);
	// duk_def_prop_getter(ctx, -1, "is_negated", pio_proto_is_negated);
	// duk_def_prop_getter(ctx, -1, "is_off", pio_proto_is_negated);
	// duk_def_prop_getter(ctx, -1, "is_on", pio_proto_is_asserted);
	// duk_def_prop_getter(ctx, -1, "is_set", pio_proto_is_high);
	duk_put_prop_string(ctx, -2, "prototype");

	duk_put_global_string(ctx, "ParallelIO");
}

