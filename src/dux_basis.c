#include "dux_internal.h"
#include <stdio.h>
#include <stdarg.h>

// #define DEBUG

DUK_LOCAL const char DUX_IPK_TABLE[]        = DUX_IPK("bTable");
DUK_LOCAL const char DUX_IPK_STORE[]        = DUX_IPK("bStore");
DUK_LOCAL const char DUX_IPK_FILE_ACCESS[]  = DUX_IPK("bFile");

/*
 * Initialize Duktape extension modules
 */
DUK_EXTERNAL duk_errcode_t dux_initialize(duk_context *ctx, const dux_file_accessor *file_accessor)
{
	if (file_accessor) {
		/* [ ... ] */
		duk_push_heap_stash(ctx);
		/* [ ... stash ] */
		duk_push_pointer(ctx, (void *)file_accessor);
		/* [ ... stash pointer ] */
		duk_put_prop_string(ctx, -2, DUX_IPK_FILE_ACCESS);
		/* [ ... stash ] */
		duk_pop(ctx);
		/* [ ... ] */
	}
	return dux_invoke_initializers(ctx,
		DUX_INIT_MODULES
		DUX_INIT_PROMISE
		DUX_INIT_THRPOOL
		DUX_INIT_NODE
		DUX_INIT_HARDWARE
		DUX_INIT_PERIDOT
		DUX_INIT_IMMEDIATE
		DUX_INIT_PACKAGE_DELAY
		DUX_INIT_PACKAGE_SPRINTF
		NULL
	);
}

/*
 * Tick handler
 */
DUK_EXTERNAL duk_bool_t dux_tick(duk_context *ctx)
{
	duk_int_t result;
	result = dux_invoke_tick_handlers(ctx,
		DUX_TICK_MODULES
		DUX_TICK_PROMISE
		DUX_TICK_THRPOOL
		DUX_TICK_NODE
		DUX_TICK_HARDWARE
		DUX_TICK_PERIDOT
		DUX_TICK_IMMEDIATE	/* IMMEDIATE must be the last tick handler */
		NULL
	);

	return (result & DUX_TICK_RET_CONTINUE) ? 1 : 0;
}

/*
 * Invoker for initializers
 */
DUK_INTERNAL duk_errcode_t dux_invoke_initializers(duk_context *ctx, ...)
{
#ifdef DEBUG
	static int level;
#endif
	duk_errcode_t result = DUK_ERR_NONE;
	dux_initializer init;
	va_list args;
	va_start(args, ctx);
	while (result == DUK_ERR_NONE) {
		init = va_arg(args, dux_initializer);
		if (!init) {
			break;
		}
#ifdef DEBUG
		printf("[%d] init:%p (top:%d)\n", level++, init, duk_get_top(ctx));
#endif
		result = (*init)(ctx);
#ifdef DEBUG
		printf("[%d] => %d (top:%d)\n", --level, result, duk_get_top(ctx));
#endif
	}
	va_end(args);
	return result;
}

/*
 * Invoker for tick handlers
 */
DUK_INTERNAL duk_int_t dux_invoke_tick_handlers(duk_context *ctx, ...)
{
#ifdef DEBUG
	static int level;
#endif
	duk_int_t result = 0;
	dux_tick_handler tick;
	va_list args;
	va_start(args, ctx);
	while ((result & DUX_TICK_RET_ABORT) == 0) {
		tick = va_arg(args, dux_tick_handler);
		if (!tick) {
			break;
		}
#ifdef DEBUG
		printf("[%d] tick:%p (top:%d)\n", level++, tick, duk_get_top(ctx));
#endif
		result |= (*tick)(ctx);
#ifdef DEBUG
		printf("[%d] => %d (top:%d)\n", --level, result, duk_get_top(ctx));
#endif
	}
	va_end(args);
	return result;
}

/*
 * Top level error reporter
 */
DUK_INTERNAL void dux_report_error(duk_context *ctx)
{
	/* TODO */
	fprintf(stderr, "Uncaught error: %s\n", duk_safe_to_string(ctx, -1));
}

/*
 * Top level warning reporter
 */
DUK_INTERNAL void dux_report_warning(duk_context *ctx)
{
	fprintf(stderr, "Warning: %s\n", duk_safe_to_string(ctx, -1));
}

/*
 * Push inherited prototype
 */
DUK_INTERNAL void dux_push_inherited_object(duk_context *ctx, duk_idx_t super_idx)
{
	/* [ ... super ... constructor ] */
	/*  super_idx^     ^top          */
	super_idx = duk_normalize_index(ctx, super_idx);
	duk_get_global_string(ctx, "Object");
	duk_push_string(ctx, "create");
	duk_get_prop_string(ctx, super_idx, "prototype");
	/* [ ... super ... constructor Object "create" super_proto ] */
	duk_call_prop(ctx, -3, 1);
	/* [ ... super ... constructor Object inherited ] */
	duk_replace(ctx, -2);
	/* [ ... super ... constructor inherited ] */
	duk_dup(ctx, -2);
	/* [ ... super ... constructor inherited constructor ] */
	duk_put_prop_string(ctx, -2, "constructor");
	/* [ ... super ... constructor inherited ] */
}

/*
 * Convert to byte array (fixed buffer)
 */
DUK_INTERNAL void *dux_to_byte_buffer(duk_context *ctx, duk_idx_t index, duk_size_t *out_size)
{
	/* [ ... val ... ] */
	duk_size_t len;
	void *buf;
	const void *src;

	index = duk_normalize_index(ctx, index);
	if (duk_is_array(ctx, index))
	{
		/* Array */
		duk_uarridx_t aidx;
		unsigned char *dest;

		len = duk_get_length(ctx, index);
		buf = duk_push_fixed_buffer(ctx, len);
		dest = (unsigned char *)buf;
		for (aidx = 0; aidx < len; ++aidx)
		{
			duk_int_t value;
			duk_get_prop_index(ctx, index, aidx);
			value = dux_require_int_range(ctx, -1, 0, 255);
			duk_pop(ctx);
			*dest++ = (unsigned char)value;
		}
	}
	else if (((src = duk_get_lstring(ctx, index, &len)) != NULL) ||
			 ((src = duk_require_buffer_data(ctx, index, &len)) != NULL))
	{
		/* string or non-empty buffers */
		buf = duk_push_fixed_buffer(ctx, len);
		memcpy(buf, src, len);
	}
	else
	{
		/* invalid data type */
		return NULL;
	}

	if (out_size)
	{
		*out_size = len;
	}

	duk_replace(ctx, index);
	return buf;
}

DUK_INTERNAL duk_int_t dux_require_int_range(duk_context *ctx, duk_idx_t index,
		duk_int_t minimum, duk_int_t maximum)
{
	duk_int_t value = duk_require_int(ctx, index);
	if ((value < minimum) || (value > maximum))
	{
		(void)duk_range_error(ctx, "value out of range: %d", value);
		/* unreachable */
	}
	return value;
}

DUK_INTERNAL duk_bool_t dux_get_array_index(duk_context *ctx, duk_idx_t key_idx, duk_uarridx_t *result)
{
	const char *str;
	char ch;
	duk_uint_t arr_idx;
	duk_int_t top = duk_get_top(ctx);

	if (duk_is_number(ctx, key_idx)) {
		duk_dup(ctx, key_idx);
		str = duk_to_string(ctx, -1);
	} else {
		str = duk_get_string(ctx, key_idx);
		if (!str) {
			goto fail;
		}
	}

	int len = strlen(str);
	if (len < 1 || len > 10) {
		goto fail;
	}

	arr_idx = 0;
	for (; (ch = *str) != '\0'; ++str) {
		duk_uarridx_t old;

		if (ch < '0') {
			/* non-digit character */
			goto fail;
		} else if (ch == '0') {
			if (str[1] != '\0') {
				/* leading zeros not allowed */
				goto fail;
			}
			break;
		} else if (ch > '9') {
			/* non-digit character */
			goto fail;
		}
		old = arr_idx;
		arr_idx = (arr_idx * 10) + (ch - '0');
		if (arr_idx <= old) {
			/* overflow */
			goto fail;
		}
	}

	if (result) {
		*result = arr_idx;
	}
	duk_set_top(ctx, top);
	return 1;

fail:
	duk_set_top(ctx, top);
	return 0;
}

DUK_INTERNAL duk_int_t dux_read_file(duk_context *ctx, const char *path)
{
	const dux_file_accessor *accessor;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_FILE_ACCESS);
	accessor = (const dux_file_accessor *)duk_get_pointer(ctx, -1);
	duk_pop_2(ctx);
	/* [ ... ] */
	if ((!accessor) || (!accessor->reader)) {
		duk_push_error_object(ctx, DUK_ERR_ERROR, "No file reader");
		/* [ ... err ] */
		return DUK_EXEC_ERROR;
	}
	return (*accessor->reader)(ctx, path);
}
