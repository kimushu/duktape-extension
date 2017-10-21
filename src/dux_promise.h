#ifndef DUX_PROMISE_H_INCLUDED
#define DUX_PROMISE_H_INCLUDED

#if !defined(DUX_OPT_NO_PROMISE)

/*
 * Functions
 */
DUK_INTERNAL_DECL duk_errcode_t dux_promise_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_promise_tick(duk_context *ctx);
#define DUX_INIT_PROMISE    dux_promise_init,
#define DUX_TICK_PROMISE    dux_promise_tick,

DUK_INTERNAL_DECL void dux_promise_new(duk_context *ctx);
DUK_INTERNAL_DECL void dux_promise_new_with_node_callback(duk_context *ctx, duk_idx_t func_idx);

#else   /* !DUX_OPT_NO_PROMISE */

#define DUX_INIT_PROMISE
#define DUX_TICK_PROMISE

#endif  /* DUX_OPT_NO_PROMISE */
#endif  /* !DUX_PROMISE_H_INCLUDED */
