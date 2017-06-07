#include "dux_internal.h"

DUK_LOCAL const char DUX_IPK_MODULES_GLOBALS[] = DUX_IPK("modG");

DUK_LOCAL duk_ret_t modules_require(duk_context *ctx)
{
	const char *name;
	dux_module_entry entry;

	/* [ name ] */
	name = duk_require_string(ctx, 0);
	duk_push_current_function(ctx);
	/* [ name require ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_MODULES_GLOBALS);
	/* [ name require obj ] */
	if (!duk_get_prop_string(ctx, 2, name)) {
		/* no module */
		duk_error(ctx, DUK_ERR_ERROR, "Cannot find module '%s'", name);
	}
	/* [ name require obj module|entry ] */
	entry = duk_get_pointer(ctx, 3);
	if (!entry) {
		/* module is already loaded */
		/* [ name require obj module:3 ] */
		duk_get_prop_string(ctx, 3, "exports");
		/* [ name require obj module:3 exports:4 ] */
		return 1;
	}

	/* module is not loaded */
	/* [ name require obj entry:3 ] */
	duk_pop(ctx);
	/* [ name require obj ] */
	duk_push_object(ctx);
	/* [ name require obj module:3 ] */
	duk_push_object(ctx);
	/* [ name require obj module:3 exports:4 ] */
	duk_dup(ctx, 4);
	/* [ name require obj module:3 exports:4 exports:5 ] */
	duk_put_prop_string(ctx, 3, "exports");
	/* [ name require obj module:3 exports:4 ] */
	duk_dup(ctx, 3);
	/* [ name require obj module:3 exports:4 module:5 ] */
	duk_put_prop_string(ctx, 2, name);
	/* [ name require obj module:3 exports:4 ] */
	duk_swap(ctx, 1, 2);
	/* [ name obj require module:3 exports:4 ] */
	duk_push_c_lightfunc(ctx, entry, 3, 3, 0);
	duk_replace(ctx, 1);
	/* [ name func(entry) require module:3 exports:4 ] */
	duk_dup(ctx, 3);
	duk_replace(ctx, 0);
	/* [ module func(entry) require module:3 exports:4 ] */
	duk_call(ctx, 3);
	/* [ module retval ] */
	duk_get_prop_string(ctx, 0, "exports");
	/* [ module retval exports ] */
	return 1;
}

DUK_INTERNAL duk_errcode_t dux_modules_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_c_function(ctx, modules_require, 1);
	/* [ ... require ] */
	duk_push_object(ctx);
	/* [ ... require obj ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_MODULES_GLOBALS);
	/* [ ... require ] */
	duk_put_global_string(ctx, "require");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

DUK_INTERNAL duk_errcode_t dux_modules_register(duk_context *ctx, const char *name, dux_module_entry entry)
{
	/* [ ... ] */
	duk_get_global_string(ctx, "require");
	/* [ ... require ] */
	if (!duk_get_prop_string(ctx, -1, DUX_IPK_MODULES_GLOBALS)) {
		/* [ ... require undefined ] */
		/* modules are not initialized */
		duk_pop_2(ctx);
		return DUK_ERR_ERROR;
	}
	/* [ ... require obj ] */
	if (duk_has_prop_string(ctx, -1, name)) {
		/* module is already registered */
		duk_pop_2(ctx);
		return DUK_ERR_ERROR;
	}
	duk_push_pointer(ctx, entry);
	/* [ ... require obj ptr ] */
	duk_put_prop_string(ctx, -2, name);
	/* [ ... require obj ] */
	duk_pop_2(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}
