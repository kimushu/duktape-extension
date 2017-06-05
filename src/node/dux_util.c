#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_UTIL)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_PROMISIFY_FUNC[] = DUX_IPK("upFunc");
DUK_LOCAL const char DUX_SYM_PROMISIFY_CUSTOM[] = "\x80util.promisify.custom";

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
		case 'o':
			if (arg < nargs)
			{
				duk_dup(ctx, arg++);
			}
			else
			{
				duk_push_undefined(ctx);
			}
			duk_safe_to_string(ctx, -1);
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
	return DUK_RET_ERROR;
}

#if !defined(DUX_OPT_NO_PROMISE)
/*
 * Callback for promisified function
 */
DUK_LOCAL duk_ret_t util_promisify_callback(duk_context *ctx)
{
	/* [ resolve reject err result ] */
	if (duk_is_null_or_undefined(ctx, 2)) {
		/* Fulfilled */
		duk_swap(ctx, 0, 2);
		/* [ null|undefined reject resolve result ] */
		duk_call(ctx, 1);
	} else {
		/* Rejected */
		duk_pop(ctx);
		/* [ resolve reject err ] */
		duk_call(ctx, 1);
	}
	return 0;
}

/*
 * Executor of promisified function
 */
DUK_LOCAL duk_ret_t util_promisify_executor(duk_context *ctx)
{
	duk_context *sub;

	/* ctx: [ thread resolve reject ] */
	sub = duk_require_context(ctx, 0);
	duk_push_c_function(sub, util_promisify_callback, 4);
	duk_xcopy_top(sub, ctx, 2);
	/* sub: [ func this ... callback resolve reject ] */
	dux_bind_arguments(sub, 2);
	/* sub: [ func this ... bound_callback ] */
	if (duk_pcall_method(sub, duk_get_top(sub) - 2) != DUK_EXEC_SUCCESS) {
		/* sub: [ err ] */
		duk_xmove_top(ctx, sub, 1);
		/* ctx: [ thread resolve reject err ] */
		/* sub: [  ] */
		duk_call(ctx, 1);
	}
	return 0;
}

/*
 * Entry of promisified function
 */
DUK_LOCAL duk_ret_t util_promisify_invoker(duk_context *ctx)
{
	duk_context *sub;

	/* [ ... ] */
	duk_push_thread(ctx);
	duk_insert(ctx, 0);
	/* [ thread ... ] */
	duk_push_current_function(ctx);
	/* [ thread ... invoker ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_PROMISIFY_FUNC);
	duk_push_this(ctx);
	/* [ thread ... invoker func this ] */
	sub = duk_require_context(ctx, 0);
	duk_xmove_top(sub, ctx, 2);
	duk_pop(ctx);
	duk_xmove_top(sub, ctx, duk_get_top(ctx) - 1);
	/* ctx: [ thread ] */
	/* sub: [ func this ... ] */

	duk_get_global_string(ctx, "Promise");
	/* ctx: [ thread Promise ] */
	duk_push_c_function(ctx, util_promisify_executor, 3);
	duk_dup(ctx, 0);
	/* ctx: [ thread Promise executor thread ] */
	dux_bind_arguments(ctx, 1);
	/* ctx: [ thread Promise bound_executor ] */
	duk_new(ctx, 1);
	/* ctx: [ thread promise ] */
	return 1;
}

/*
 * Entry of util.promisify()
 */
DUK_LOCAL duk_ret_t util_promisify(duk_context *ctx)
{
	/* [ func ] */
	duk_require_function(ctx, 0);
	if (duk_get_prop_string(ctx, 0, DUX_SYM_PROMISIFY_CUSTOM)) {
		/* func already has promisified function */
		/* [ func promisified_func ] */
		return 1;
	}
	duk_pop(ctx);

	/* [ func ] */
	duk_push_c_function(ctx, util_promisify_invoker, DUK_VARARGS);
	duk_swap(ctx, 0, 1);
	/* [ invoker func ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_PROMISIFY_FUNC);
	/* [ invoker ] */
	return 1;
}
#endif	/* !DUX_OPT_NO_PROMISE */

/*
 * List of methods for Util object
 */
DUK_LOCAL duk_function_list_entry util_funcs[] = {
	// debuglog
	// deprecate
	{ "format", util_format, DUK_VARARGS },
	// inherits
	{ "inspect", util_inspect, 2 },
	{ NULL, NULL, 0 }
};

DUK_LOCAL duk_errcode_t dux_util_entry(duk_context *ctx)
{
	/* [ require module exports ] */
	duk_put_function_list(ctx, 2, util_funcs);
#if !defined(DUX_OPT_NO_PROMISE)
	duk_push_c_function(ctx, util_promisify, 1);
	/* [ require module exports func:3 ] */
	duk_push_string(ctx, "custom");
	duk_push_string(ctx, DUX_SYM_PROMISIFY_CUSTOM);
	/* [ require module exports func:3 "custom":4 symbol:5 ] */
	duk_def_prop(ctx, 3, DUK_DEFPROP_HAVE_VALUE |
		DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_SET_ENUMERABLE);
	/* [ require module exports func:3 ] */
	duk_put_prop_string(ctx, 2, "promisify");
#endif	/* !DUX_OPT_NO_PROMISE */
	return DUK_ERR_NONE;
}

DUK_INTERNAL duk_errcode_t dux_util_init(duk_context *ctx)
{
	return dux_modules_register(ctx, "util", dux_util_entry);
}

DUK_INTERNAL duk_ret_t dux_util_format(duk_context *ctx)
{
	return util_format(ctx);
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_UTIL */
