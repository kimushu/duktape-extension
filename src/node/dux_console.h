#ifndef DUX_CONSOLE_H_INCLUDED
#define DUX_CONSOLE_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_CONSOLE)

DUK_INTERNAL_DECL duk_errcode_t dux_console_init(duk_context *ctx);
#define DUX_INIT_CONSOLE    dux_console_init,
#define DUX_TICK_CONSOLE

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_CONSOLE */

#define DUX_INIT_CONSOLE
#define DUX_TICK_CONSOLE

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_CONSOLE */
#endif  /* !DUX_CONSOLE_H_INCLUDED */
