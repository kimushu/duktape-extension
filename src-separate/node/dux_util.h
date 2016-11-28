#ifndef DUX_UTIL_H_INCLUDED
#define DUX_UTIL_H_INCLUDED

#if !defined(DUX_OPT_NO_UTIL)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_util_init(duk_context *ctx);
#define dux_util_tick(ctx)  (DUX_TICK_RET_JOBLESS)

DUK_INTERNAL_DECL duk_ret_t dux_util_format(duk_context *ctx);

#else   /* !DUX_OPT_NO_UTIL */

#define dux_util_init(ctx)  (DUK_ERR_NONE)
#define dux_util_tick(ctx)  (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_UTIL */
#endif  /* !DUX_UTIL_H_INCLUDED */
