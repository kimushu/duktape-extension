#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

#if defined(DUX_ENABLE_PACKAGE_DELAY) && !defined(DUX_OPT_NO_PROMISE)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_package_delay(duk_context *ctx);
#define DUX_INIT_PACKAGE_DELAY      dux_package_delay,

#else   /* !DUX_ENABLE_PACKAGE_DELAY || DUX_OPT_NO_PROMISE */

#define DUX_INIT_PACKAGE_DELAY

#endif  /* !DUX_ENABLE_PACKAGE_DELAY || DUX_OPT_NO_PROMISE */
#endif  /* !DELAY_H_INCLUDED */
