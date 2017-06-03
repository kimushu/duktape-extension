#ifndef DUX_PROMISE_H_INCLUDED
#define DUX_PROMISE_H_INCLUDED

#if !defined(DUX_OPT_NO_PROMISE)

/*
 * Functions
 */
DUK_INTERNAL_DECL duk_errcode_t dux_promise_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_promise_tick(duk_context *ctx);

DUK_INTERNAL_DECL void dux_promise_get_cb_with_bool(duk_context *ctx, duk_idx_t func_idx);

#else   /* !DUX_OPT_NO_PROMISE */

#define dux_promise_init(ctx)   (DUK_ERR_NONE)
#define dux_promise_tick(ctx)   (DUX_TICK_RET_JOBLESS)

DUK_LOCAL
DUK_INLINE void dux_promise_get_cb_with_bool(dux_context *ctx, duk_idx_t func_idx)
{
	if (!duk_is_null_or_undefined(ctx, func_idx))
	{
		duk_require_callable(ctx, func_idx);
	}
	duk_push_undefined(ctx);
}

#endif  /* DUX_OPT_NO_PROMISE */
#endif  /* !DUX_PROMISE_H_INCLUDED */
