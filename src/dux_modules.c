#include "dux_internal.h"

DUK_LOCAL const char DUX_KEY_MODULES_CACHE[] = "cache";

DUK_LOCAL duk_ret_t modules_not_found(duk_context *ctx, const char *name)
{
	return duk_error(ctx, DUK_ERR_ERROR, "Cannot find module '%s'", name);
}

DUK_LOCAL void modules_construct_for_core(duk_context *ctx, const char *id)
{
	/* [ ... ] */
	duk_idx_t obj_idx = duk_push_object(ctx);
	/* [ ... module ] */
	duk_push_string(ctx, id);
	duk_put_prop_string(ctx, obj_idx, "id");
	duk_push_undefined(ctx);
	duk_put_prop_string(ctx, obj_idx, "parent");
	duk_push_null(ctx);
	duk_put_prop_string(ctx, obj_idx, "filename");
	duk_dup(ctx, duk_push_object(ctx));
	duk_put_prop_string(ctx, obj_idx, "exports");
	/* [ ... module exports ] */
}

DUK_LOCAL duk_ret_t modules_load_core_module(duk_context *ctx, const char *name)
{
	dux_module_entry entry;

	duk_set_top(ctx, 0);
	/* [  ] */
	duk_push_current_function(ctx);
	/* [ require ] */
	duk_get_prop_string(ctx, 0, DUX_KEY_MODULES_CACHE);
	/* [ require cache ] */
	if (!duk_get_prop_string(ctx, 1, name)) {
		// Not found
		return modules_not_found(ctx, name);
	}
	/* [ require cache module|entry ] */
	entry = (dux_module_entry)duk_get_pointer(ctx, 2);
	if (!entry) {
		// Already loaded (cached)
		/* [ require cache module ] */
		duk_get_prop_string(ctx, 2, "exports");
		/* [ require cache module exports ] */
		return 1;
	}

	// Core module is not loaded
	duk_pop(ctx);
	/* [ require cache ] */
	duk_push_c_lightfunc(ctx, entry, 3, 3, 0);
	/* [ require cache func(entry) ] */
	duk_dup(ctx, 0);
	/* [ require cache func(entry) require:3 ] */
	modules_construct_for_core(ctx, name);
	/* [ require cache func(entry) require:3 module:4 exports:5 ] */
	duk_copy(ctx, 4, 0);
	/* [ module cache func(entry) require:3 module:4 exports:5 ] */
	duk_dup(ctx, 4);
	duk_put_prop_string(ctx, 1, name);
	/* [ module cache func(entry) require:3 module:4 exports:5 ] */
	duk_call(ctx, 3);
	/* [ module cache retval ] */
	duk_get_prop_string(ctx, 0, "exports");
	/* [ module cache retval exports ] */
	return 1;
}

DUK_LOCAL duk_ret_t modules_require(duk_context *ctx)
{
	const char *name;
	const char *full_path;
	dux_module_entry entry;

	/* [ name ] */
	name = duk_require_string(ctx, 0);
	duk_push_this(ctx);
	/* [ name parent_module ] */

	if ((name[0] != '/') && (name[0] != '.')) {
		// Load core module
		return modules_load_core_module(ctx, name);
	}

#if !defined(DUX_OPT_NO_PATH)
	if (name[0] == '/') {
		// Load from full path
		full_path = dux_path_normalize(ctx, name);
		/* [ name parent_module full_path ] */
	} else {
		// Load from relative path
		const char *dir;
		duk_get_prop_string(ctx, 1, "filename");
		/* [ name parent_module filename ] */
		full_path = duk_get_string(ctx, 2);
		if (!full_path) {
			return modules_not_found(ctx, name);
		}
		dir = dux_path_dirname(ctx, full_path);
		/* [ name parent_module filename dirname ] */
		duk_push_string(ctx, "/");
		duk_dup(ctx, 0);
		duk_concat(ctx, 3);
		/* [ name parent_module filename concat_path ] */
		full_path = dux_path_normalize(ctx, duk_get_string(ctx, 3));
		/* [ name parent_module filename concat_path full_path ] */
		duk_replace(ctx, 2);
		/* [ name parent_module full_path concat_path ] */
		duk_pop(ctx);
		/* [ name parent_module full_path ] */
	}
#else   /* DUX_OPT_NO_PATH */
	duk_dup(ctx, 0);
#endif  /* DUX_OPT_NO_PATH */

	/* [ name parent_module full_path ] */
	// Try X as JavaScript
	if (dux_read_file(ctx, full_path) == DUK_EXEC_SUCCESS) {
		/* [ name parent_module full_path source:3 ] */
		int path_len = strlen(full_path);
		if ((path_len >= 5) && (strcmp(full_path + path_len - 5, ".json") == 0)) {
			// Load JSON object
			duk_json_decode(ctx, 3);
			return 1;
		}
	} else {
		/* [ name parent_module full_path err:3 ] */
		duk_pop(ctx);
		/* [ name parent_module full_path ] */
		duk_dup(ctx, 2);
		duk_push_string(ctx, ".js");
		duk_concat(ctx, 2);
		/* [ name parent_module full_path full_path.js:3 ] */
		full_path = duk_get_string(ctx, 3);
		// Try X.js as JavaScript
		if (dux_read_file(ctx, full_path) == DUK_EXEC_SUCCESS) {
			/* [ name parent_module full_path full_path.js:3 source:4 ] */
			duk_remove(ctx, 2);
			/* [ name parent_module full_path.js source:3 ] */
		} else {
			/* [ name parent_module full_path full_path.js:3 err:4 ] */
			duk_pop(ctx);
			duk_push_string(ctx, "on");
			duk_concat(ctx, 2);
			full_path = duk_get_string(ctx, 3);
			/* [ name parent_module full_path full_path.json:3 ] */
			// Try X.json as JSON object
			if (dux_read_file(ctx, full_path) == DUK_EXEC_SUCCESS) {
				/* [ name parent_module full_path full_path.json:3 json:4 ] */
				// Load JSON object
				duk_json_decode(ctx, 4);
				return 1;
			} else {
				/* [ name parent_module full_path full_path.json:3 err:4 ] */
				return modules_not_found(ctx, name);
			}
		}
	}

	/* [ name parent_module full_path source ] */

	// Load JavaScript
	duk_replace(ctx, 0);
	/* [ source parent_module full_path ] */
	duk_push_string(ctx, "function(__filename,__dirname,exports,require,module){");
	duk_dup(ctx, 0);
	duk_push_string(ctx, "\n}");
	duk_concat(ctx, 3);
	/* [ source parent_module full_path combined_source ] */
	duk_dup(ctx, 2);
	/* [ source parent_module full_path combined_source full_path ] */
	duk_compile(ctx, DUK_COMPILE_FUNCTION);
	/* [ source parent_module full_path function:3 ] */
	duk_swap(ctx, 1, 3);
	/* [ source function full_path parent_module:3 ] */
	duk_push_object(ctx);
	duk_replace(ctx, 0);
	/* [ module function full_path parent_module:3 ] */
	duk_put_prop_string(ctx, 0, "parent");
	/* [ module function full_path ] */
#if !defined(DUX_OPT_NO_PATH)
	dux_path_dirname(ctx, full_path);
#else   /* DUX_OPT_NO_PATH */
	duk_push_null(ctx);
#endif  /* DUX_OPT_NO_PATH */
	/* [ module function full_path dirname:3 ] */
	duk_dup(ctx, 2);
	duk_put_prop_string(ctx, 0, "id");
	duk_dup(ctx, 2);
	duk_put_prop_string(ctx, 0, "filename");
	duk_push_object(ctx);
	/* [ module function full_path dirname:3 exports:4 ] */
	duk_dup(ctx, 4);
	duk_put_prop_string(ctx, 0, "exports");
	/* [ module function full_path dirname:3 exports:4 ] */
	duk_push_current_function(ctx);
	/* [ module function full_path dirname:3 exports:4 require:5 ] */
	duk_get_prop_string(ctx, 5, DUX_KEY_MODULES_CACHE);
	/* [ module function full_path dirname:3 exports:4 require:5 cache:6 ] */
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, 6, full_path);
	duk_pop(ctx);
	/* [ module function full_path dirname:3 exports:4 require:5 ] */
	duk_dup(ctx, 0);
	dux_bind_this_arguments(ctx, 0);
	/* [ module function full_path dirname:3 exports:4 bound_require:5 ] */
	duk_dup(ctx, 0);
	/* [ module function full_path dirname:3 exports:4 bound_require:5 module:6 ] */
	duk_call(ctx, 5);
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
	duk_dup_top(ctx);
	/* [ ... require cache cache ] */
	duk_put_prop_string(ctx, -3, DUX_KEY_MODULES_CACHE);
	/* [ ... require cache ] */
	modules_construct_for_core(ctx, "<repl>");
	/* [ ... require cache module exports ] */
	duk_copy(ctx, -2, -1);
	/* [ ... require cache module module ] */
	duk_put_global_string(ctx, "module");
	/* [ ... require cache module ] */
	duk_swap(ctx, -3, -2);
	/* [ ... cache require module ] */
	dux_bind_this_arguments(ctx, 0);
	/* [ ... cache bound_require ] */
	duk_swap(ctx, -2, -1);
	/* [ ... bound_require cache ] */
	duk_put_prop_string(ctx, -2, DUX_KEY_MODULES_CACHE);
	/* [ ... bound_require ] */
	duk_put_global_string(ctx, "require");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

DUK_INTERNAL duk_errcode_t dux_modules_register(duk_context *ctx, const char *name, dux_module_entry entry)
{
	/* [ ... ] */
	duk_get_global_string(ctx, "require");
	/* [ ... require ] */
	if (!duk_get_prop_string(ctx, -1, DUX_KEY_MODULES_CACHE)) {
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
