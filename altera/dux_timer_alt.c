/*
 * ECMA classes/functions:
 *    global.clearInterval(id)
 *    global.clearTimeout(id)
 *    global.setInterval(function, interval [, arg1, ..., argN])  // => id
 *    global.setTimeout(function, interval [, arg1, ..., argN])   // => id
 *
 * Native C functions:
 *    void dux_timer_init(duk_context *ctx);
 *    duk_uint_t dux_timer_tick(duk_context *ctx);
 *
 * Internal data structure:
 *    heap_stash[DUX_TIMER_LIST] = new Array(
 *      <uint>, // Next free ID
 *      <Duktape.Buffer>{func: <func>}, // [1]
 *      <Duktape.Buffer>{func: <func>}, // [2]
 *          :
 *      <Duktape.Buffer>{func: <func>}  // [N]
 *    );
 */

#include "dux_timer.h"
#include "sys/alt_alarm.h"

extern void top_level_error(duk_context *ctx);

static const char *const DUX_TIMER_LIST = "dux_timer_alt.timers";

static duk_uint_t g_time_per_tick;
static duk_uint_t g_last_time;
static alt_u32 g_last_tick;

/*
 * Push timer array
 */
static void timer_push_array(duk_context *ctx)
{
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_TIMER_LIST))
	{
		duk_pop(ctx);
		/* [ ... stash ] */
		duk_push_array(ctx);
		duk_push_uint(ctx, 1);
		duk_put_prop_index(ctx, -2, 0);
		/* [ ... stash arr ] */
		duk_dup_top(ctx);
		/* [ ... stash arr arr ] */
		duk_put_prop_string(ctx, -3, DUX_TIMER_LIST);
		/* [ ... stash arr ] */
	}
	/* [ ... stash arr ] */
	duk_remove(ctx, -2);
	/* [ ... arr ] */
}

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
	duk_uarridx_t id, nextId;

	duk_require_function(ctx, 0);
	interval = duk_require_uint(ctx, 1);
	num_args = duk_get_top(ctx) - 2;
	/* [ func uint arg1 ... argN ] */
	duk_push_undefined(ctx);
	duk_replace(ctx, 1);
	duk_push_string(ctx, "bind");
	duk_insert(ctx, 1);
	/* [ func "bind" undef arg1 ... argN ] */
	duk_call_prop(ctx, 0, 1 + num_args);
	/* [ func bound_func ] */
	duk_push_fixed_buffer(ctx, sizeof(dux_timer_t));
	duk_push_buffer_object(ctx, -1, 0, sizeof(dux_timer_t), DUK_BUFOBJ_DUKTAPE_BUFFER);
	timer = (dux_timer_t *)duk_get_buffer_data(ctx, -1, NULL);
	/* [ func bound_func buf bufobj ] */
	duk_replace(ctx, 0);
	duk_pop(ctx);
	/* [ bufobj bound_func ] */
	duk_put_prop_string(ctx, -2, "func");
	/* [ bufobj ] */
	timer_push_array(ctx);
	/* [ bufobj arr ] */
	duk_get_prop_index(ctx, -1, 0);
	id = duk_get_uint(ctx, -1);
	duk_pop(ctx);
	duk_swap_top(ctx, 0);
	/* [ arr bufobj ] */

	timer->id = id;
	timer->flags = flags;
	timer->time_start = timer->time_prev = timer_current();
	timer->time_next = timer->time_start + interval;
	timer->interval = interval;

	for (nextId = id + 1;; ++nextId)
	{
		if (!duk_has_prop_index(ctx, 0, nextId))
		{
			duk_push_uint(ctx, nextId);
			duk_put_prop_index(ctx, 0, 0);
			break;
		}
	}

	duk_put_prop_index(ctx, 0, id);
	/* [ arr ] */
	duk_push_uint(ctx, id);
	/* [ arr uint ] */
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
	/* [ uint ] */
	duk_uarridx_t id = duk_require_uint(ctx, 0);
	duk_uarridx_t nextId;
	dux_timer_t *timer;

	if (id == 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	timer_push_array(ctx);
	/* [ uint arr ] */
	if (duk_get_prop_index(ctx, -1, id))
	{
		/* [ uint arr bufobj ] */
		timer = (dux_timer_t *)duk_get_buffer_data(ctx, -1, NULL);

		if ((timer->flags ^ flags) & DUX_TIMER_ONESHOT)
		{
			/* Interval/Timeout mismatch */
			duk_pop_3(ctx);
			/* [  ] */
			return DUK_RET_TYPE_ERROR;
		}

		duk_pop(ctx);
		/* [ uint arr ] */
		duk_del_prop_index(ctx, -1, id);

		duk_get_prop_index(ctx, -1, 0);
		/* [ uint arr uint ] */
		nextId = duk_get_uint(ctx, -1);
		if (id < nextId)
		{
			duk_push_uint(ctx, id);
			/* [ uint arr uint uint ] */
			duk_put_prop_index(ctx, -3, 0);
			/* [ uint arr uint ] */
		}

		duk_pop_3(ctx);
		/* [  ] */
		return 0;	/* return undefined */
	}

	/* Not found */
	duk_pop_2(ctx);
	/* [  ] */
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
}

/*
 * Tick handler
 */
duk_uint_t dux_timer_tick(duk_context *ctx)
{
	/* [ ... ] */
	alt_u32 tick = timer_current();
	dux_timer_t *timer;
	duk_uarridx_t id;
	duk_size_t length;
	duk_uint_t processed = 0;

	timer_push_array(ctx);
	/* [ ... arr ] */

	length = duk_get_length(ctx, -1);
	for (id = 1; id < length; ++id)
	{
		/* [ ... arr ] */
		if (!duk_get_prop_index(ctx, -1, id))
		{
			/* [ ... arr undefined ] */
			duk_pop(ctx);
			/* [ ... arr ] */
			continue;
		}

		++processed;

		/* [ ... arr bufobj ] */
		timer = (dux_timer_t *)duk_get_buffer_data(ctx, -1, NULL);

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
				/* [ ... arr ] */
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
				/* [ ... arr ] */
				continue;
			}
		}

		/* Expires */
		duk_get_prop_string(ctx, -1, "func");
		/* [ ... arr bufobj func ] */
		if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS)
		{
			/* [ ... arr bufobj err ] */
			top_level_error(ctx);
		}
		duk_pop_2(ctx);
		/* [ ... arr ] */

		if (timer->flags & DUX_TIMER_ONESHOT)
		{
			duk_push_uint(ctx, id);
			/* [ ... arr uint ] */
			timer_clearTimeout(ctx);
			/* [ ... arr ] */
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

