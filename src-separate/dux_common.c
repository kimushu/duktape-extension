#include "dux_common.h"
#include <pthread.h>
#include <stdio.h>

static const char *const DUX_COMMON_TICK = "\xff" "dux_common_tick";
static const char *const DUX_COMMON_CB = "\xff" "dux_common_cb";

/*
 * Print top level error
 */
__attribute__((weak)) void top_level_error(duk_context *ctx)
{
	/* [ ... err ] */
	fprintf(stderr, "Error: %s\n", duk_safe_to_string(ctx, -1));
}

/*
 * Register tick handler
 * [ ... func ]  ->  [ ... ]
 */
void dux_register_tick(duk_context *ctx, const char *key)
{
	/* [ ... func ] */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_COMMON_TICK);
	if (!duk_is_object(ctx, -1))
	{
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_dup_top(ctx);
		duk_put_prop_string(ctx, -3, DUX_COMMON_TICK);
	}
	/* [ ... func stash obj ] */
	if (duk_is_null_or_undefined(ctx, -3))
	{
		duk_del_prop_string(ctx, -1, key);
	}
	else
	{
		duk_dup(ctx, -3);
		/* [ ... func stash obj func ] */
		duk_put_prop_string(ctx, -2, key);
	}
	/* [ ... func stash obj ] */
	duk_pop_3(ctx);
	/* [ ... ] */
}

/*
 * Queue callback
 * [ ... func ]  ->  [ ... ]
 */
void dux_queue_callback(duk_context *ctx)
{
	/* [ ... func ] */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_COMMON_CB);
	if (!duk_is_array(ctx, -1))
	{
		duk_pop(ctx);
		duk_push_array(ctx);
		duk_dup_top(ctx);
		duk_put_prop_string(ctx, -3, DUX_COMMON_CB);
	}
	/* [ ... func stash arr ] */
	duk_dup(ctx, -3);
	duk_put_prop_index(ctx, -2, duk_get_length(ctx, -1));
	duk_pop_3(ctx);
	/* [ ... ] */
}

/*
 * Process tick handlers
 * [ ... ]  ->  [ ... ]
 */
void dux_tick(duk_context *ctx)
{
	/* [ ... ] */
	duk_context *safe_ctx;
	duk_push_thread(ctx);
	safe_ctx = duk_require_context(ctx, -1);
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_COMMON_TICK);
	if (duk_is_object(ctx, -1))
	{
		duk_enum(ctx, -1, DUK_ENUM_INCLUDE_INTERNAL);
		/* [ ... thr stash obj enum ] */
		while (duk_next(ctx, -1, 1))
		{
			/* [ ... stash obj enum key value(func) ] */
			if (duk_pcall(ctx, 0) != 0)
			{
				/* [ ... stash obj enum key err ] */
				duk_set_top(safe_ctx, 0);
				duk_xcopy_top(safe_ctx, ctx, 1);
				top_level_error(safe_ctx);
			}
			duk_pop_2(ctx);
			/* [ ... stash obj enum ] */
		}
		duk_pop(ctx);
	}
	duk_pop(ctx);
	/* [ ... thr stash ] */
	duk_get_prop_string(ctx, -1, DUX_COMMON_CB);
	if (duk_is_array(ctx, -1))
	{
		duk_uarridx_t pos;
		duk_size_t len;

		duk_push_array(ctx);
		duk_put_prop_string(ctx, -3, DUX_COMMON_CB);
		/* [ ... thr stash arr ] */
		len = duk_get_length(ctx, -1);
		for (pos = 0; pos < len; ++pos)
		{
			duk_get_prop_index(ctx, -1, pos);
			if (duk_pcall(ctx, 0) != 0)
			{
				/* [ ... thr stash arr err ] */
				duk_set_top(safe_ctx, 0);
				duk_xcopy_top(safe_ctx, ctx, 1);
				top_level_error(safe_ctx);
			}
			duk_pop(ctx);
		}
	}
	/* [ ... thr stash arr/undefined ] */
	duk_pop_3(ctx);
	/* [ ... ] */
}

