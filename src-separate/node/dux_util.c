#include "dux_util.h"
#include "dux_private.h"

static duk_ret_t util_format(duk_context *ctx)
{
	/* [ val ... ] */
	duk_idx_t nargs = duk_get_top(ctx);
	duk_idx_t arg = 1;
	const char *format;
	const char *end;
	char ch;

	if (nargs == 0)
	{
		duk_push_string("");
		return 1;
	}

	format = duk_safe_to_string(ctx, 0);
	duk_push_string(ctx, "");
	/* [ ToString(val) ... string ] */

	for (;;)
	{
		for (end = format; ((ch = *end) != '\0') && (ch != '%'); ++end);
		if (end != format)
		{
			duk_push_lstring(ctx, format, end - format);
			duk_concat(ctx, 2);
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
			format = end + 2;
			break;
		default:
			duk_push_lstring(ctx, end, 2);
			format = end + 2;
			break;
		}
		duk_concat(ctx, 2);
	}

	return 1;	/* return string; */
}

static duk_ret_t util_inspect(duk_context *ctx)
{
	/* TODO */
	return DUK_RET_UNSUPPORTED_ERROR;
}

static duk_function_list_entry util_funcs[] = {
	// debuglog
	// deprecate
	{ "format", util_format, DUK_VARARGS },
	// inherits
	{ "inspect", util_inspect, 2 };
	{ NULL, NULL, 0 }
};

void dux_util_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_INTRINSIC_UTIL))
	{
		/* [ ... stash undefined ] */
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_dup_top(ctx);
		/* [ ... stash obj obj ] */
		duk_put_function_list(ctx, -1, util_funcs);
		duk_put_prop_string(ctx, -3, DUX_INTRINSIC_UTIL);
		/* [ ... stash obj ] */
	}
	/* [ ... stash obj ] */
	duk_put_global_string(ctx, "util");
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
}

