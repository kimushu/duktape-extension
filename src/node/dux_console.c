#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_CONSOLE)
#if defined(DUX_OPT_NO_PROCESS)
# error "DUX_OPT_NO_PROCESS must be used with DUX_OPT_NO_CONSOLE"
#endif
#if defined(DUX_OPT_NO_UTIL)
# error "DUX_OPT_NO_UTIL must be used with DUX_OPT_NO_CONSOLE"
#endif
#include "../dux_internal.h"
#include <stdio.h>
#include <unistd.h>

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_CONSOLE[]      = DUX_IPK("Console");
DUK_LOCAL const char DUX_IPK_CONSOLE_OUT[]  = DUX_IPK("coOut");
DUK_LOCAL const char DUX_IPK_CONSOLE_ERR[]  = DUX_IPK("coErr");
DUK_LOCAL const char DUX_CONSOLE_NEWLINE[]  = "\n";

/*
 * Common implementation of console functions
 */
DUK_LOCAL duk_ret_t console_print(duk_context *ctx, const char *ipk)
{
	duk_ret_t result;
	int fd;
	FILE *fp;

	/* [ ... ] */
	result = dux_util_format(ctx);
	if (result != 1)
	{
		return result;
	}

	/* [ ... string ] */
	duk_swap_top(ctx, 0);
	duk_set_top(ctx, 1);
	/* [ string ] */
	duk_push_this(ctx);
	/* [ string this ] */
	duk_get_prop_string(ctx, 1, ipk);
	/* [ string this pointer|stream ] */

	if ((fd = duk_get_int_default(ctx, 2, -1)) >= 0) {
		/* [ string this int ] */
		int len;
		const char *buf = duk_safe_to_lstring(ctx, 0, (duk_size_t *)&len);
		while (len > 0) {
			int written = write(fd, buf, len);
			if (written <= 0) {
				break;
			}
			buf += written;
			len -= written;
		}
		write(fd, DUX_CONSOLE_NEWLINE, 1);
	} else if ((fp = (FILE *)duk_get_pointer(ctx, 2)) != NULL) {
		/* [ string this pointer ] */
		fputs(duk_safe_to_string(ctx, 0), fp);
		fputs(DUX_CONSOLE_NEWLINE, fp);
	} else {
		/* [ string this stream ] */
		duk_push_string(ctx, "write");
		duk_dup(ctx, 0);
		/* [ string this stream "write":3 string:4 ] */
		duk_call_prop(ctx, 2, 1);
		duk_pop(ctx);
		/* [ string this stream ] */
		duk_push_string(ctx, "write");
		duk_push_string(ctx, DUX_CONSOLE_NEWLINE);
		/* [ string this stream "write":3 string:4 ] */
		duk_call_prop(ctx, 2, 1);
	}

	return 0; /* return undefined */
}

/*
 * Constructor of Console class
 */
DUK_LOCAL duk_ret_t console_constructor(duk_context *ctx)
{
	/* [ stdout stderr ] */
	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

	duk_push_this(ctx);
	duk_insert(ctx, 0);
	/* [ this stdout stderr ] */

	// Store stderr
	if (duk_is_null_or_undefined(ctx, 2)) {
		duk_pop(ctx);
		duk_dup(ctx, 1);
	}

	if (duk_is_number(ctx, 2) || duk_is_pointer(ctx, 2) || duk_is_object(ctx, 2)) {
		duk_put_prop_string(ctx, 0, DUX_IPK_CONSOLE_ERR);
	} else {
		return DUK_RET_TYPE_ERROR;
	}
	/* [ this stdout ] */

	// Store stdout
	if (duk_is_number(ctx, 1) || duk_is_pointer(ctx, 1) || duk_is_object(ctx, 1)) {
		duk_put_prop_string(ctx, 0, DUX_IPK_CONSOLE_OUT);
	} else {
		return DUK_RET_TYPE_ERROR;
	}
	/* [ this ] */

	return 0; /* return this */
}

/*
 * Entry of Console.prototype.assert()
 */
DUK_LOCAL duk_ret_t console_proto_assert(duk_context *ctx)
{
	if (duk_to_boolean(ctx, 0))
	{
		return 0; /* return undefined */
	}
	duk_remove(ctx, 0);
	dux_util_format(ctx);
	(void)duk_generic_error(ctx, "Assertion failed: %s", duk_safe_to_string(ctx, -1));
	/* unreachable */
	return 0;
}

/*
 * Entry of Console.prototype.error() / Console.prototype.warn()
 */
DUK_LOCAL duk_ret_t console_proto_error(duk_context *ctx)
{
	return console_print(ctx, DUX_IPK_CONSOLE_ERR);
}

/*
 * Entry of Console.prototype.log() / Console.prototype.info()
 */
DUK_LOCAL duk_ret_t console_proto_log(duk_context *ctx)
{
	return console_print(ctx, DUX_IPK_CONSOLE_OUT);
}

/*
 * Getter of global.console
 */
DUK_LOCAL duk_ret_t global_console_getter(duk_context *ctx)
{
	/* [  ] */
	duk_get_global_string(ctx, "require");
	duk_push_string(ctx, "console");
	duk_call(ctx, 1);
	/* [ console ] */
	duk_push_global_object(ctx);
	/* [ console global ] */
	duk_dup(ctx, 0);
	duk_push_string(ctx, "console");
	/* [ console global "console" console ] */
	duk_def_prop(ctx, 1, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE);
	/* [ console global ] */
	duk_pop(ctx);
	/* [ console ] */
	return 1;
}

/*
 * List of console functions
 */
DUK_LOCAL duk_function_list_entry console_proto_funcs[] = {
	{ "assert", console_proto_assert,   DUK_VARARGS },
	{ "error",  console_proto_error,    DUK_VARARGS },
	{ "info",   console_proto_log,      DUK_VARARGS },
	{ "log",    console_proto_log,      DUK_VARARGS },
	{ "warn",   console_proto_error,    DUK_VARARGS },
	{ NULL, NULL, 0 }
};

/*
 * Entry of Console module
 */
DUK_INTERNAL duk_errcode_t console_entry(duk_context *ctx)
{
    /* [ require module exports ] */
	dux_push_named_c_constructor(ctx, "Console",
			console_constructor, 2,
			NULL, console_proto_funcs,
			NULL, NULL);
	/* [ require module exports constructor:3 ] */
	duk_dup(ctx, 3);
	/* [ require module exports constructor:3 constructor:4 ] */
	duk_push_int(ctx, STDOUT_FILENO);
	duk_push_int(ctx, STDERR_FILENO);
	duk_new(ctx, 2);
	/* [ require module exports constructor:3 console:4 ] */
	duk_swap(ctx, 3, 4);
	/* [ require module exports console:3 constructor:4 ] */
	duk_put_prop_string(ctx, 3, "Console");
	/* [ require module exports console:3 ] */
	duk_put_prop_string(ctx, 1, "exports");
	/* [ require module exports ] */
	return DUK_ERR_NONE;
}

DUK_INTERNAL duk_errcode_t dux_console_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_global_object(ctx);
	/* [ ... global ] */
	duk_push_string(ctx, "console");
	duk_push_c_function(ctx, global_console_getter, 0);
	/* [ ... global "console" getter ] */
	duk_def_prop(ctx, -3, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_GETTER);
	/* [ ... global ] */
	duk_pop(ctx);
	/* [ ... ] */
	return dux_modules_register(ctx, "console", console_entry);
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_CONSOLE */
