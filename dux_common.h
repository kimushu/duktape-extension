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
		const duk_function_list_entry *static_methods,
		const duk_function_list_entry *prototype_methods)
{
	duk_idx_t result = dux_push_named_c_function(ctx, name, func, nargs);
	if (static_methods)
	{
		duk_put_function_list(ctx, -1, static_methods);
	}
	if (prototype_methods)
	{
		duk_push_object(ctx);
		duk_put_function_list(ctx, -1, prototype_methods);
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
