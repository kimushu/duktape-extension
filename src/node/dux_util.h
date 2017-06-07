#ifndef DUX_UTIL_H_INCLUDED
#define DUX_UTIL_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_UTIL)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_util_init(duk_context *ctx);
#define DUX_INIT_UTIL   dux_util_init,
#define DUX_TICK_UTIL

DUK_INTERNAL_DECL duk_ret_t dux_util_format(duk_context *ctx);

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_UTIL */

#define DUX_INIT_UTIL
#define DUX_TICK_UTIL

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_UTIL */
#endif  /* !DUX_UTIL_H_INCLUDED */
