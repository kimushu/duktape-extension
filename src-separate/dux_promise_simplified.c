/*
 * ECMA objects:
 *    class Promise {
 *      constructor(<Function> executor) {
 *      }
 *
 *      then(onFulfilled, onRejected) {
 *        return <Promise>;
 *      }
 *
 *      catch(onRejected) {
 *        return <Promise>;
 *      }
 *
 *      static resolve(value) {
 *        return <Promise>;
 *      }
 *
 *      static reject(value) {
 *        return <Promise>;
 *      }
 *
 *      static all(<Promise[]>) {
 *        return <Promise>;
 *      }
 *
 *      static race(<Promise[]>) {
 *        return <Promise>;
 *      }
 *    }
 *
 *  Promise states:
 *    <Pending>:
 *      Without [[Value]] property
 *      With [[FulfillReactions]] property (array)
 *      With [[RejectReactions]] property (array)
 *
 *    <Resolved>:
 *      With [[Value]] property (Value)
 *      With [[FulfillReactions]] property (undefined)
 *      Without [[RejectReactions]] property
 *
 *    <Rejected>:
 *      With [[Value]] property (Reason)
 *      Without [[FulfillReactions]] property
 *      With [[RejectReactions]] property (undefined)
 */

#if !defined(DUX_OPT_NO_PROMISE) && !defined(DUX_OPT_STANDARD_PROMISE)
#include "dux_internal.h"

DUK_LOCAL const char DUX_IPK_PROMISE_CALLBACKS[]          = DUX_IPK("pmC");
DUK_LOCAL const char DUX_IPK_PROMISE_VALUE[]              = DUX_IPK("pmV");
DUK_LOCAL const char DUX_IPK_PROMISE_FULFILL_REACTIONS[]  = DUX_IPK("pmF");
DUK_LOCAL const char DUX_IPK_PROMISE_REJECT_REACTIONS[]   = DUX_IPK("pmR");

DUK_LOCAL_DECL duk_ret_t promise_on_resolved(duk_context *ctx);
DUK_LOCAL_DECL duk_ret_t promise_on_rejected(duk_context *ctx);

/*
 * Invoke executor (function(resolve,reject){})
 */
DUK_LOCAL duk_ret_t promise_invoke_executor(duk_context *ctx)
{
	/* [ executor promise ] */
	duk_push_c_function(ctx, promise_on_resolved, 2);
	/* [ executor promise func(resolver) ] */
	duk_dup(ctx, 1);
	dux_bind_arguments(ctx, 1);
	/* [ executor promise bound_func(resolver) ] */
	duk_push_c_function(ctx, promise_on_rejected, 2);
	/* [ executor promise bound_func(resolver) func(rejector):3 ] */
	duk_dup(ctx, 1);
	dux_bind_arguments(ctx, 1);
	/* [ executor promise bound_func(resolver) bound_func(rejector):3 ] */
	duk_swap(ctx, 0, 1);
	/* [ promise executor bound_func(resolver) bound_func(rejector):3 ] */

	if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS)
	{
		/* [ promise err ] */
		return promise_on_rejected(ctx);
	}
	else
	{
		/* [ promise retval ] */
		return 0; /* return undefined; */
	}
}

/*
 * Invoker of reactions
 * Note: The promise at stack[0] must be preserved for promise_resolve()/promise_reject()
 */
DUK_LOCAL duk_ret_t promise_on_settled(duk_context *ctx,
                                       const char *key_queued, const char *key_removed)
{
	duk_size_t reactions;
	duk_uarridx_t ridx, cidx;

	/* [ promise value/reason ] */
	duk_put_prop_string(ctx, 0, DUX_IPK_PROMISE_VALUE);
	/* [ promise ] */
	duk_del_prop_string(ctx, 0, key_removed);
	duk_get_prop_string(ctx, 0, key_queued);
	/* [ promise arr(reactions) ] */
	reactions = duk_get_length(ctx, 1);
	duk_push_undefined(ctx);
	duk_put_prop_string(ctx, 0, key_queued);
	/* [ promise arr(reactions) ] */

	if (reactions == 0)
	{
		/* No reactions to invoke */
		return 0; /* return undefined; */
	}

	duk_push_heap_stash(ctx);
	/* [ promise arr(reactions) stash ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PROMISE_CALLBACKS);
	/* [ promise arr(reactions) stash arr(callbacks):3 ] */
	cidx = duk_get_length(ctx, 3);

	for (ridx = 0; ridx < reactions; ++ridx, ++cidx)
	{
		duk_get_prop_index(ctx, 1, ridx);
		duk_put_prop_index(ctx, 3, cidx);
	}
	/* [ promise arr(reactions) stash arr(callbacks):3 ] */
	return 0; /* return undefined; */
}

/*
 * Handler for resolved
 * Note: The promise at stack[0] must be preserved for promise_resolve()
 */
DUK_LOCAL duk_ret_t promise_on_resolved(duk_context *ctx)
{
	/* [ promise value ] */
	if (duk_has_prop_string(ctx, 0, DUX_IPK_PROMISE_VALUE))
	{
		/*
		 * This promise already settled
		 */
		return 0; /* return undefined; */
	}

	if (!duk_is_object_coercible(ctx, 1))
	{
		goto resolve;
	}

	if (duk_get_prop_string(ctx, 1, DUX_IPK_PROMISE_VALUE))
	{
		/* [ promise value value/reason ] */

		if (duk_has_prop_string(ctx, 1, DUX_IPK_PROMISE_REJECT_REACTIONS))
		{
			/*
			 * value is a Promise object which has been already rejected
			 */
			duk_remove(ctx, 1);
			/* [ promise reason ] */
			return promise_on_settled(ctx,
					DUX_IPK_PROMISE_REJECT_REACTIONS,
					DUX_IPK_PROMISE_FULFILL_REACTIONS);
		}

		/*
		 * value is a Promise object which has been already resolved
		 */
		duk_remove(ctx, 1);
		/* [ promise value ] */
		goto resolve;
	}
	duk_pop(ctx);
	/* [ promise value ] */

	if (duk_get_prop_string(ctx, 1, DUX_IPK_PROMISE_FULFILL_REACTIONS))
	{
		/* [ promise value arr(reactions) ] */

		/*
		 * value is a Promise object which is pending
		 */
		duk_push_c_function(ctx, promise_on_resolved, 2);
		duk_dup(ctx, 0);
		duk_dup(ctx, 1);
		dux_bind_arguments(ctx, 2);
		/* [ promise value arr(reactions) bound_func:3 ] */
		duk_get_prop_string(ctx, 1, DUX_IPK_PROMISE_REJECT_REACTIONS);
		/* [ promise value arr(reactions) bound_func:3 arr(rReactions):4 ] */
		duk_dup(ctx, 3);
		duk_put_prop_index(ctx, 4, duk_get_length(ctx, 4));
		duk_pop(ctx);
		/* [ promise value arr(reactions) bound_func:3 ] */
		duk_put_prop_index(ctx, 2, duk_get_length(ctx, 2));
		/* [ promise value ] */
		return 0; /* return undefined; */
	}
	duk_pop(ctx);
	/* [ promise value ] */

	if (duk_get_prop_string(ctx, 1, "then") && duk_is_callable(ctx, 2))
	{
		/* [ promise value func(then) ] */

		/*
		 * value is a Thenable object
		 */
		duk_swap(ctx, 1, 2);
		/* [ promise func(then) value ] */
		dux_bind_this_arguments(ctx, 0);
		/* [ promise bound_func(then) ] */
		duk_push_c_function(ctx, promise_invoke_executor, 2);
		/* [ promise bound_func(then) func ] */
		duk_dup(ctx, 0);
		/* [ promise bound_func(then) func promise ] */
		duk_swap(ctx, 1, 2);
		/* [ promise func bound_func(then) promise ] */
		dux_bind_arguments(ctx, 2);
		/* [ promise bound_func ] */
		duk_push_heap_stash(ctx);
		/* [ promise bound_func stash ] */
		duk_get_prop_string(ctx, 2, DUX_IPK_PROMISE_CALLBACKS);
		/* [ promise bound_func stash arr(callbacks) ] */
		duk_swap(ctx, 1, 3);
		/* [ promise arr(callbacks) stash bound_func ] */
		duk_put_prop_index(ctx, 1, duk_get_length(ctx, 1));
		/* [ promise arr(callbacks) stash ] */
		return 0; /* return undefined; */
	}
	duk_pop(ctx);
	/* [ promise value ] */

resolve:
	return promise_on_settled(ctx,
			DUX_IPK_PROMISE_FULFILL_REACTIONS,
			DUX_IPK_PROMISE_REJECT_REACTIONS);
}

/*
 * Handler for rejected
 * Note: The promise at stack[0] must be preserved for promise_reject()
 */
DUK_LOCAL duk_ret_t promise_on_rejected(duk_context *ctx)
{
	/* [ promise reason ] */
	if (duk_has_prop_string(ctx, 0, DUX_IPK_PROMISE_VALUE))
	{
		/*
		 * This promise already settled
		 */
		return 0; /* return undefined; */
	}

	return promise_on_settled(ctx,
			DUX_IPK_PROMISE_REJECT_REACTIONS,
			DUX_IPK_PROMISE_FULFILL_REACTIONS);
}

/*
 * Handler for chain resolving
 */
DUK_LOCAL duk_ret_t promise_chain_resolver(duk_context *ctx)
{
	/* [ child onSettled parent ] */
	duk_get_prop_string(ctx, 2, DUX_IPK_PROMISE_VALUE);
	/* [ child onSettled parent value/reason ] */
	duk_replace(ctx, 2);
	/* [ child onSettled value/reason ] */
	if (duk_pcall(ctx, 1) == DUK_EXEC_SUCCESS)
	{
		/* [ child retval ] */
		return promise_on_resolved(ctx);
	}
	else
	{
		/* [ child err ] */
		return promise_on_rejected(ctx);
	}
}

/*
 * Entry of Promise.prototype.then()
 */
DUK_LOCAL duk_ret_t promise_proto_then(duk_context *ctx)
{
	/* [ onFulfilled onRejected ] */
	duk_push_object(ctx);
	duk_push_this(ctx);
	/* [ onFulfilled onRejected obj this:3 ] */
	duk_get_prototype(ctx, 3);
	duk_set_prototype(ctx, 2);
	/* [ onFulfilled onRejected new_promise this:3 ] */
	duk_push_array(ctx);
	duk_put_prop_string(ctx, 2, DUX_IPK_PROMISE_FULFILL_REACTIONS);
	duk_push_array(ctx);
	duk_put_prop_string(ctx, 2, DUX_IPK_PROMISE_REJECT_REACTIONS);

	if (duk_has_prop_string(ctx, 3, DUX_IPK_PROMISE_VALUE))
	{
		/* [ onFulfilled onRejected new_promise this:3 ] */

		/*
		 * This promise has been already settled
		 */
		duk_push_heap_stash(ctx);
		/* [ onFulfilled onRejected new_promise this:3 stash:4 ] */
		duk_get_prop_string(ctx, 4, DUX_IPK_PROMISE_CALLBACKS);
		/* [ onFulfilled onRejected new_promise this:3 stash:4 arr:5 ] */
		duk_push_c_function(ctx, promise_chain_resolver, 3);
		duk_replace(ctx, 4);
		/* [ onFulfilled onRejected new_promise this:3 func:4 arr:5 ] */

		if (duk_has_prop_string(ctx, 3, DUX_IPK_PROMISE_FULFILL_REACTIONS))
		{
			/* Already resolved */
			duk_swap(ctx, 0, 1);
			/* [ onRejected onFulfilled new_promise this:3 func:4 arr:5 ] */
		}
		duk_replace(ctx, 0);
		/* [ arr onSettled new_promise this:3 func:4 ] */
		duk_dup(ctx, 2);
		duk_dup(ctx, 1);
		duk_dup(ctx, 3);
		dux_bind_arguments(ctx, 3);
		/* [ arr onSettled new_promise this:3 bound_func ] */
		duk_put_prop_index(ctx, 0, duk_get_length(ctx, 0));
		/* [ arr onSettled new_promise this:3 ] */
		duk_pop(ctx);
		/* [ arr onSettled new_promise ] */
		return 1; /* return new_promise; */
	}
	/* [ onFulfilled onRejected new_promise this:3 ] */

	duk_get_prop_string(ctx, 3, DUX_IPK_PROMISE_FULFILL_REACTIONS);
	/* [ onFulfilled onRejected new_promise this:3 arr:4 ] */
	if (duk_is_null_or_undefined(ctx, 0))
	{
		duk_push_c_function(ctx, promise_on_resolved, 2);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 func:5 ] */
		duk_dup(ctx, 2);
		duk_dup(ctx, 3);
		dux_bind_arguments(ctx, 2);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 bound_func:5 ] */
	}
	else
	{
		duk_require_callable(ctx, 0);
		duk_push_c_function(ctx, promise_chain_resolver, 3);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 func:5 ] */
		duk_dup(ctx, 2);
		duk_dup(ctx, 0);
		duk_dup(ctx, 3);
		dux_bind_arguments(ctx, 3);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 bound_func:5 ] */
	}
	duk_put_prop_index(ctx, 4, duk_get_length(ctx, 4));
	/* [ onFulfilled onRejected new_promise this:3 arr:4 ] */
	duk_pop(ctx);
	/* [ onFulfilled onRejected new_promise this:3 ] */

	duk_get_prop_string(ctx, 3, DUX_IPK_PROMISE_REJECT_REACTIONS);
	/* [ onFulfilled onRejected new_promise this:3 arr:4 ] */
	if (duk_is_null_or_undefined(ctx, 1))
	{
		duk_push_c_function(ctx, promise_on_resolved, 2);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 func:5 ] */
		duk_dup(ctx, 2);
		duk_dup(ctx, 3);
		dux_bind_arguments(ctx, 2);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 bound_func:5 ] */
	}
	else
	{
		duk_require_callable(ctx, 1);
		duk_push_c_function(ctx, promise_chain_resolver, 3);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 func:5 ] */
		duk_dup(ctx, 2);
		duk_dup(ctx, 1);
		duk_dup(ctx, 3);
		dux_bind_arguments(ctx, 3);
		/* [ onFulfilled onRejected new_promise this:3 arr:4 bound_func:5 ] */
	}
	duk_put_prop_index(ctx, 4, duk_get_length(ctx, 4));
	/* [ onFulfilled onRejected new_promise this:3 arr:4 ] */
	duk_pop_2(ctx);
	/* [ onFulfilled onRejected new_promise ] */
	return 1; /* return new_promise; */
}

/*
 * Entry of Promise.prototype.catch()
 */
DUK_LOCAL duk_ret_t promise_proto_catch(duk_context *ctx)
{
	/* [ onRejected ] */
	duk_push_undefined(ctx);
	duk_swap(ctx, 0, 1);
	/* [ undefined onRejected ] */
	return promise_proto_then(ctx);
}

/*
 * Entry of Promise.all()
 */
DUK_LOCAL duk_ret_t promise_all(duk_context *ctx)
{
	return DUK_RET_UNSUPPORTED_ERROR;
}

/*
 * Entry of Promise.race()
 */
DUK_LOCAL duk_ret_t promise_race(duk_context *ctx)
{
	return DUK_RET_UNSUPPORTED_ERROR;
}

/*
 * Entry of Promise.resolve()
 */
DUK_LOCAL duk_ret_t promise_resolve(duk_context *ctx)
{
	duk_ret_t result;

	/* [ value ] */
	duk_push_object(ctx);
	duk_push_this(ctx);
	/* [ value obj constructor ] */
	duk_get_prop_string(ctx, 2, "prototype");
	duk_set_prototype(ctx, 1);
	/* [ value promise constructor ] */
	duk_pop(ctx);
	/* [ value promise ] */
	duk_swap(ctx, 0, 1);
	/* [ promise value ] */
	result = promise_on_resolved(ctx);
	if (result != 0)
	{
		return result;
	}
	/* [ promise ... ] */
	duk_set_top(ctx, 1);
	/* [ promise ] */
	return 1; /* return promise; */
}

/*
 * Entry of Promise.reject()
 */
DUK_LOCAL duk_ret_t promise_reject(duk_context *ctx)
{
	duk_ret_t result;

	/* [ reason ] */
	duk_push_object(ctx);
	duk_push_this(ctx);
	/* [ reason obj constructor ] */
	duk_get_prop_string(ctx, 2, "prototype");
	duk_set_prototype(ctx, 1);
	/* [ reason promise constructor ] */
	duk_pop(ctx);
	/* [ reason promise ] */
	duk_swap(ctx, 0, 1);
	/* [ promise reason ] */
	result = promise_on_rejected(ctx);
	if (result != 0)
	{
		return result;
	}
	/* [ promise ... ] */
	duk_set_top(ctx, 1);
	/* [ promise ] */
	return 1; /* return promise; */
}

/*
 * Entry of Promise constructor
 */
DUK_LOCAL duk_ret_t promise_constructor(duk_context *ctx)
{
	/* [ executor ] */

	if (!duk_is_constructor_call(ctx))
	{
		return DUK_RET_TYPE_ERROR;
	}
	duk_require_callable(ctx, 0);

	duk_push_this(ctx);
	/* [ executor this ] */
	duk_push_array(ctx);
	duk_put_prop_string(ctx, 1, DUX_IPK_PROMISE_FULFILL_REACTIONS);
	duk_push_array(ctx);
	duk_put_prop_string(ctx, 1, DUX_IPK_PROMISE_REJECT_REACTIONS);
	/* [ executor this ] */
	return promise_invoke_executor(ctx);
}

/*
 * List of static methods
 */
DUK_LOCAL const duk_function_list_entry promise_funcs[] = {
	{ "all", promise_all, DUK_VARARGS },
	{ "race", promise_race, DUK_VARARGS },
	{ "reject", promise_reject, 1 },
	{ "resolve", promise_resolve, 1 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype methods
 */
DUK_LOCAL const duk_function_list_entry promise_proto_funcs[] = {
	{ "then", promise_proto_then, 2 },
	{ "catch", promise_proto_catch, 1 },
	{ NULL, NULL, 0 }
};

/*
 * Initialize simplified promise
 */
DUK_INTERNAL duk_errcode_t dux_promise_init(duk_context *ctx)
{
	/* [ ... ] */
	dux_push_named_c_constructor(
			ctx, "Promise", promise_constructor, 1,
			promise_funcs, promise_proto_funcs, NULL, NULL);
	/* [ ... constructor ] */
	duk_push_heap_stash(ctx);
	/* [ ... constructor stash ] */
	duk_push_array(ctx);
	duk_put_prop_string(ctx, -2, DUX_IPK_PROMISE_CALLBACKS);
	/* [ ... constructor stash ] */
	duk_pop(ctx);
	/* [ ... constructor ] */
	duk_put_global_string(ctx, "Promise");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Tick handler for promises
 */
DUK_INTERNAL duk_int_t dux_promise_tick(duk_context *ctx)
{
	duk_int_t result = DUX_TICK_RET_JOBLESS;
	duk_size_t callbacks;
	duk_uarridx_t cidx;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (duk_get_prop_string(ctx, -1, DUX_IPK_PROMISE_CALLBACKS) &&
		((callbacks = duk_get_length(ctx, -1)) > 0))
	{
		/* [ ... stash arr ] */
		duk_push_array(ctx);
		duk_put_prop_string(ctx, -3, DUX_IPK_PROMISE_CALLBACKS);
		/* [ ... stash arr ] */

		result = DUX_TICK_RET_CONTINUE;
		for (cidx = 0; cidx < callbacks; ++cidx)
		{
			duk_get_prop_index(ctx, -1, cidx);
			/* [ ... stash arr func ] */
			if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS)
			{
				/* [ ... stash arr err ] */
				// TODO: crash?
				dux_report_error(ctx);
			}
			/* [ ... stash arr retval/err ] */
			duk_pop(ctx);
			/* [ ... stash arr ] */
		}
	}
	duk_pop_2(ctx);
	/* [ ... ] */
	return result;
}

#endif  /* !DUX_OPT_NO_PROMISE && !DUX_OPT_STANDARD_PROMISE */
