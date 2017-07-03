#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_STREAM)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_WRITABLE_STATE[] = DUX_IPK("stWs");
DUK_LOCAL const char DUX_IPK_READABLE_STATE[] = DUX_IPK("stRs");

DUK_LOCAL duk_ret_t stream_constructor(duk_context *ctx)
{
	/* [  ] */
	return 0;
}

DUK_LOCAL dux_stream_writable_data *writable_get_data(duk_context *ctx)
{
	dux_stream_writable_data *data;
	/* [ ... ] */
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUX_IPK_WRITABLE_STATE);
	data = (dux_stream_writable_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	/* [ ... ] */
	return data;
}

DUK_LOCAL duk_ret_t writable_proto_write_body(duk_context *ctx, duk_bool_t end)
{
	/* [ chunk encoding callback ] */
	dux_stream_writable_data *data = writable_get_data(ctx);
	if (duk_is_callable(ctx, 1)) {
		duk_push_undefined(ctx);
		duk_insert(ctx, 1);
		duk_set_top(ctx, 3);
	}
	// FIXME: encoding
	
}

DUK_LOCAL duk_ret_t writable_proto_cork(duk_context *ctx)
{
	dux_stream_writable_data *data = writable_get_data(ctx);
	if (++data->corked == 0) {
		return DUK_RET_ERROR;
	}
	return 0;
}

DUK_LOCAL duk_ret_t writable_proto_end(duk_context *ctx)
{
	/* [ chunk encoding callback ] */
	return writable_proto_write_body(ctx, 1);
}

DUK_LOCAL duk_ret_t writable_proto_setDefaultEncoding(duk_context *ctx)
{
}

DUK_LOCAL duk_ret_t writable_proto_uncork(duk_context *ctx)
{
	dux_stream_writable_data *data = writable_get_data(ctx);
	if (data->corked > 0) {
		--data->corked;
	}
	return 0;
}

DUK_LOCAL duk_ret_t writable_proto_write(duk_context *ctx)
{
	/* [ chunk encoding callback ] */
	return writable_proto_write_body(ctx, 0);
}

/**
 * List of methods for Writable object
 */
DUK_LOCAL duk_function_list_entry writable_proto_funcs[] = {
    { "cork", writable_proto_cork, 0 },
    { "end", writable_proto_end, 3 },
    { "setDefaultEncoding", writable_proto_setDefaultEncoding, 1 },
    { "uncork", writable_proto_uncork, 0 },
    { "write", writable_proto_write, 3 },
    { NULL, NULL, 0 }
};

/**
 * Constructor of Writable
 */
DUK_LOCAL duk_ret_t writable_constructor(duk_context *ctx)
{
	/* [ obj/undefined ] */
	if (!duk_is_null_or_undefined(ctx, 0)) {
	}
	return 0;
}

/**
 * List of methods for Readable object
 */
DUK_LOCAL duk_function_list_entry readable_proto_funcs[] = {
    { "isPaused", readable_proto_isPaused, 0 },
    { "pause", readable_proto_pause, 0 },
    { "pipe", readable_proto_pipe, 2 },
    { "read", readable_proto_read, 1 },
    { "resume", readable_proto_resume, 0 },
    { "setEncoding", readable_proto_setEncoding, 1 },
    { "unpipe", readable_proto_unpipe, 1 },
    { "unshift", readable_proto_unshift, 1 },
    { NULL, NULL, 0 }
};

/**
 * Constructor of Readable
 */
DUK_LOCAL duk_ret_t readable_constructor(duk_context *ctx)
{
	/* [ obj/undefined ] */
	if (!duk_is_null_or_undefined(ctx, 0)) {
	}
	return 0;
}

/**
 * Entry of stream module
 */
DUK_LOCAL duk_errcode_t stream_entry(duk_context *ctx)
{
	/* [ require module exports ] */
	duk_swap(ctx, 0, 2);
	/* [ exports module require ] */
	duk_push_string(ctx, "events");
	duk_call(ctx, 1);
	/* [ exports module EventEmitter ] */
	dux_push_inherited_named_c_constructor(
		ctx, 2, "Stream", stream_constructor, 0,
		NULL, NULL, NULL, NULL
	);
	/* [ exports module EventEmitter constructor ] */
	duk_dup(ctx, 3);
	duk_put_prop_string(ctx, 3, "Stream");
	/* [ exports module EventEmitter constructor ] */
	dux_push_inherited_named_c_constructor(
		ctx, 3, "Writable", writable_constructor, 1,
		NULL, writable_proto_funcs, NULL, NULL
	);
	duk_put_prop_string(ctx, 3, "Writable");
	/* [ exports module EventEmitter constructor ] */
	dux_push_inherited_named_c_constructor(
		ctx, 3, "Readable", readable_constructor, 1,
		NULL, readable_proto_funcs, NULL, NULL
	);
	duk_put_prop_string(ctx, 3, "Readable");
	/* [ exports module EventEmitter constructor ] */
	duk_put_prop_string(ctx, 1, "exports");
	/* [ exports module EventEmitter ] */
	return DUK_ERR_NONE;
}

/**
 * Initialize stream module
 */
DUK_INTERNAL duk_errcode_t dux_stream_init(duk_context *ctx)
{
	return dux_modules_register(ctx, "stream", stream_entry);
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_STREAM */
