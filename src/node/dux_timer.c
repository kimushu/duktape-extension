#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_TIMER)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_IPK_TIMER[] = DUX_IPK("Timer");

#define MAX_ID(len)     ((len)>>1)
#define FREE_IDX        (0)
#define DESC_IDX(id)    ((id)*2+0)
#define FUNC_IDX(id)    ((id)*2+1)

/*
 * Push timer array
 */
DUK_LOCAL void timer_push_array(duk_context *ctx)
{
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_IPK_TIMER))
	{
		duk_pop(ctx);
		/* [ ... stash ] */
		duk_push_array(ctx);
		duk_push_uint(ctx, 1);
		duk_put_prop_index(ctx, -2, FREE_IDX);
		/* [ ... stash arr ] */
		duk_dup_top(ctx);
		/* [ ... stash arr arr ] */
		duk_put_prop_string(ctx, -3, DUX_IPK_TIMER);
		/* [ ... stash arr ] */
	}
	/* [ ... stash arr ] */
	duk_remove(ctx, -2);
	/* [ ... arr ] */
}

/*
 * Common implementation of setInterval/setTimeout
 */
DUK_LOCAL duk_ret_t timer_set(duk_context *ctx, duk_uint_t flags)
{
	duk_uint_t interval;
	duk_idx_t nargs;
	dux_timer_desc *desc;
	duk_uarridx_t id, nextId;

	/* [ func uint arg1 ... argN ] */
	duk_require_callable(ctx, 0);
	interval = duk_require_uint(ctx, 1);
	duk_remove(ctx, 1);
	/* [ func arg1 ... argN ] */
	nargs = duk_get_top(ctx) - 1;
	if (nargs > 0)
	{
		dux_bind_arguments(ctx, nargs);
		/* [ bound_func ] */
	}
	/* [ func ] */
	duk_push_fixed_buffer(ctx, sizeof(dux_timer_desc));
	/* [ func buf ] */
	desc = (dux_timer_desc *)duk_get_buffer(ctx, 1, NULL);
	timer_push_array(ctx);
	/* [ func buf arr ] */
	duk_swap(ctx, 0, 2);
	/* [ arr buf func ] */
	duk_get_prop_index(ctx, 0, FREE_IDX);
	/* [ arr buf func uint ] */
	id = duk_get_uint(ctx, 3);
	duk_pop(ctx);
	/* [ arr buf func ] */

	/* Construct timer handle */
	desc->id = id;
	desc->flags = flags;
	desc->time_start = desc->time_prev = dux_timer_arch_current();
	desc->time_next = desc->time_start + interval;
	desc->interval = interval;

	for (nextId = id + 1;; ++nextId)
	{
		if (!duk_has_prop_index(ctx, 0, DESC_IDX(nextId)))
		{
			duk_push_uint(ctx, nextId);
			duk_put_prop_index(ctx, 0, FREE_IDX);
			break;
		}
	}

	/* [ arr buf func ] */
	duk_put_prop_index(ctx, 0, FUNC_IDX(id));
	/* [ arr buf ] */
	duk_put_prop_index(ctx, 0, DESC_IDX(id));
	/* [ arr ] */
	duk_push_uint(ctx, id);
	/* [ arr uint ] */
	return 1; /* return uint */
}

/*
 * Entry for global.setInterval()
 */
DUK_LOCAL duk_ret_t timer_setInterval(duk_context *ctx)
{
	return timer_set(ctx, 0);
}

/*
 * Entry for global.setTimeout()
 */
DUK_LOCAL duk_ret_t timer_setTimeout(duk_context *ctx)
{
	return timer_set(ctx, DUX_TIMER_ONESHOT);
}

/*
 * Common implementation of clearInterval/clearTimeout
 * Note: This function may be called from dux_timer_tick
 */
DUK_LOCAL duk_ret_t timer_clear(duk_context *ctx, duk_uint_t flags)
{
	duk_uarridx_t id;
	duk_uarridx_t free_id;
	dux_timer_desc *desc;

	/* [ ... uint ] */
	id = duk_require_uint(ctx, -1);
	if (id == 0)
	{
		return DUK_RET_RANGE_ERROR;
	}

	timer_push_array(ctx);
	/* [ ... uint arr ] */
	if (!duk_get_prop_index(ctx, -1, DESC_IDX(id)))
	{
		/* Not found */
		return DUK_RET_RANGE_ERROR;
	}

	/* [ ... uint arr buf ] */
	desc = (dux_timer_desc *)duk_get_buffer(ctx, -1, NULL);
	if (!desc)
	{
		return DUK_RET_RANGE_ERROR;
	}

	if ((desc->flags ^ flags) & DUX_TIMER_ONESHOT)
	{
		/* Interval/Timeout mismatch */
		return DUK_RET_TYPE_ERROR;
	}

	duk_del_prop_index(ctx, -2, DESC_IDX(id));
	duk_del_prop_index(ctx, -2, FUNC_IDX(id));

	duk_get_prop_index(ctx, -2, FREE_IDX);
	/* [ ... uint arr buf uint ] */
	free_id = duk_get_uint(ctx, -1);
	if (id < free_id)
	{
		duk_push_uint(ctx, id);
		/* [ ... uint arr buf uint uint ] */
		duk_put_prop_index(ctx, -4, FREE_IDX);
		/* [ ... uint arr buf uint ] */
	}
	return 0; /* return undefined */
}

/*
 * Entry of global.clearInterval
 */
DUK_LOCAL duk_ret_t timer_clearInterval(duk_context *ctx)
{
	return timer_clear(ctx, 0);
}

/*
 * Entry of global.clearTimeout
 */
DUK_LOCAL duk_ret_t timer_clearTimeout(duk_context *ctx)
{
	return timer_clear(ctx, DUX_TIMER_ONESHOT);
}

/*
 * List of timer functions
 */
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
DUK_INTERNAL duk_errcode_t dux_timer_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_del_prop_string(ctx, -1, DUX_IPK_TIMER);
	duk_push_global_object(ctx);
	/* [ ... stash global ] */
	duk_put_function_list(ctx, -1, timer_funcs);
	duk_pop_2(ctx);
	/* [ ... ] */
	dux_timer_arch_init();
	return DUK_ERR_NONE;
}

/*
 * Tick handler for Timers
 */
DUK_INTERNAL duk_int_t dux_timer_tick(duk_context *ctx)
{
	/* [ ... ] */
	duk_uint_t tick = dux_timer_arch_current();
	duk_int_t result = DUX_TICK_RET_JOBLESS;
	dux_timer_desc *desc;
	duk_uarridx_t id, free_id;
	duk_size_t max_id;

	timer_push_array(ctx);
	/* [ ... arr ] */

	max_id = MAX_ID(duk_get_length(ctx, -1));
	for (id = 1; id <= max_id; ++id)
	{
		/* [ ... arr ] */
		desc = NULL;
		if (duk_get_prop_index(ctx, -1, DESC_IDX(id)))
		{
			desc = (dux_timer_desc *)duk_get_buffer(ctx, -1, NULL);
		}
		duk_pop(ctx);
		if (!desc)
		{
			continue;
		}

		result = DUX_TICK_RET_CONTINUE;
		if (!(desc->flags & DUX_TIMER_STARTED)) {
			desc->flags |= DUX_TIMER_STARTED;
			continue;
		}
		if (desc->time_prev <= desc->time_next)
		{
			/*
			 * No roll-over (P<N)
			 *
			 *            <------>             (continue)
			 * [0.........P.......N.........M] (P=prev,N=next,M=max)
			 *  --------->        <----------  (expire)
			 *
			 * --- or ---
			 *
			 * No interval (P==N)
			 * 
			 * [ 0..........P==N...........M ] (P=prev,N=next,M=max)
			 *   <------------------------->   (expire)
			 */
			if ((desc->time_prev <= tick) && (tick < desc->time_next))
			{
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
			if ((tick < desc->time_next) || (desc->time_prev <= tick))
			{
				continue;
			}
		}

		/* Expires */
		duk_get_prop_index(ctx, -1, FUNC_IDX(id));
		/* [ ... arr func ] */
		if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS)
		{
			/* [ ... arr err ] */
			dux_report_error(ctx);
		}
		duk_pop(ctx);
		/* [ ... arr ] */

		if (desc->flags & DUX_TIMER_ONESHOT)
		{
			duk_del_prop_index(ctx, -1, DESC_IDX(id));
			duk_del_prop_index(ctx, -1, FUNC_IDX(id));

			duk_get_prop_index(ctx, -1, FREE_IDX);
			/* [ ... arr uint ] */
			free_id = duk_get_uint(ctx, -1);
			if (id < free_id)
			{
				duk_push_uint(ctx, id);
				/* [ ... arr uint uint ] */
				duk_put_prop_index(ctx, -3, FREE_IDX);
				/* [ ... arr uint ] */
			}
			duk_pop(ctx);
			/* [ ... arr ] */
		}
		else
		{
			desc->time_next += desc->interval;
		}
	}

	duk_pop(ctx);
	/* [ ... ] */
	return result;
}

#undef MAX_ID
#undef FREE_IDX
#undef DESC_IDX
#undef FUNC_IDX

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_TIMER */
