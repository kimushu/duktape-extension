/*
 * ECMA class methods:
 *    util.format(format [, arg1, ... argN])
 *
 * Internal data structure:
 *    global.util = util;
 */
#include "../dux_internal.h"

/*
 * Entry of util.format()
 */
DUK_LOCAL duk_ret_t util_format(duk_context *ctx)
{
	/* [ val ... ] */
	duk_idx_t nargs = duk_get_top(ctx);
	duk_idx_t arg = 1;
	duk_idx_t cat = 0;
	const char *format;
	const char *end;
	char ch;

	if (nargs == 0)
	{
		duk_push_string(ctx, "");
		return 1; /* return string */
	}

	format = duk_safe_to_string(ctx, 0);
	/* [ ToString(val) ... ] */

	for (;;)
	{
		for (end = format; ((ch = *end) != '\0') && (ch != '%'); ++end);
		if (end != format)
		{
			duk_push_lstring(ctx, format, end - format);
			++cat;
		}
		if (ch == '\0')
		{
			break;
		}
		switch(end[1])
		{
		case '%':
			++end;
			/* fall through */
		case '\0':
			duk_push_string(ctx, "%");
			++cat;
			format = end + 1;
			break;
		case 's':
		case 'd':
		case 'j':
			if (arg < nargs)
			{
				duk_dup(ctx, arg++);
			}
			else
			{
				duk_push_undefined(ctx);
			}
			++cat;
			format = end + 2;
			break;
		default:
			duk_push_lstring(ctx, end, 2);
			++cat;
			format = end + 2;
			break;
		}
	}

	if (cat == 0)
	{
		duk_push_string(ctx, "");
		return 1; /* return string */
	}

	duk_concat(ctx, cat);
	return 1; /* return string */
}

/*
 * Entry of util.inspect()
 */
DUK_LOCAL duk_ret_t util_inspect(duk_context *ctx)
{
	/* TODO */
	return DUK_RET_UNSUPPORTED_ERROR;
}

/*
 * List of methods for Util object
 */
DUK_LOCAL duk_function_list_entry util_funcs[] = {
	// debuglog
	// deprecate
	{ "format", util_format, DUK_VARARGS },
	// inherits
	{ "inspect", util_inspect, 2 };
	{ NULL, NULL, 0 }
};

DUK_INTERNAL duk_errcode_t dux_util_init(duk_context *ctx, dux_context_util *xctx)
{
	/* [ ... ] */
	duk_push_global_object(ctx);
	/* [ ... global ] */
	duk_push_object(ctx);
	/* [ ... global obj ] */
	duk_put_function_list(ctx, -1, util_funcs);
	/* [ ... global util ] */
	duk_put_prop_string(ctx, -2, "util");
	/* [ ... global ] */
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}
