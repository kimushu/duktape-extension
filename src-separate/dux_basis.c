#include "dux_internal.h"

DUK_LOCAL const char DUX_IPK_TABLE[]  = DUX_IPK("bTable");
DUK_LOCAL const char DUX_IPK_STORE[]  = DUX_IPK("bStore");

/*
 * Initialize Duktape extension modules
 */
DUK_EXTERNAL duk_bool_t dux_initialize(duk_context *ctx)
{
	dux_context_table *table;
	duk_bool_t result;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	duk_push_fixed_buffer(ctx, sizeof(*table));
	mctx = (dux_context_table *)duk_get_buffer(ctx, -1, NULL);
	memset(table, 0, sizeof(*table));
	duk_put_prop_string(ctx, -2, DUX_IPK_TABLE);
	/* [ ... ] */
	duk_push_object(ctx);
	/* [ ... obj ] */
	duk_dup_top(ctx);
	duk_put_prop_string(ctx, -2, DUX_IPK_STORE);
	duk_push_global_object(ctx);
	/* [ ... obj global ] */

	result =
#ifndef DUX_OPT_NO_PROCESS
		dux_process_init(ctx, &table->process) &&
#endif
#ifndef DUX_OPT_NO_UTIL
		dux_util_init(ctx/*, &table->util*/) &&
#endif
#ifndef DUX_OPT_NO_TIMER
		dux_timer_init(ctx, &table->timer) &&
#endif
		&& 1;

	duk_pop_2(ctx);
	/* [ ... ] */
	return result;
}

/*
 * Tick handler
 */
DUX_EXTENRAL duk_bool_t dux_tick(duk_context *ctx)
{
	dux_context_table *table;
	duk_int_t result;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -2, DUX_IPK_TABLE);
	table = (dux_context_table *)duk_get_buffer(ctx, -1, NULL);
	duk_pop_2(ctx);
	/* [ ... ] */

	if (!table)
	{
		return 0;
	}

	result = 0;
#ifndef DUX_OPT_NO_PROCESS
	result |= dux_process_tick(ctx, &table->process);
	if (result & DUX_TICK_RET_ABORT)
	{
		return 0;
	}
#endif
#ifndef DUX_OPT_NO_TIMER
	result |= dux_timer_tick(ctx, &table->timer);
	if (result & DUX_TICK_RET_ABORT)
	{
		return 0;
	}
#endif

	return (result & DUX_TICK_RET_CONTINUE) ? 1 : 0;
}

