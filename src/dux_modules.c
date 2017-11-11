#include "dux_internal.h"

DUK_LOCAL const char DUX_IPK_MODULES[]          = DUX_IPK("Modules");
DUK_LOCAL const char DUX_KEY_MODULES_CACHE[]    = "cache";
DUK_LOCAL const char DUX_KEY_MODULES_ID[]       = "id";
DUK_LOCAL const char DUX_KEY_MODULES_PARENT[]   = "parent";
DUK_LOCAL const char DUX_KEY_MODULES_EXPORTS[]  = "exports";
DUK_LOCAL const char DUX_KEY_MODULES_FILENAME[] = "filename";
DUK_LOCAL const char DUX_KEY_MODULES_REQUIRE[]  = "require";
DUK_LOCAL const char DUX_KEY_MODULES_MODULE[]   = "module";

/**
 * @func modules_load_javascript
 * @brief JavaScript module loader
 */
DUK_LOCAL duk_ret_t modules_load_javascript(duk_context *ctx)
{
	/* [ any parent_module require cache:3 filename:4 source:5 ] */
	const char *filename = duk_get_string(ctx, 4);

	// Add wrapper
	duk_push_string(ctx, "function(__filename,__dirname,require,module,exports){");
	/* [ any parent_module require cache:3 filename:4 source:5 prologue:6 ] */
	duk_swap(ctx, 5, 6);
	/* [ any parent_module require cache:3 filename:4 prologue:5 source:6 ] */
	duk_push_string(ctx, "\n}");
	/* [ any parent_module require cache:3 filename:4 prologue:5 source:6 epilogue:7 ] */
	duk_concat(ctx, 3);
	/* [ any parent_module require cache:3 filename:4 wrapped:5 ] */
	duk_dup(ctx, 4);
	/* [ any parent_module require cache:3 filename:4 wrapped:5 filename:6 ] */

	// Compile
	duk_compile(ctx, DUK_COMPILE_FUNCTION);
	/* [ any parent_module require cache:3 filename:4 func:5 ] */

	// Create module instance
	duk_push_heap_stash(ctx);
	/* [ any parent_module require cache:3 filename:4 func:5 stash:6 ] */
	duk_get_prop_string(ctx, 6, DUX_IPK_MODULES);
	/* [ any parent_module require cache:3 filename:4 func:5 stash:6 Module:7 ] */
	duk_remove(ctx, 6);
	/* [ any parent_module require cache:3 filename:4 func:5 Module:6 ] */
	if (filename) {
		duk_dup(ctx, 4);
	} else {
		duk_push_string(ctx, "<input>");
	}
	duk_dup(ctx, 1);
	duk_dup(ctx, 4);
	duk_new(ctx, 3);
	/* [ any parent_module require cache:3 filename:4 func:5 module:6 ] */

	// Store cache
	if (filename) {
		duk_copy(ctx, 6, 1);
		/* [ any module require cache:3 filename:4 func:5 module:6 ] */
		duk_put_prop_string(ctx, 3, filename);
		/* [ any module require cache:3 filename:4 func:5 ] */
	} else {
		duk_replace(ctx, 1);
		/* [ any module require cache:3 filename:4 func:5 ] */
	}
	duk_replace(ctx, 3);
	/* [ any module require func:3 filename:4 ] */

	// Setup
#if !defined(DUX_OPT_NO_PATH)
	if (filename) {
		dux_path_dirname(ctx, filename);
	} else {
		duk_push_undefined(ctx);
	}
#else   /* DUX_OPT_NO_PATH */
	duk_push_null(ctx);
#endif  /* DUX_OPT_NO_PATH */
	/* [ any module require func:3 filename:4 dirname:5 ] */
	duk_dup(ctx, 2);
	/* [ any module require func:3 filename:4 dirname:5 require:6 ] */
	duk_dup(ctx, 1);
	/* [ any module require func:3 filename:4 dirname:5 require:6 module:7 ] */
	dux_bind_this(ctx);
	/* [ any module require func:3 filename:4 dirname:5 bound_require:6 ] */
	duk_dup(ctx, 1);
	/* [ any module require func:3 filename:4 dirname:5 bound_require:6 module:7 ] */
	duk_get_prop_string(ctx, 7, DUX_KEY_MODULES_EXPORTS);
	/* [ any module require func:3 filename:4 dirname:5 bound_require:6 module:7 exports:8 ] */

	// Load module
	duk_call(ctx, 5);
	/* [ any module require retval:3 ] */

	// Return exports with compaction
	duk_compact(ctx, 1);
	duk_get_prop_string(ctx, 1, DUX_KEY_MODULES_EXPORTS);
	/* [ any module require retval:3 exports:4 ] */
	duk_compact(ctx, 4);
	return 1;
}

/**
 * @func modules_not_found
 * @brief Raise error with module not found message
 */
DUK_LOCAL duk_ret_t modules_not_found(duk_context *ctx, const char *name)
{
	return duk_error(ctx, DUK_ERR_ERROR, "Cannot find module '%s'", name);
}

/**
 * @func modules_require_core
 * @brief require() implementation for built-in core modules
 */
DUK_LOCAL duk_ret_t modules_require_core(duk_context *ctx, const char *name)
{
	/* [ name parent_module require cache:3 ] */
	dux_module_entry entry;

	if (!duk_get_prop_string(ctx, 3, name)) {
		// Not found
		return modules_not_found(ctx, name);
	}
	/* [ name parent_module require cache:3 entry|module:4 ] */
	entry = (dux_module_entry)duk_get_pointer(ctx, 4);
	if (!entry) {
		// Already loaded (cached)
		/* [ name parent_module require cache:3 module:4 ] */
		duk_get_prop_string(ctx, 4, DUX_KEY_MODULES_EXPORTS);
		/* [ name parent_module require cache:3 module:4 exports:5 ] */
		return 1;
	}

	// Core module is not loaded

	// Create module instance
	duk_pop(ctx);
	/* [ name parent_module require cache:3 ] */
	duk_push_c_lightfunc(ctx, entry, 3, 3, 0);
	/* [ name parent_module require cache:3 func:4 ] */
	duk_dup(ctx, 2);
	/* [ name parent_module require cache:3 func:4 require:5 ] */
	duk_push_heap_stash(ctx);
	/* [ name parent_module require cache:3 func:4 require:5 stash:6 ] */
	duk_get_prop_string(ctx, 6, DUX_IPK_MODULES);
	duk_remove(ctx, 6);
	/* [ name parent_module require cache:3 func:4 require:5 Module:6 ] */
	duk_push_string(ctx, name);
	duk_push_undefined(ctx);	// Core modules does not retain relationship
	duk_push_null(ctx);
	duk_new(ctx, 3);
	/* [ name parent_module require cache:3 func:4 require:5 module:6 ] */
	duk_copy(ctx, 6, 1);
	/* [ name module require cache:3 func:4 require:5 module:6 ] */

	// Store cache
	duk_dup(ctx, 6);
	duk_put_prop_string(ctx, 3, name);
	/* [ name module require cache:3 func:4 require:5 module:6 ] */

	// Setup
	dux_bind_this(ctx);
	/* [ name module require cache:3 func:4 bound_require:5 ] */
	duk_dup(ctx, 1);
	/* [ name module require cache:3 func:4 bound_require:5 module:6 ] */
	duk_get_prop_string(ctx, 6, DUX_KEY_MODULES_EXPORTS);
	/* [ name module require cache:3 func:4 bound_require:5 module:6 exports:7 ] */

	// Load module
	duk_call(ctx, 3);
	/* [ name module require cache:3 retval:4 ] */

	// Return exports with compaction
	duk_compact(ctx, 1);
	duk_get_prop_string(ctx, 1, DUX_KEY_MODULES_EXPORTS);
	/* [ name module require cache:3 retval:4 exports:5 ] */
	duk_compact(ctx, 5);
	return 1;
}

/**
 * @func modules_require_file
 * @brief require() implementation for files
 */
DUK_LOCAL duk_ret_t modules_require_file(duk_context *ctx, const char *name, const char *filename)
{
	/* [ name parent_module require cache:3 filename:4 ] */

	// Try X as JavaScript
	if (dux_read_file(ctx, filename) == DUK_EXEC_SUCCESS) {
		/* [ name parent_module require cache:3 filename:4 source:5 ] */
		int path_len = strlen(filename);
		if ((path_len >= 5) && (strcmp(filename + path_len - 5, ".json") == 0)) {
			// Load JSON object
			duk_json_decode(ctx, 5);
			/* [ name parent_module require cache:3 filename:4 source:5 object:6 ] */
			return 1;
		}
	} else {
		/* [ name parent_module require cache:3 filename:4 undefined:5 ] */
		duk_pop(ctx);
		/* [ name parent_module require cache:3 filename:4 ] */
		duk_dup(ctx, 4);
		duk_push_string(ctx, ".js");
		duk_concat(ctx, 2);
		/* [ name parent_module require cache:3 filename:4 filename.js:5 ] */
		filename = duk_get_string(ctx, 5);

		// Try X.js as JavaScript
		if (dux_read_file(ctx, filename) == DUK_EXEC_SUCCESS) {
			/* [ name parent_module require cache:3 filename:4 filename.js:5 source:6 ] */
			duk_remove(ctx, 4);
			/* [ name parent_module require cache:3 filename.js:4 source:5 ] */
		} else {
			/* [ name parent_module require cache:3 filename:4 filename.js:5 undefined:6 ] */
			duk_pop(ctx);
			duk_push_string(ctx, "on");
			duk_concat(ctx, 2);
			/* [ name parent_module require cache:3 filename:4 filename.json:5 ] */
			filename = duk_get_string(ctx, 5);

			// Try X.json as JSON object
			if (dux_read_file(ctx, filename) == DUK_EXEC_SUCCESS) {
				/* [ name parent_module require cache:3 filename:4 filename.json:5 source:6 ] */
				// Load JSON object
				duk_json_decode(ctx, 6);
				/* [ name parent_module require cache:3 filename:4 filename.json:5 source:6 object:7 ] */
				return 1;
			} else {
				/* [ name parent_module require cache:3 filename:4 filename.json:5 err:6 ] */
				return modules_not_found(ctx, name);
			}
		}
	}

	/* [ name parent_module require cache:3 filename:4 source:5 ] */

	// Load JavaScript
	return modules_load_javascript(ctx);
}

DUK_LOCAL duk_ret_t modules_require(duk_context *ctx)
{
	/* [ name ] */
	const char *name;
	const char *filename;

	filename = name = duk_require_string(ctx, 0);
	duk_push_this(ctx);
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, 2, DUX_KEY_MODULES_CACHE);
	/* [ name parent_module require cache:3 ] */

	if (name[0] == '/') {
		// Absolute path
#if !defined(DUX_OPT_NO_PATH)
		filename = dux_path_normalize(ctx, name);
#else   /* DUX_OPT_NO_PATH */
		duk_dup(ctx, 0);
#endif  /* DUX_OPT_NO_PATH */
	} else if (name[0] == '.') {
		// Relative path
#if !defined(DUX_OPT_NO_PATH)
		const char *parent;
		duk_get_prop_string(ctx, 1, DUX_KEY_MODULES_FILENAME);
		/* [ name parent_module require cache:3 parent_filename:4 ] */
		parent = duk_get_string(ctx, 4);
		if (parent) {
			dux_path_dirname(ctx, parent);
			duk_push_string(ctx, "/");
			duk_dup(ctx, 0);
			duk_concat(ctx, 3);
			/* [ name parent_module require cache:3 parent_filename:4 joined:5 ] */
			filename = dux_path_normalize(ctx, duk_require_string(ctx, 5));
			/* [ name parent_module require cache:3 parent_filename:4 joined:5 filename:6 ] */
			duk_replace(ctx, 4);
			duk_pop(ctx);
			/* [ name parent_module require cache:3 filename:4 ] */
		} else {
			duk_copy(ctx, 0, 4);
		}
#else   /* DUX_OPT_NO_PATH */
		duk_dup(ctx, 0);
#endif  /* DUX_OPT_NO_PATH */
	} else {
		// Load from core modules
		// FIXME: installed packages
		return modules_require_core(ctx, name);
	}
	/* [ name parent_module require cache:3 filename:4 ] */

	// Not core modules nor installed packages

	if (duk_get_prop_string(ctx, 3, filename)) {
		// Cached
		/* [ name parent_module require cache:3 filename:4 module:5 ] */
		duk_get_prop_string(ctx, 5, DUX_KEY_MODULES_EXPORTS);
		/* [ name parent_module require cache:3 filename:4 module:5 exports:6 ] */
		return 1;
	}
	/* [ name parent_module require cache:3 filename:4 undefined:5 ] */

	// No cache. Try load as a new module

	duk_pop(ctx);
	/* [ name parent_module require cache:3 filename:4 ] */
	return modules_require_file(ctx, name, filename);
}

/**
 * @func modules_constructor
 * @brief Constructor for Module object
 */
DUK_INTERNAL duk_ret_t modules_constructor(duk_context *ctx)
{
	/* [ id parent filename ] */
	if (!duk_is_constructor_call(ctx)) {
		return DUK_RET_TYPE_ERROR;
	}
	duk_push_this(ctx);
	/* [ id parent filename module:3 ] */

	// Set properties
	duk_push_string(ctx, DUX_KEY_MODULES_ID);
	duk_dup(ctx, 0);
	duk_def_prop(ctx, 3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
	duk_push_string(ctx, DUX_KEY_MODULES_PARENT);
	duk_dup(ctx, 1);
	duk_def_prop(ctx, 3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
	duk_push_string(ctx, DUX_KEY_MODULES_FILENAME);
	duk_dup(ctx, 2);
	duk_def_prop(ctx, 3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);

	// Create empty exports
	duk_push_object(ctx);
	duk_put_prop_string(ctx, 3, DUX_KEY_MODULES_EXPORTS);
	return 0;
}

DUK_INTERNAL duk_function_list_entry modules_proto_funcs[] = {
	{ DUX_KEY_MODULES_REQUIRE, modules_require, 1 },
	{ NULL, NULL, 0 }
};

/**
 * @func dux_modules_init
 * @brief Initialize modules
 */
DUK_INTERNAL duk_errcode_t dux_modules_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	dux_push_named_c_constructor(ctx, "Module", modules_constructor, 3, NULL, modules_proto_funcs, NULL, NULL);
	/* [ ... stash Module ] */
	duk_get_prop_string(ctx, -1, "prototype");
	/* [ ... stash Module module_proto ] */
	duk_get_prop_string(ctx, -1, DUX_KEY_MODULES_REQUIRE);
	/* [ ... stash Module module_proto require ] */
	duk_push_object(ctx);
	/* [ ... stash Module module_proto require new_proto ] */
	duk_get_prototype(ctx, -2);
	/* [ ... stash Module module_proto require new_proto proto ] */
	duk_set_prototype(ctx, -2);
	/* [ ... stash Module module_proto require new_proto ] */
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, DUX_KEY_MODULES_CACHE);
	duk_set_prototype(ctx, -2);
	/* [ ... stash Module module_proto require ] */
	duk_dup(ctx, -3);
	/* [ ... stash Module module_proto require Module ] */
	duk_push_string(ctx, "<repl>"); // id
	duk_push_undefined(ctx);        // parent
	duk_push_null(ctx);             // filename
	duk_new(ctx, 3);
	/* [ ... stash Module module_proto require module ] */
	duk_dup(ctx, -1);
	duk_put_global_string(ctx, DUX_KEY_MODULES_MODULE);
	dux_bind_this(ctx);
	/* [ ... stash Module module_proto bound_require ] */
	duk_put_global_string(ctx, DUX_KEY_MODULES_REQUIRE);
	/* [ ... stash Module module_proto ] */
	duk_pop(ctx);
	/* [ ... stash Module ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_MODULES);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/**
 * @func dux_modules_register
 * @brief Register core module entry
 */
DUK_INTERNAL duk_errcode_t dux_modules_register(duk_context *ctx, const char *name, dux_module_entry entry)
{
	/* [ ... ] */
	duk_get_global_string(ctx, DUX_KEY_MODULES_REQUIRE);
	/* [ ... global_require ] */
	duk_get_prop_string(ctx, -1, DUX_KEY_MODULES_CACHE);
	/* [ ... global_require cache ] */
	duk_push_pointer(ctx, entry);
	/* [ ... global_require cache ptr ] */
	duk_put_prop_string(ctx, -2, name);
	/* [ ... global_require cache ] */
	duk_pop_2(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/**
 * @func dux_eval_module_raw
 * @brief Evaluate file/source as a module
 */
DUK_EXTERNAL duk_ret_t dux_eval_module_raw(duk_context *ctx, const void *data, duk_size_t len, duk_int_t flags)
{
	/* [ ... ] */
	duk_push_c_lightfunc(ctx, modules_load_javascript, 6, 6, 0);
	duk_push_undefined(ctx);
	duk_get_global_string(ctx, DUX_KEY_MODULES_MODULE);
	duk_get_prop_string(ctx, -1, DUX_KEY_MODULES_REQUIRE);
	duk_get_prop_string(ctx, -1, DUX_KEY_MODULES_CACHE);
	/* [ ... loader undefined global_module global_require cache ] */
	if (flags & DUK_COMPILE_NOSOURCE) {
		// Load from file
		const char *filename = dux_path_normalize(ctx, (const char *)data);
		/* [ ... loader undefined global_module global_require cache filename ] */
		if (dux_read_file(ctx, filename) != DUK_EXEC_SUCCESS) {
			/* [ ... loader undefined global_module global_require cache filename err ] */
			if (!(flags & DUK_COMPILE_SAFE)) {
				return duk_throw(ctx);
			}
			if (flags & DUK_COMPILE_NORESULT) {
				duk_pop_n(ctx, 7);
				/* [ ... ] */
			} else {
				duk_replace(ctx, -7);
				duk_pop_n(ctx, 5);
				/* [ ... err ] */
			}
			return DUK_EXEC_ERROR;
		}
		/* [ ... loader undefined global_module global_require cache filename source ] */
	} else {
		// Load from source
		duk_push_undefined(ctx);
		if (flags & DUK_COMPILE_STRLEN) {
			duk_push_string(ctx, (const char *)data);
		} else {
			duk_push_lstring(ctx, (const char *)data, len);
		}
		/* [ ... loader undefined global_module global_require cache undefined source ] */
	}

	/* [ ... loader any module require cache filename|undefined source ] */
	if (duk_pcall(ctx, 6) != DUK_EXEC_SUCCESS) {
		/* [ ... err ] */
		if (!(flags & DUK_COMPILE_SAFE)) {
			return duk_throw(ctx);
		}
		if (flags & DUK_COMPILE_NORESULT) {
			duk_pop(ctx);
			/* [ ... ] */
		}
		return DUK_EXEC_ERROR;
	}
	/* [ ... retval ] */
	if (flags & DUK_COMPILE_NORESULT) {
		duk_pop(ctx);
		/* [ ... ] */
	}
	return DUK_EXEC_SUCCESS;
}
