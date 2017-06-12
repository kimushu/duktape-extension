#ifndef DUX_IMMEDIATE_H_INCLUDED
#define DUX_IMMEDIATE_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_IMMEDIATE)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_immediate_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_immediate_tick(duk_context *ctx);
#define DUX_INIT_IMMEDIATE  dux_immediate_init,
#define DUX_TICK_IMMEDIATE  dux_immediate_tick,

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_IMMEDIATE */

#define DUX_INIT_IMMEDIATE
#define DUX_TICK_IMMEDIATE

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_IMMEDIATE */
#endif  /* !DUX_IMMEDIATE_H_INCLUDED */
