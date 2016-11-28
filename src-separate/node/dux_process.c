/*
 * ECMA class methods:
 *    process.exit([exitCode])
 *    process.nextTick(function [, arg1, ..., argN])
 *
 * ECMA class properties:
 *    process.arch
 *    process.exitCode
 *
 * Internal data structure:
 *    heap_stash[DUX_IPK_PROCESS] = global.process = process;
 *    process[DUX_IPK_PROCESS_DATA] = new PlainBuffer(dux_process_data);
 *    process[DUX_IPK_PROCESS_THREAD] = new Duktape.Thread;
 */
#if !defined(DUX_OPT_NO_PROCESS)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_PROCESS[]        = DUX_IPK("Process");
DUK_LOCAL const char DUX_IPK_PROCESS_DATA[]   = DUX_IPK("pData");
DUK_LOCAL const char DUX_IPK_PROCESS_THREAD[] = DUX_IPK("pThr");

/*
 * Entry of process.exit()
 */
DUK_LOCAL duk_ret_t process_exit(duk_context *ctx)
{
	dux_process_data *data;

	/* [ int/undefined ] */
	duk_push_this(ctx);
	/* [ int/undefined process ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_PROCESS_DATA);
	/* [ int/undefined process buf ] */
	data = (dux_process_data *)duk_require_buffer(ctx, 2, NULL);
	data->exit_code = duk_get_int(ctx, 0);
	data->exit_valid = 1;
	data->force_exit = 1;
	return 0; /* return undefined */
}

/*
 * Entry of process.nextTick()
 */
DUK_LOCAL duk_ret_t process_nextTick(duk_context *ctx)
{
	duk_idx_t nargs;
	dux_process_data *data;

	/* [ func arg1 ... argN ] */
	duk_require_callable(ctx, 0);
	nargs = duk_get_top(ctx) - 1;

	if (nargs > 0)
	{
		dux_bind_arguments(ctx, nargs);
		/* [ bound_func ] */
	}
	/* [ func ] */

	duk_push_this(ctx);
	/* [ func process ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_PROCESS_DATA);
	/* [ func process buf ] */
	data = (dux_process_data *)duk_require_buffer(ctx, 2, NULL);
	duk_swap(ctx, 0, 2);
	/* [ buf process func ] */
	if (!data->tick_context)
	{
		duk_push_thread(ctx);
		/* [ buf process func thr ] */
		data->tick_context = duk_get_context(ctx, 3);
		duk_put_prop_string(ctx, 1, DUX_IPK_PROCESS_THREAD);
		/* [ buf process func ] */
	}
	duk_xmove_top(data->tick_context, ctx, 1);
	return 0; /* return undefined */
}

/*
 * Getter of process.arch
 */
DUK_LOCAL duk_ret_t process_arch_getter(duk_context *ctx)
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
	return 1; /* return string */
}

/*
 * Getter of process.exitCode
 */
DUK_LOCAL duk_ret_t process_exitCode_getter(duk_context *ctx)
{
	dux_process_data *data;

	/* [  ] */
	duk_push_this(ctx);
	/* [ process ] */
	duk_get_prop_string(ctx, 0, DUX_IPK_PROCESS_DATA);
	/* [ process buf ] */
	data = (dux_process_data *)duk_require_buffer(ctx, 1, NULL);
	if (!data->exit_valid)
	{
		return 0; /* return undefined */
	}
	duk_push_int(ctx, data->exit_code);
	return 1; /* return int */
}

/*
 * Setter of process.exitCode
 */
DUK_LOCAL duk_ret_t process_exitCode_setter(duk_context *ctx)
{
	dux_process_data *data;

	/* [ int ] */
	duk_push_this(ctx);
	/* [ int process ] */
	duk_get_prop_string(ctx, 1, DUX_IPK_PROCESS_DATA);
	/* [ int process buf ] */
	data = (dux_process_data *)duk_require_buffer(ctx, 2, NULL);
	data->exit_code = duk_get_int(ctx, 0);
	data->exit_valid = 1;
	return 0; /* return undefined */
}

#if 0
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
#endif

/*
 * Getter of process.version
 */
DUK_LOCAL duk_ret_t process_version_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_string(ctx, DUX_VERSION_STRING);
	/* [ string ] */
	return 1; /* return string */
}

/*
 * List of methods for Process object
 */
DUK_LOCAL duk_function_list_entry process_funcs[] = {
	{ "exit", process_exit, 1 },
	{ "nextTick", process_nextTick, DUK_VARARGS },
	{ NULL, NULL, 0 }
};

/*
 * List of properties for Process object
 */
DUK_LOCAL dux_property_list_entry process_props[] = {
	{ "arch", process_arch_getter, NULL },
	{ "exitCode", process_exitCode_getter, process_exitCode_setter },
//	{ "stderr", process_stderr_getter, NULL },
//	{ "stdin", process_stdin_getter, NULL },
//	{ "stdout", process_stdout_getter, NULL },
	{ "version", process_version_getter, NULL },
	{ NULL, NULL, NULL }
};

/*
 * Initialize Process module
 */
DUK_INTERNAL duk_errcode_t dux_process_init(duk_context *ctx)
{
	dux_process_data *data;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_push_object(ctx);
	/* [ ... stash obj ] */
	duk_put_function_list(ctx, -1, process_funcs);
	dux_put_property_list(ctx, -1, process_props);
	duk_push_fixed_buffer(ctx, sizeof(dux_process_data));
	/* [ ... stash obj buf ] */
	data = (dux_process_data *)duk_get_buffer(ctx, -1, NULL);
	if (!data)
	{
		duk_pop_3(ctx);
		return DUK_ERR_ALLOC_ERROR;
	}
	memset(data, 0, sizeof(dux_process_data));
	duk_put_prop_string(ctx, -2, DUX_IPK_PROCESS_DATA);
	/* [ ... stash obj ] */
	duk_dup_top(ctx);
	/* [ ... stash obj obj ] */
	duk_put_global_string(ctx, "process");
	/* [ ... stash obj ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_PROCESS);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Tick handler for Process module
 */
DUK_INTERNAL duk_int_t dux_process_tick(duk_context *ctx)
{
	dux_process_data *data;
	duk_context *tctx;
	duk_idx_t index;
	duk_idx_t nfuncs;
	duk_bool_t force;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_IPK_PROCESS))
	{
		/* [ ... stash undefined ] */
		duk_pop_2(ctx);
		return DUX_TICK_RET_JOBLESS;
	}
	/* [ ... stash process ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_PROCESS_DATA);
	/* [ ... stash process buf ] */
	data = (dux_process_data *)duk_get_buffer(ctx, -1, NULL);
	if (!data)
	{
		duk_pop_3(ctx);
		/* [ ... ] */
		return DUX_TICK_RET_JOBLESS;
	}
	tctx = data->tick_context;
	if (!tctx)
	{
		duk_pop_3(ctx);
		/* [ ... ] */
		return data->force_exit ? DUX_TICK_RET_ABORT : DUX_TICK_RET_JOBLESS;
	}

	duk_get_prop_string(ctx, -2, DUX_IPK_PROCESS_THREAD);
	/* [ ... stash process buf thr ] */
	data->tick_context = NULL;
	nfuncs = duk_get_top(tctx);
	for (index = 0; (!(force = data->force_exit)) && (index < nfuncs); ++index)
	{
		duk_dup(tctx, index);
		/* tctx [ func1 ... funcN funcI ] */
		if (duk_pcall(tctx, 0) != DUK_EXEC_SUCCESS)
		{
			/* tctx [ func1 ... funcN err ] */
			dux_report_error(tctx);
		}
		/* tctx [ func1 ... funcN retval/err ] */
		duk_pop(tctx);
		/* tctx [ func1 ... funcN ] */
	}
	/* [ ... stash process buf thr ] */
	duk_pop_n(ctx, 4);
	/* [ ... ] */
	return force ? DUX_TICK_RET_ABORT : DUX_TICK_RET_CONTINUE;
}

#endif  /* !DUX_OPT_NO_PROCESS */
