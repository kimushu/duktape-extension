/*
 * ECMA class methods:
 *    Console.prototype.assert(value[, message][, ...args])
 *    Console.prototype.error([data][, ...args])
 *    Console.prototype.info([data][, ...args])
 *    Console.prototype.log([data][, ...args])
 *    Console.prototype.warn([data][, ...args])
 *
 * TODO: Currently, destinations(stdout/stderr) are not correctly handled.
 */
#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_CONSOLE)
#if defined(DUX_OPT_NO_PROCESS)
# error "DUX_OPT_NO_PROCESS must be used with DUX_OPT_NO_CONSOLE"
#endif
#if defined(DUX_OPT_NO_UTIL)
# error "DUX_OPT_NO_UTIL must be used with DUX_OPT_NO_CONSOLE"
#endif
#include "../dux_internal.h"
#include <stdio.h>

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_CONSOLE[]      = DUX_IPK("Console");
DUK_LOCAL const char DUX_IPK_CONSOLE_OUT[]  = DUX_IPK("coOut");
DUK_LOCAL const char DUX_IPK_CONSOLE_ERR[]  = DUX_IPK("coErr");

/*
 * Common implementation of console functions
 */
DUK_LOCAL duk_ret_t console_print(duk_context *ctx, FILE* dest, const char *format)
{
	duk_ret_t result;

	/* [ ... ] */
	result = dux_util_format(ctx);
	if (result != 1)
	{
		return result;
	}

	/* [ ... string ] */
	fprintf(dest, format, duk_safe_to_string(ctx, -1));

	return 0; /* return undefined */
}

/*
 * Constructor of Console class
 */
DUK_LOCAL duk_ret_t console_constructor(duk_context *ctx)
{
	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}

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
	duk_push_error_object(ctx, DUK_ERR_ASSERTION_ERROR, duk_safe_to_string(ctx, -1));
	duk_throw(ctx);
	/* unreachable */
	return 0;
}

/*
 * Entry of Console.prototype.error()
 */
DUK_LOCAL duk_ret_t console_proto_error(duk_context *ctx)
{
	return console_print(ctx, stderr, "(error) %s\n");
}

/*
 * Entry of Console.prototype.info()
 */
DUK_LOCAL duk_ret_t console_proto_info(duk_context *ctx)
{
	return console_print(ctx, stdout, "(info) %s\n");
}

/*
 * Entry of Console.prototype.log()
 */
DUK_LOCAL duk_ret_t console_proto_log(duk_context *ctx)
{
	return console_print(ctx, stdout, "(log) %s\n");
}

/*
 * Entry of Console.prototype.warn()
 */
DUK_LOCAL duk_ret_t console_proto_warn(duk_context *ctx)
{
	return console_print(ctx, stderr, "(warn) %s\n");
}

/*
 * Getter of global.console
 */
DUK_LOCAL duk_ret_t global_console_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx); /* this == global */
	duk_push_heap_stash(ctx);
	/* [ global stash ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_CONSOLE);
	/* [ global stash constructor ] */
	duk_dup(ctx, 2);
	/* [ global stash constructor constructor ] */
	if (duk_pnew(ctx, 0) != DUK_EXEC_SUCCESS)
	{
		duk_throw(ctx);
		/* unreachable */
		return 0;
	}
	/* [ global stash constructor obj ] */
	duk_swap(ctx, 2, 3);
	/* [ global stash obj constructor ] */
	duk_put_prop_string(ctx, 2, "Console");
	/* [ global stash obj ] */
	duk_push_string(ctx, "console");
	/* [ global stash obj "console" ] */
	duk_dup(ctx, 2);
	/* [ global stash obj "console" obj ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE);
	/* [ global stash obj ] */
	return 1; /* return obj */
}

/*
 * List of console functions
 */
DUK_LOCAL duk_function_list_entry console_proto_funcs[] = {
	{ "assert", console_proto_assert,   DUK_VARARGS },
	{ "error",  console_proto_error,    DUK_VARARGS },
	{ "info",   console_proto_info,     DUK_VARARGS },
	{ "log",    console_proto_log,      DUK_VARARGS },
	{ "warn",   console_proto_warn,     DUK_VARARGS },
	{ NULL, NULL, 0 }
};

/*
 * Initialize Console module
 */
DUK_INTERNAL duk_errcode_t dux_console_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	dux_push_named_c_constructor(ctx, "Console",
			console_constructor, 2,
			NULL, console_proto_funcs,
			NULL, NULL);
	/* [ ... stash constructor ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_CONSOLE);
	/* [ ... stash ] */
	duk_push_global_object(ctx);
	/* [ ... stash global ] */
	duk_push_string(ctx, "console");
	duk_push_c_function(ctx, global_console_getter, 0);
	/* [ ... stash global "console" getter ] */
	duk_def_prop(ctx, -3, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_GETTER);
	/* [ ... stash global ] */
	duk_pop_2(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_CONSOLE */
