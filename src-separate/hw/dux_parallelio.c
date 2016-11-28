/*
 * ECMA classes:
 *    new ParallelIO(
 *      <uint> width,
 *      <uint> offset,
 *      <pointer> val_ptr,
 *      <pointer> dir_ptr,
 *      <uint> dir_pol[dir_ptr!=NULL] / dir_val[dir_ptr==NULL],
 *      <uint> pol_val
 *    );
 *
 * ECMA class methods:
 *    ParallelIO.prototype.assert()
 *    ParallelIO.prototype.clear()  // same as low()
 *    ParallelIO.prototype.high()
 *    ParallelIO.prototype.low()
 *    ParallelIO.prototype.negate()
 *    ParallelIO.prototype.off()    // same as negate()
 *    ParallelIO.prototype.on()     // same as assert()
 *    ParallelIO.prototype.set()    // same as high()
 *    ParallelIO.prototype.toggle()
 *
 * ECMA class properties:
 *    ParallelIO.prototype.value
 *    ParallelIO.prototype.width
 *
 * Internal data structure:
 *    <ParallelIO object> = {
 *      [DUX_IPK_PARALLELIO_DATA]: <PlainBuffer dux_parallelio_data>
 *      [DUX_IPK_PARALLELIO_LINK]: <PlainBuffer dux_parallelio_data>
 *    };
 */
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_PARALLELIO)
#include "../dux_internal.h"

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_PARALLELIO_DATA[] = DUX_IPK("piData");
DUK_LOCAL const char DUX_IPK_PARALLELIO_LINK[] = DUX_IPK("piLink");

/*
 * Structures
 */

struct dux_parallelio_data;
typedef struct dux_parallelio_data
{
	struct dux_parallelio_data *link;
	duk_int_t width;        /* 1-32 */
	duk_int_t offset;       /* 0-31 */
	duk_uint_t mask;        /* ((1<<width)-1)<<offset */
	duk_uint_t *val_ptr;
	duk_uint_t *dir_ptr;
	duk_uint_t dir_val;     /* 0=Input,1=Output */
	duk_uint_t dir_pol;     /* 0=NonInvert,1=Invert */
	duk_uint_t pol_val;     /* 0=ActiveHigh,1=ActiveLow [linked] */
}
dux_parallelio_data;

/*
 * Get data pointer
 */
DUK_LOCAL dux_parallelio_data *pio_get_data(duk_context *ctx)
{
	dux_parallelio_data *data;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_IPK_PARALLELIO_DATA);
	data = (dux_parallelio_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	return data;
}

/*
 * Constructor of ParallelIO class
 */
DUK_LOCAL duk_ret_t pio_constructor(duk_context *ctx)
{
	dux_parallelio_data data, *pdata;

	/* [ uint uint pointer pointer uint uint ] */
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
	data.val_ptr = (duk_uint_t *)duk_require_pointer(ctx, 2);
	data.dir_ptr = (duk_uint_t *)duk_require_pointer(ctx, 3);
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

	duk_set_top(ctx, 0);
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	duk_push_fixed_buffer(ctx, sizeof(*pdata));
	/* [ this buf ] */
	pdata = (dux_parallelio_data *)duk_require_buffer(ctx, 1, NULL);
	data.link = pdata;
	memcpy(pdata, &data, sizeof(*pdata));
	duk_put_prop_string(ctx, 0, DUX_IPK_PARALLELIO_DATA);
	/* [ this ] */

	return 0; /* return this */
}

/*
 * Entry of ParallelIO.prototype.assert()
 */
DUK_LOCAL duk_ret_t pio_proto_assert(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs) |
		((data->link->pol_val ^ data->mask) & outputs);
	duk_push_this(ctx);
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.low()
 */
DUK_LOCAL duk_ret_t pio_proto_low(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr = (*data->val_ptr & ~outputs);
	duk_push_this(ctx);
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.high()
 */
DUK_LOCAL duk_ret_t pio_proto_high(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr = (*data->val_ptr & ~outputs) | outputs;
	duk_push_this(ctx);
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.negate()
 */
DUK_LOCAL duk_ret_t pio_proto_negate(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr =
		(*data->val_ptr & ~outputs) |
		((data->link->pol_val ^ ~data->mask) & outputs);
	duk_push_this(ctx);
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.toggle()
 */
DUK_LOCAL duk_ret_t pio_proto_toggle(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t outputs = data->dir_val;
	*data->val_ptr ^= outputs;
	duk_push_this(ctx);
	return 1; /* return this */
}

/*
 * Entry of ParallelIO.prototype.slice()
 */
DUK_LOCAL duk_ret_t pio_proto_slice(duk_context *ctx)
{
	dux_parallelio_data *data;
	dux_parallelio_data *new_data;
	duk_int_t begin, end;

	/* [ uint uint/undefined ] */
	duk_push_this(ctx);
	/* [ uint uint/undefined this ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PARALLELIO_DATA);
	/* [ uint uint/undefined this buf ] */
	data = (dux_parallelio_data *)duk_require_buffer(ctx, 3, NULL);

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
		duk_get_prop_string(ctx, 2, DUX_IPK_PARALLELIO_LINK);
		/* [ uint uint/undefined this buf ] */
	}
	duk_push_object(ctx);
	duk_replace(ctx, 1);
	/* [ uint obj this buf ] */
	duk_get_prototype(ctx, 2);
	duk_set_prototype(ctx, 1);
	/* [ uint obj this buf ] */
	duk_put_prop_string(ctx, 1, DUX_IPK_PARALLELIO_LINK);
	/* [ uint obj this ] */
	duk_pop(ctx);
	/* [ uint obj ] */
	duk_push_fixed_buffer(ctx, sizeof(*new_data));
	/* [ uint obj buf ] */
	new_data = (dux_parallelio_data *)duk_require_buffer(ctx, 2, NULL);
	memcpy(new_data, data, sizeof(*new_data));
	new_data->link = data->link;
	new_data->width = end - begin;
	new_data->offset += begin;
	new_data->mask = ((1 << new_data->width) - 1) << new_data->offset;
	duk_put_prop_string(ctx, 1, DUX_IPK_PARALLELIO_DATA);
	/* [ uint obj ] */
	return 1; /* return obj */
}

/*
 * Getter of ParallelIO.prototype.activeHigh
 */
DUK_LOCAL duk_ret_t pio_proto_activeHigh_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t pol = data->link->pol_val & data->mask;
	if (pol == 0)
	{
		duk_push_true(ctx);
	}
	else if (pol == data->mask)
	{
		duk_push_false(ctx);
	}
	else
	{
		duk_push_undefined(ctx);
	}
	return 1; /* return true/false/undefined */
}

/*
 * Setter of ParallelIO.prototype.activeHigh
 */
DUK_LOCAL duk_ret_t pio_proto_activeHigh_setter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	data->link->pol_val |= data->mask;
	return 0; /* return undefined */
}

/*
 * Getter of ParallelIO.prototype.activeLow
 */
DUK_LOCAL duk_ret_t pio_proto_activeLow_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t pol = data->link->pol_val & data->mask;
	if (pol == data->mask)
	{
		duk_push_true(ctx);
	}
	else if (pol == 0)
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
 * Setter of ParallelIO.prototype.activeLow
 */
DUK_LOCAL duk_ret_t pio_proto_activeLow_setter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	data->link->pol_val |= data->mask;
	return 0; /* return undefined */
}

/*
 * Getter of ParallelIO.prototype.isInput
 */
DUK_LOCAL duk_ret_t pio_proto_isInput_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t dir = (data->dir_val ^ data->dir_pol) & data->mask;
	if (dir == 0)
	{
		duk_push_true(ctx);
	}
	else if (dir == data->mask)
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
 * Getter of ParallelIO.prototype.isOutput
 */
DUK_LOCAL duk_ret_t pio_proto_isOutput_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t dir = (data->dir_val ^ data->dir_pol) & data->mask;
	if (dir == data->mask)
	{
		duk_push_true(ctx);
	}
	else if (dir == 0)
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
DUK_LOCAL duk_ret_t pio_proto_isAsserted_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t val = (*data->val_ptr ^ data->link->pol_val) & data->mask;
	if (val == data->mask)
	{
		duk_push_true(ctx);
	}
	else if (val == 0)
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
 * Getter of ParallelIO.prototype.isHigh
 */
DUK_LOCAL duk_ret_t pio_proto_isHigh_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t val = *data->val_ptr & data->mask;
	if (val == data->mask)
	{
		duk_push_true(ctx);
	}
	else if (val == 0)
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
 * Getter of ParallelIO.prototype.isLow
 */
DUK_LOCAL duk_ret_t pio_proto_isLow_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t val = *data->val_ptr & data->mask;
	if (val == 0)
	{
		duk_push_true(ctx);
	}
	else if (val == data->mask)
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
 * Getter of ParallelIO.prototype.isNegated
 */
DUK_LOCAL duk_ret_t pio_proto_isNegated_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t val = (*data->val_ptr ^ data->link->pol_val) & data->mask;
	if (val == 0)
	{
		duk_push_true(ctx);
	}
	else if (val == data->mask)
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
 * Getter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t pio_proto_value_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_push_uint(ctx, (*data->val_ptr & data->mask) >> data->offset);
	return 1; /* return uint */
}

/*
 * Setter of ParallelIO.prototype.value
 */
DUK_LOCAL duk_ret_t pio_proto_value_setter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_uint_t value = duk_require_uint(ctx, 0);
	if ((data->dir_val & data->mask) != data->mask)
	{
		return DUK_RET_TYPE_ERROR;
	}
	if (value >= (1 << data->width))
	{
		return DUK_RET_RANGE_ERROR;
	}
	*data->val_ptr =
		(*data->val_ptr & ~data->mask) |
		(value << data->offset);
	return 0; /* return undefined */
}

/*
 * Getter of ParallelIO.prototype.width
 */
DUK_LOCAL duk_ret_t pio_proto_width_getter(duk_context *ctx)
{
	dux_parallelio_data *data = pio_get_data(ctx);
	duk_push_uint(ctx, data->width);
	return 1; /* return uint */
}

/*
 * List of ParallelIO's instance methods
 */
DUK_LOCAL const duk_function_list_entry pio_proto_funcs[] = {
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
	/* Slicing */
	{ "slice", pio_proto_slice, 2 },
	{ NULL, NULL, 0 }
};

/*
 * List of ParallelIO's instance properties
 */
DUK_LOCAL const dux_property_list_entry pio_proto_props[] = {
	/* Polarity */
	{ "activeHigh", pio_proto_activeHigh_getter, pio_proto_activeHigh_setter },
	{ "activeLow", pio_proto_activeLow_getter, pio_proto_activeLow_setter },
	/* Direction */
	{ "isInput", pio_proto_isInput_getter, NULL },
	{ "isOutput", pio_proto_isOutput_getter, NULL },
	/* Pin value */
	{ "isAsserted", pio_proto_isAsserted_getter, NULL },
	{ "isCleared", pio_proto_isLow_getter, NULL },
	{ "isHigh", pio_proto_isHigh_getter, NULL },
	{ "isLow", pio_proto_isLow_getter, NULL },
	{ "isNegated", pio_proto_isNegated_getter, NULL },
	{ "isOff", pio_proto_isNegated_getter, NULL },
	{ "isOn", pio_proto_isAsserted_getter, NULL },
	{ "isSet", pio_proto_isHigh_getter, NULL },
	{ "value", pio_proto_value_getter, pio_proto_value_setter },
	/* Others */
	{ "width", pio_proto_width_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize ParallelIO object
 */
DUK_INTERNAL duk_errcode_t dux_parallelio_init(duk_context *ctx)
{
	/* [ ... ] */
	dux_push_named_c_constructor(
			ctx, "ParallelIO", pio_constructor, 6,
			NULL, pio_proto_funcs, NULL, pio_proto_props);
	/* [ ... constructor ] */
	duk_put_global_string(ctx, "ParallelIO");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_PARALLELIO */
