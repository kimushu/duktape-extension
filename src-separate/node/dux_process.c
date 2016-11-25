#include "dux_process.h"
#include "dux_common.h"
#include "dux_private.h"
#include <unistd.h>

static const char* const DUX_IPK_EXITCODE = "\xff" "kEc";

/*
 * C function entry of process.exit
 */
static duk_ret_t process_exit(duk_context *ctx)
{
	/* [ int/undefined ] */
	duk_push_this(ctx);
	/* [ int/undefined this ] */
	duk_push_int(ctx, duk_get_int(ctx, 0));
	/* [ int/undefined this int ] */
	if (!duk_has_prop_string(ctx, 1, DUX_IPK_EXITCODE))
	{
		duk_put_prop_string(ctx, 1, DUX_IPK_EXITCODE);
	}
	return 0;	/* return undefined; */
}

/*
 * C function entry of process.nextTick
 */
static duk_ret_t process_nexttick(duk_context *ctx)
{
	/* [ func arg1 ... argN ] */
	duk_idx_t bind_nargs;

	duk_require_callable(ctx, 0);
	bind_nargs = duk_get_top(ctx);

	if (nargs > 1)
	{
		duk_push_string(ctx, "bind");
		duk_insert(ctx, 1);
		/* [ func "bind" arg1 ... argN ] */
		duk_push_undefined(ctx);
		duk_insert(ctx, 2);
		/* [ func "bind" undefined arg1 ... argN ] */
		duk_call_prop(ctx, bind_nargs);
		/* [ func bound_func ] */
	}
	else
	{
		/* [ func ] */
	}
	dux_process_register_tick(ctx);
	return 0;	/* return undefined; */
}

/*
 * C function entry of getter for process.arch
 */
static duk_ret_t process_arch_getter(duk_context *ctx)
{
#if defined(__x86_64__)
	duk_push_string(ctx, "x64");
#elif defined(__i386__)
	duk_push_string(ctx, "ia32");
#elif defined(__arm__)
	duk_push_string(ctx, "arm");
#elif defined(__nios2__)
	duk_push_string(ctx, "nios2");
#else
	duk_push_string(ctx, "unknown");
#endif
	return 1;	/* return string; */
}

/*
 * C function entry of getter for process.stderr
 */
static duk_ret_t process_stderr_getter(duk_context *ctx)
{
	/* [  ] */
	dux_push_stream_constructor(ctx);
	/* [ constructor ] */
	duk_push_uint(ctx, STDERR_FILENO);
	duk_push_uint(ctx, O_WRONLY);
	/* [ constructor uint uint ] */
	duk_new(ctx, 2);
	/* [ retval ] */
	duk_push_this(ctx);
	duk_push_string(ctx, "stderr");
	duk_dup(ctx, 0);
	/* [ retval process "stderr" retval ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE |
			DUK_DEFPROP_CLEAR_WRITABLE);
	/* [ retval ] */
	return 1;	/* return retval; */
}

/*
 * C function entry of getter for process.stdin
 */
static duk_ret_t process_stdin_getter(duk_context *ctx)
{
	/* [  ] */
	dux_push_stream_constructor(ctx);
	/* [ constructor ] */
	duk_push_uint(ctx, STDIN_FILENO);
	duk_push_uint(ctx, O_RDONLY);
	/* [ constructor uint uint ] */
	duk_new(ctx, 2);
	/* [ retval ] */
	duk_push_this(ctx);
	duk_push_string(ctx, "stdin");
	duk_dup(ctx, 0);
	/* [ retval process "stdin" retval ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE |
			DUK_DEFPROP_CLEAR_WRITABLE);
	/* [ retval ] */
	return 1;	/* return retval; */
}

/*
 * C function entry of getter for process.stdout
 */
static duk_ret_t process_stdout_getter(duk_context *ctx)
{
	/* [  ] */
	dux_push_stream_constructor(ctx);
	/* [ constructor ] */
	duk_push_uint(ctx, STDOUT_FILENO);
	duk_push_uint(ctx, O_WRONLY);
	/* [ constructor uint uint ] */
	duk_new(ctx, 2);
	/* [ retval ] */
	duk_push_this(ctx);
	duk_push_string(ctx, "stdout");
	duk_dup(ctx, 0);
	/* [ retval process "stdout" retval ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE |
			DUK_DEFPROP_CLEAR_WRITABLE);
	/* [ retval ] */
	return 1;	/* return retval; */
}

static duk_function_list_entry process_funcs[] = {
	{ "exit", process_exit, 1 },
	{ "nextTick", process_nexttick, DUK_VARARGS },
	{ NULL, NULL, 0 }
};

static dux_property_list_entry process_props[] = {
	{ "arch", process_arch_getter, NULL },
	{ "stderr", process_stderr_getter, NULL },
	{ "stdin", process_stdin_getter, NULL },
	{ "stdout", process_stdout_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize Process object
 */
void dux_process_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_INTRINSIC_PROCESS))
	{
		/* [ ... stash undefined ] */
		duk_push_object(ctx);
		duk_copy(ctx, -1, -2);
		/* [ ... stash obj obj ] */
		duk_put_function_list(ctx, -1, process_funcs);
		dux_put_property_list(ctx, -1, process_props);
		duk_put_prop_string(ctx, -2, DUX_INTRINSIC_PROCESS);
		/* [ ... stash obj ] */
	}
	/* [ ... stash obj ] */
	duk_put_global_string(ctx, "process");
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
}

/*
 * Get exit code
 */
duk_bool_t dux_process_is_exit(duk_context *ctx, duk_int_t *code)
{
	/* [ ... ] */
	duk_bool_t result = 0;

	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (duk_get_prop_string(ctx, -1, DUX_INTRINSIC_PROCESS))
	{
		/* [ ... stash obj ] */
		if (duk_get_prop_string(ctx, -1, DUX_IPK_EXITCODE))
		{
			/* [ ... stash obj int ] */
			if (code)
			{
				*code = duk_get_int(ctx, -1);
			}
			result = 1;
		}
		/* [ ... stash obj int/undefined ] */
		duk_pop_3(ctx);
		/* [ ... ] */
	}
	else
	{
		duk_pop_2(ctx);
		/* [ ... ] */
	}

	return result;
}

/*
 * Register next tick handler
 */
duk_bool_t dux_process_register_tick(duk_context *ctx)
{
	/* [ ... func ] */
	duk_context *sub_ctx;
	duk_push_heap_stash(ctx);
	/* [ ... func stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_INTRINSIC_PROCESS))
	{
		/* [ ... func stash undefined ] */
		duk_pop_3(ctx);
		return 0;
	}
	/* [ ... func stash obj ] */
	if (!duk_get_prop_string(ctx, -1, IPK_NEXT_TICK))
	{
		/* [ ... func stash obj undefined ] */
		duk_push_thread(ctx);
		duk_copy(ctx, -1, -2);
		/* [ ... func stash obj thr thr ] */
		duk_put_prop_string(ctx, -3, IPK_NEXT_TICK);
		/* [ ... func stash obj thr ] */
	}
	/* [ ... func stash obj thr ] */
	sub_ctx = duk_require_context(ctx, -1);
	duk_pop_3(ctx);
	/* [ ... func ] */
	duk_xmove_top(sub_ctx, ctx, 1);
	/* sub: [ func1 ... funcN ] */
	/* [ ... func ] */
	return 1;
}

/*
 * Tick handler
 */
duk_uint_t dux_process_tick(duk_context *ctx)
{
	/* [ ... ] */
	duk_context *sub_ctx;
	duk_idx_t index;
	duk_idx_t items;

	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, DUX_INTRINSIC_PROCESS))
	{
		/* [ ... stash undefined ] */
		duk_pop_2(ctx);
		/* [ ... ] */
		return 0;
	}
	duk_remove(ctx, -2);
	/* [ ... obj ] */
	if (!duk_get_prop_string(ctx, IPK_NEXT_TICK))
	{
		/* [ ... obj undefined ] */
		duk_pop_2(ctx);
		/* [ ... ] */
		return 0;
	}
	/* [ ... obj thr] */
	duk_push_thread(ctx);
	/* [ ... obj thr new_thr ] */
	duk_put_prop_string(ctx, -3, IPK_NEXT_TICK);
	/* [ ... obj thr ] */
	duk_remove(ctx, -2);
	/* [ ... thr ] */
	sub_ctx = duk_require_context(ctx, -1);
	/* sub: [ func1 ... funcN ] */
	items = duk_get_top(sub_ctx);
	for (index = 0; index < items; ++index)
	{
		duk_dup(sub_ctx, index);
		/* sub: [ func1 ... funcN funcI ] */
		if (duk_pcall(sub_ctx, 0) != 0)
		{
			/* sub: [ func1 ... funcN err ] */
			top_level_error(sub_ctx);
		}
		else
		{
			/* sub: [ func1 ... funcN retval ] */
		}
		duk_pop(sub_ctx);
		/* sub: [ func1 ... funcN ] */
	}
	/* [ ... ] */
	duk_pop(ctx);
	return items;
}

