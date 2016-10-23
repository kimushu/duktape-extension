#include "dux_timer.h"
#include "sys/alt_alarm.h"

extern void top_level_error(duk_context *ctx, duk_int_t result);

static const char *const DUX_TIMER_TICK = "\xff" "dux_timer.tick";

static const char *const DUX_TIMER_LIST = "\xff" "dux_timer_alt";
static const char *const DUX_TIMER_PTR = "\xff" "ptr";
static const char *const DUX_TIMER_FUNC = "\xff" "func";

static duk_uarridx_t g_minimumId = 1;

static duk_uint_t g_time_per_tick;
static duk_uint_t g_last_time;
static alt_u32 g_last_tick;

/*
 * Get current time
 */
static duk_uint_t timer_current(void)
{
	alt_u32 new_tick = alt_nticks();
	alt_u32 ticks = new_tick - g_last_tick;
	g_last_tick = new_tick;
	if (g_time_per_tick == 0)
	{
		g_time_per_tick = alt_ticks_per_second() / 1000;
	}
	return g_last_time += g_time_per_tick * ticks;
}

/*
 * Native implementation of setInterval/setTimeout
 */
static duk_ret_t timer_set(duk_context *ctx, duk_uint_t flags)
{
	duk_uint_t interval;
	duk_idx_t num_args;
	duk_idx_t index;
	dux_timer_t *timer;
	duk_uarridx_t id;

	duk_require_function(ctx, 0);
	interval = duk_require_uint(ctx, 1);
	num_args = duk_get_top(ctx) - 2;

	timer = (dux_timer_t *)duk_alloc(ctx, sizeof(dux_timer_t));
	if (!timer)
	{
		return DUK_RET_ALLOC_ERROR;
	}

	timer->id = id = g_minimumId;
	timer->flags = flags;
	timer->time_start = timer->time_prev = timer_current();
	timer->time_next = timer->time_start + interval;
	timer->interval = interval;

	duk_push_array(ctx);
	/* [ func uint args... arr ] */
	duk_replace(ctx, 1);
	duk_swap(ctx, 0, 1);
	/* [ arr func args... ] */
	for (index = num_args - 1; index > 0; --index)
	{
		duk_put_prop_index(ctx, 0, index);
	}
	/* [ arr(args...) func ] */
	duk_put_prop_string(ctx, 0, DUX_TIMER_FUNC);
	duk_push_pointer(ctx, timer);
	/* [ arr(args...) ptr ] */
	duk_put_prop_string(ctx, 0, DUX_TIMER_PTR);
	/* [ arr(args...) ] */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_TIMER_LIST);
	duk_remove(ctx, -2);
	duk_swap_top(ctx, 0);
	/* [ arr arr(args...) ] */
	duk_put_prop_index(ctx, 0, id);
	/* [ arr ] */

	while (duk_has_prop_index(ctx, 0, ++g_minimumId));

	duk_pop(ctx);
	duk_push_uint(ctx, id);
	/* [ uint ] */
	return 1;
}

/*
 * C function entry for global.setInterval
 */
static duk_ret_t timer_setInterval(duk_context *ctx)
{
	return timer_set(ctx, 0);
}

/*
 * C function entry for global.setTimeout
 */
static duk_ret_t timer_setTimeout(duk_context *ctx)
{
	return timer_set(ctx, DUX_TIMER_ONESHOT);
}

/*
 * Native implementation of clearInterval/clearTimeout
 */
static duk_ret_t timer_clear(duk_context *ctx, duk_uint_t flags)
{
	duk_uarridx_t id = duk_require_uint(ctx, 0);
	dux_timer_t *timer;

	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_TIMER_LIST);
	duk_remove(ctx, -2);
	if (duk_get_prop_index(ctx, -1, id))
	{
		duk_get_prop_string(ctx, -1, DUX_TIMER_PTR);
		timer = (dux_timer_t *)duk_get_pointer(ctx, -1);
		duk_pop_2(ctx);

		if ((timer->flags ^ flags) & DUX_TIMER_ONESHOT)
		{
			/* Interval/Timeout mismatch */
			return DUK_RET_TYPE_ERROR;
		}
		duk_free(ctx, timer);

		duk_del_prop_index(ctx, -1, id);
		if (id < g_minimumId)
		{
			g_minimumId = id;
		}

		return 0;	/* return undefined */
	}

	/* Not found */
	return DUK_RET_TYPE_ERROR;
}

/*
 * C function entry for global.clearInterval
 */
static duk_ret_t timer_clearInterval(duk_context *ctx)
{
	return timer_clear(ctx, 0);
}

/*
 * C function entry for global.clearTimeout
 */
static duk_ret_t timer_clearTimeout(duk_context *ctx)
{
	return timer_clear(ctx, DUX_TIMER_ONESHOT);
}

/*
 * Tick handler
 */
static duk_int_t timer_tick(duk_context *ctx)
{
	alt_u32 tick = timer_current();
	dux_timer_t *timer;
	duk_uarridx_t index;
	duk_size_t length;
	duk_int_t result;
	duk_int_t processed = 0;

	/* [ ... ] */

	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_TIMER_LIST);
	duk_remove(ctx, -2);
	length = duk_get_length(ctx, -1);

	for (index = 0; index < length; ++index)
	{
		/* [ ... arr ] */
		if (!duk_get_prop_index(ctx, -1, index))
		{
			duk_pop(ctx);
			continue;
		}

		++processed;
		duk_get_prop_string(ctx, -1, DUX_TIMER_PTR);
		timer = (dux_timer_t *)duk_get_pointer(ctx, -1);
		duk_pop(ctx);
		/* [ ... arr arr(args...) ] */

		if (timer->time_prev < timer->time_next)
		{
			/*
			 * No roll-over (P<N)
			 *
			 *            <------>             (continue)
			 * [0.........P.......N.........M] (P=prev,N=next,M=max)
			 *  --------->        <----------  (expire)
			 */
			if ((timer->time_prev <= tick) && (tick < timer->time_next))
			{
				duk_pop(ctx);
				continue;
			}
		}
		else
		{
			/*
			 * With roll-over (N<P)
			 *
			 *  --->                    <----  (continue)
			 * [0...N...................P...M] (P=prev,N=next,M=max)
			 *      <------------------>       (expire)
			 */
			if ((tick < timer->time_next) || (timer->time_prev <= tick))
			{
				duk_pop(ctx);
				continue;
			}
		}

		/* Expires */
		duk_push_string(ctx, "apply");
		duk_push_undefined(ctx);
		duk_get_prop_string(ctx, -3, DUX_TIMER_FUNC);
		/* [ ... arr arr(args...) "apply" undef func ] */
		duk_swap_top(ctx, -4);
		/* [ ... arr func "apply" undef arr(args...) ] */
		result = duk_pcall_prop(ctx, -4, 2);
		/* [ ... arr func retval/err ] */
		if (result != DUK_EXEC_SUCCESS)
		{
			top_level_error(ctx, result);
		}
		duk_pop_2(ctx);
		/* [ ... arr ] */

		if (timer->flags & DUX_TIMER_ONESHOT)
		{
			duk_del_prop_index(ctx, -1, index);
			duk_free(ctx, timer);
		}
		else
		{
			timer->time_next += timer->interval;
		}
	}

	duk_pop(ctx);
	/* [ ... ] */

	return processed;
}

static duk_function_list_entry timer_funcs[] = {
	{ "clearInterval", timer_clearInterval, 1 },
	{ "clearTimeout", timer_clearTimeout, 1 },
	{ "setInterval", timer_setInterval, DUK_VARARGS },
	{ "setTimeout", timer_setTimeout, DUK_VARARGS },
	{ NULL, NULL, 0 }
};

/*
 * Initialize Timer functions
 */
void dux_timer_init(duk_context *ctx)
{
	duk_push_global_object(ctx);
	duk_put_function_list(ctx, -1, timer_funcs);
	duk_pop(ctx);

	duk_push_heap_stash(ctx);
	duk_push_array(ctx);
	duk_put_prop_string(ctx, -2, DUX_TIMER_LIST);
	duk_pop(ctx);

	duk_push_c_function(ctx, timer_tick, 0);
	dux_register_tick(ctx, DUX_TIMER_TICK);
}

