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

static duk_ret_t console_proto_assert(duk_context *ctx)
{
	//
}

static duk_ret_t console_proto_dir(duk_context *ctx)
{
	//
}

static duk_ret_t console_proto_error(duk_context *ctx)
{
	return console_proto_output(ctx);
}

static duk_ret_t console_proto_info(duk_context *ctx)
{
	return console_proto_output(ctx);
}

static duk_ret_t console_proto_log(duk_context *ctx)
{
	return console_proto_output(ctx);
}

static duk_ret_t console_proto_warn(duk_context *ctx)
{
	return console_proto_output(ctx);
}

static duk_function_list_entry console_proto_funcs[] = {
	{ "assert",  console_proto_assert,  DUK_VARARGS },
	{ "dir",     console_proto_dir,     DUK_VARARGS },
	{ "error",   console_proto_error,   DUK_VARARGS },
	{ "info",    console_proto_info,    DUK_VARARGS },
	{ "log",     console_proto_log,     DUK_VARARGS },
	{ "time",    console_proto_time,    1 },
	{ "timeEnd", console_proto_timeend, 1 },
	{ "trace",   console_proto_trace,   DUK_VARARGS },
	{ "warn",    console_proto_warn,    DUK_VARARGS },
	{ NULL, NULL, 0 }
};

static duk_ret_t global_console_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	duk_push_heap_stash(ctx);
	/* [ this stash ] */
	duk_get_prop_string(ctx, 1, DUX_INTRINSIC_PROCESS);
	duk_push_string(ctx, "console");
	duk_get_prop_string(ctx, 1, DUX_INTRINSIC_CONSOLE);
	/* [ this stash process "console" constructor ] */
	duk_get_prop_string(ctx, 2, "stdout");
	duk_get_prop_string(ctx, 2, "stderr");
	/* [ this stash process "console" constructor stdout stderr ] */
	duk_new(ctx, 2);
	/* [ this stash process "console" obj ] */
	duk_copy(ctx, 4, 2);
	/* [ this stash obj "console" obj ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE);
	/* [ this stash obj ] */
	return 1;	/* return obj; */
}

void dux_console_init(duk_context *ctx)
{
	/* [ ... ] */

	/* Console class */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_INTRINSIC_CONSOLE))
	{
		/* [ ... stash undefined ] */
		duk_pop(ctx);
		dux_push_named_c_constructor(ctx, "Console",
				console_constructor, 2,
				NULL, console_proto_funcs,
				NULL, NULL);
		duk_dup_top(ctx);
		/* [ ... stash constructor constructor ] */
		duk_put_prop_string(ctx, -3, DUX_INTRINSIC_CONSOLE);
		/* [ ... stash constructor ] */
	}
	/* [ ... stash constructor ] */
	duk_put_global_string(ctx, "Console");
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */

	/* console property */
	duk_push_global_object(ctx);
	duk_push_string(ctx, "console");
	duk_push_c_function(ctx, global_console_getter, 0);
	/* [ ... global "console" getter ] */
	duk_def_prop(ctx, -3, DUK_DEFPROP_FORCE |
			DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_HAVE_GETTER);
	/* [ ... global ] */
	duk_pop(ctx);
	/* [ ... ] */
}

