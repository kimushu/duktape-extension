#ifndef DUX_COMMON_H_
#define DUX_COMMON_H_

#include "duktape.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void dux_register_tick(duk_context *ctx, const char *key);
extern void dux_queue_callback(duk_context *ctx);
extern void dux_tick(duk_context *ctx);

typedef struct dux_property_list_entry
{
	const char *key;
	duk_c_function getter;
	duk_c_function setter;
}
dux_property_list_entry;

/*
 * Bind arguments (Function.bind(undefined, args...))
 *
 * [ func arg1 ... argN ]  ->  [ bound_func ]
 */
__inline__ static
void dux_bind_arguments(duk_context *ctx, duk_idx_t nargs)
{
	/* [ ... func ] */
	duk_push_string(ctx, "bind");
	duk_insert(ctx, -(1 + nargs));
	/* [ ... func "bind" arg1 ... argN ] */
	duk_push_undefined(ctx);
	duk_insert(ctx, -(1 + nargs));
	/* [ ... func "bind" undefined arg1 ... argN ] */
	duk_call_prop(ctx, -(3 + nargs), (2 + nargs));
	/* [ ... func bound_func ] */
	duk_replace(ctx, -2);
	/* [ ... bound_func ] */
}

/*
 * Allocate and clear memory (may call GC)
 */
__inline__ static
void *dux_calloc(duk_context *ctx, duk_size_t size)
{
	void *ptr = duk_alloc(ctx, size);
	if (ptr)
	{
		memset(ptr, 0, size);
	}
	return ptr;
}

/*
 * Allocate and clear memory (never call GC)
 */
__inline__ static
void *dux_calloc_raw(duk_context *ctx, duk_size_t size)
{
	void *ptr = duk_alloc_raw(ctx, size);
	if (ptr)
	{
		memset(ptr, 0, size);
	}
	return ptr;
}

/*
 * Define multiple properties
 */
__inline__ static
void dux_put_property_list(duk_context *ctx, duk_idx_t obj_index,
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
			flags |= DUK_DEFPROP_HAVE_SETTER | DUK_DEFPROP_SET_WRITABLE;
		}
		duk_def_prop(ctx, obj_index, flags);
	}
}

/*
 * Push duk_c_function with name property
 */
__inline__ static
duk_idx_t dux_push_named_c_function(
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
 * Push duk_c_function with name property and methods
 */
__inline__ static
duk_idx_t dux_push_named_c_constructor(
		duk_context *ctx, const char *name,
		duk_c_function func, duk_idx_t nargs,
		const duk_function_list_entry *static_funcs,
		const duk_function_list_entry *prototype_funcs,
		const dux_property_list_entry *static_props,
		const dux_property_list_entry *prototype_props)
{
	duk_idx_t result = dux_push_named_c_function(ctx, name, func, nargs);
	if (static_funcs)
	{
		duk_put_function_list(ctx, -1, static_funcs);
	}
	if (static_props)
	{
		dux_put_property_list(ctx, -1, static_props);
	}
	if (prototype_funcs || prototype_props)
	{
		duk_push_object(ctx);
		if (prototype_funcs)
		{
			duk_put_function_list(ctx, -1, prototype_funcs);
		}
		if (prototype_props)
		{
			dux_put_property_list(ctx, -1, prototype_props);
		}
		duk_put_prop_string(ctx, -2, "prototype");
	}
	return result;
}

/*
 * Unregister tick handler
 */
__inline__ static
void dux_unregister_rick(duk_context *ctx, const char *key)
{
	duk_push_undefined(ctx);
	dux_register_tick(ctx, key);
}

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_COMMON_H_ */
