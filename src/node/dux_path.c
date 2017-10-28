#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_PATH)
#include "../dux_internal.h"
#include <string.h>

DUK_LOCAL const char path_delimiter[] = { DUX_PATH_DELIMITER, '\0' };
DUK_LOCAL const char path_separator[] = { DUX_PATH_SEPARATOR, '\0' };

DUK_INTERNAL const char *dux_path_basename(duk_context *ctx, const char *path, const char *ext)
{
	const char *start = strrchr((char *)path, DUX_PATH_SEPARATOR);
	start = (start) ? (start + 1) : path;
	if (ext) {
		const char *end = start + strlen(start) - strlen(ext);
		if (strcmp(end, ext) == 0) {
			return duk_push_lstring(ctx, start, end - start);
		}
	}
	return duk_push_string(ctx, start);
}

DUK_INTERNAL const char *dux_path_dirname(duk_context *ctx, const char *path)
{
	char *sep = strrchr(path, DUX_PATH_SEPARATOR);
	if (!sep) {
		return duk_push_string(ctx, ".");
	}
	if (sep == path) {
		return duk_push_string(ctx, path_separator);
	}
	return duk_push_lstring(ctx, path, sep - path);
}

DUK_INTERNAL const char *dux_path_normalize(duk_context *ctx, const char *path)
{
	int segments = 0;
	int depth = 0;
	int has_root = 0;
	/* [ ... ] */
	duk_push_string(ctx, path_separator);
	/* [ ... <sep> ] */
	if (path[0] == DUX_PATH_SEPARATOR) {
		duk_push_string(ctx, "");
		++segments;
		has_root = 1;
		++path;
	}
	for (;;) {
		const char *sep = strchr(path, DUX_PATH_SEPARATOR);
		int len = (sep ? (sep - path) : strlen(path));
		switch (path[0]) {
		case '\0':
			goto push;
		case DUX_PATH_SEPARATOR:
			goto next;
		case '.':
			if (len == 1) {
				goto next;
			}
			if ((len == 2) && (path[1] == '.')) {
				if (depth > 0) {
					--depth;
					duk_pop(ctx);
					--segments;
				} else if (!has_root) {
					goto push;
				}
				goto next;
			}
			break;
		}
		++depth;
push:
		duk_push_lstring(ctx, path, len);
		++segments;
next:
		if (!sep) {
			break;
		}
		path = sep + 1;
	}

	duk_join(ctx, segments);
	/* [ ... path ] */
	return duk_get_string(ctx, -1);
}

DUK_INTERNAL const char *dux_path_relative(duk_context *ctx, const char *from, const char *to)
{
	// FIXME
	(void)duk_error(ctx, DUK_RET_ERROR, "Not implemented");
	return NULL;
}

DUK_LOCAL duk_ret_t path_basename(duk_context *ctx)
{
	/* [ path ext/null ] */
	dux_path_basename(
		ctx,
		duk_require_string(ctx, 0),
		duk_is_null_or_undefined(ctx, 1) ? NULL : duk_require_string(ctx, 1)
	);
	/* [ path ext/null basename ] */
	return 1;
}

DUK_LOCAL duk_ret_t path_dirname(duk_context *ctx)
{
	/* [ path ] */
	(void)dux_path_dirname(ctx, duk_require_string(ctx, 0));
	/* [ path dirname ] */
	return 1;
}

DUK_LOCAL duk_ret_t path_normalize(duk_context *ctx)
{
	/* [ path ] */
	(void)dux_path_normalize(ctx, duk_require_string(ctx, 0));
	/* [ path normalized ] */
	return 1;
}

DUK_LOCAL duk_ret_t path_relative(duk_context *ctx)
{
	/* [ from to ] */
	(void)dux_path_relative(ctx, duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	/* [ from to relative ] */
	return 1;
}

/*
 * List of methods for path object
 */
DUK_LOCAL duk_function_list_entry path_funcs[] = {
	{ "basename", path_basename, 2 },
	{ "dirname", path_dirname, 1 },
	{ "normalize", path_normalize, 1 },
	{ "relative", path_relative, 2 },
	{ NULL, NULL, 0 }
};

DUK_LOCAL duk_errcode_t path_entry(duk_context *ctx)
{
	/* [ require module exports ] */
	duk_put_function_list(ctx, 2, path_funcs);

	duk_push_string(ctx, "delimiter");
	duk_push_string(ctx, path_delimiter);
	duk_def_prop(ctx, 2, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);

	duk_push_string(ctx, "posix");
	duk_dup(ctx, 2);
	duk_def_prop(ctx, 2, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
	
	duk_push_string(ctx, "sep");
	duk_push_string(ctx, path_separator);
	duk_def_prop(ctx, 2, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);

	return DUK_ERR_NONE;
}

DUK_INTERNAL duk_errcode_t dux_path_init(duk_context *ctx)
{
	return dux_modules_register(ctx, "path", path_entry);
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_PATH */
