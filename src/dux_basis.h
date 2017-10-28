#ifndef DUX_BASIS_H_INCLUDED
#define DUX_BASIS_H_INCLUDED

/*
 * Macros
 */

#define DUX_IPK(t)          ("\377d" t)

/*
 * Constants
 */

enum
{
	DUX_TICK_RET_JOBLESS  = 0,
	DUX_TICK_RET_CONTINUE = 1,
	DUX_TICK_RET_ABORT    = 2,
};

/*
 * Structures
 */

typedef struct dux_property_list_entry
{
	const char *key;
	duk_c_function getter;
	duk_c_function setter;
}
dux_property_list_entry;

/*
 * Typedefs
 */
typedef duk_errcode_t (*dux_initializer)(duk_context *ctx);
typedef duk_int_t (*dux_tick_handler)(duk_context *ctx);

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_invoke_initializers(duk_context *ctx, ...);
DUK_INTERNAL_DECL duk_int_t dux_invoke_tick_handlers(duk_context *ctx, ...);
DUK_INTERNAL_DECL void dux_report_error(duk_context *ctx);
DUK_INTERNAL_DECL void dux_report_warning(duk_context *ctx);
DUK_INTERNAL_DECL void dux_push_inherited_object(duk_context *ctx, duk_idx_t super_idx);
DUK_INTERNAL_DECL void *dux_to_byte_buffer(duk_context *ctx, duk_idx_t index, duk_size_t *out_size);
DUK_INTERNAL_DECL duk_int_t dux_require_int_range(duk_context *ctx, duk_idx_t index,
		duk_int_t minimum, duk_int_t maximum);
DUK_INTERNAL_DECL duk_bool_t dux_get_array_index(duk_context *ctx, duk_idx_t key_idx, duk_uarridx_t *result);
DUK_INTERNAL_DECL duk_ret_t dux_read_file(duk_context *ctx, const char *path);

/*
 * Bind arguments (Function.bind(undefined, args...))
 *
 * [ ... func arg1 ... argN ]  ->  [ ... bound_func ]
 */
DUK_LOCAL
DUK_INLINE void dux_bind_arguments(duk_context *ctx, duk_idx_t nargs)
{
	/* [ ... func arg1 ... argN ] */
	duk_push_string(ctx, "bind");
	duk_insert(ctx, -(1 + nargs));
	/* [ ... func "bind" arg1 ... argN ] */
	duk_push_undefined(ctx);
	duk_insert(ctx, -(1 + nargs));
	/* [ ... func "bind" undefined arg1 ... argN ] */
	duk_call_prop(ctx, -(3 + nargs), (1 + nargs));
	/* [ ... func bound_func ] */
	duk_replace(ctx, -2);
	/* [ ... bound_func ] */
}

/*
 * Bind arguments with this value (Function.bind(thisArg, args...))
 *
 * [ ... func thisArg arg1 ... argN ]  ->  [ ... bound_func ]
 */
DUK_LOCAL
DUK_INLINE void dux_bind_this_arguments(duk_context *ctx, duk_idx_t nargs)
{
	/* [ ... func thisArg arg1 ... argN ] */
	duk_push_string(ctx, "bind");
	duk_insert(ctx, -(2 + nargs));
	/* [ ... func "bind" thisArg arg1 ... argN ] */
	duk_call_prop(ctx, -(3 + nargs), (1 + nargs));
	/* [ ... func bound_func ] */
	duk_replace(ctx, -2);
	/* [ ... bound_func ] */
}

/*
 * Bind this value (Function.bind(thisArg))
 *
 * [ ... func thisArg ]  ->  [ ... bound_func ]
 */
DUK_LOCAL
DUK_INLINE void dux_bind_this(duk_context *ctx)
{
	return dux_bind_this_arguments(ctx, 0);
}

/*
 * Define multiple properties
 */
DUK_LOCAL
DUK_INLINE void dux_put_property_list(duk_context *ctx, duk_idx_t obj_index,
		const dux_property_list_entry *props)
{
	const char *key;
	obj_index = duk_normalize_index(ctx, obj_index);
	for (; (key = props->key) != NULL; ++props)
	{
		duk_uint_t flags = DUK_DEFPROP_SET_ENUMERABLE;
		duk_c_function func;

		duk_push_string(ctx, key);
		func = props->getter;
		if (func)
		{
			duk_push_c_function(ctx, func, 0);
			flags |= DUK_DEFPROP_HAVE_GETTER;
		}
		func = props->setter;
		if (func)
		{
			duk_push_c_function(ctx, func, 1);
			flags |= DUK_DEFPROP_HAVE_SETTER;
		}
		duk_def_prop(ctx, obj_index, flags);
	}
}

/*
 * Push duk_c_function with name property
 */
DUK_LOCAL
DUK_INLINE duk_idx_t dux_push_named_c_function(
		duk_context *ctx, const char *name,
		duk_c_function func, duk_idx_t nargs)
{
	duk_idx_t result = duk_push_c_function(ctx, func, nargs);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
	return result;
}

/*
 * Push duk_c_function with name, property and methods
 */
DUK_LOCAL
DUK_INLINE duk_idx_t dux_push_named_c_constructor(
		duk_context *ctx, const char *name,
		duk_c_function func, duk_idx_t nargs,
		const duk_function_list_entry *static_funcs,
		const duk_function_list_entry *prototype_funcs,
		const dux_property_list_entry *static_props,
		const dux_property_list_entry *prototype_props)
{
	/* [ ... ] */
	duk_idx_t result = dux_push_named_c_function(ctx, name, func, nargs);
	/* [ ... constructor ] */
	if (static_funcs)
	{
		duk_put_function_list(ctx, -1, static_funcs);
	}
	if (static_props)
	{
		dux_put_property_list(ctx, -1, static_props);
	}
	duk_push_object(ctx);
	/* [ ... constructor obj ] */
	if (prototype_funcs)
	{
		duk_put_function_list(ctx, -1, prototype_funcs);
	}
	if (prototype_props)
	{
		dux_put_property_list(ctx, -1, prototype_props);
	}
	duk_dup(ctx, -2);
	duk_put_prop_string(ctx, -2, "constructor");
	duk_put_prop_string(ctx, -2, "prototype");
	/* [ ... constructor ] */
	return result;
}

/*
 * Push duk_c_function as inherited constructor
 * with name, property and methods
 */
DUK_LOCAL
DUK_INLINE duk_idx_t dux_push_inherited_named_c_constructor(
		duk_context *ctx, duk_idx_t super_idx, const char *name,
		duk_c_function func, duk_idx_t nargs,
		const duk_function_list_entry *static_funcs,
		const duk_function_list_entry *prototype_funcs,
		const dux_property_list_entry *static_props,
		const dux_property_list_entry *prototype_props)
{
	duk_idx_t result;
	super_idx = duk_normalize_index(ctx, super_idx);
	result = dux_push_named_c_function(ctx, name, func, nargs);
	if (static_funcs)
	{
		duk_put_function_list(ctx, -1, static_funcs);
	}
	if (static_props)
	{
		dux_put_property_list(ctx, -1, static_props);
	}
	dux_push_inherited_object(ctx, super_idx);
	if (prototype_funcs)
	{
		duk_put_function_list(ctx, -1, prototype_funcs);
	}
	if (prototype_props)
	{
		dux_put_property_list(ctx, -1, prototype_props);
	}
	duk_put_prop_string(ctx, -2, "prototype");
	return result;
}

/*
 * Push function of constructor of super class
 */
DUK_LOCAL
DUK_INLINE void dux_push_super_constructor(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_current_function(ctx);
	/* [ ... constructor ] */
	duk_get_prop_string(ctx, -1, "prototype");
	/* [ ... constructor prototype ] */
	duk_get_prototype(ctx, -1);
	/* [ ... constructor prototype super_prototype ] */
	duk_get_prop_string(ctx, -1, "constructor");
	/* [ ... constructor prototype super_prototype super ] */
	duk_replace(ctx, -4);
	/* [ ... super prototype super_prototype ] */
	duk_pop_2(ctx);
	/* [ ... super ] */
}

#endif  /* !DUX_BASIS_H_INCLUDED */
