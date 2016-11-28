#ifndef DUX_CONSOLE_H_INCLUDED
#define DUX_CONSOLE_H_INCLUDED

#if !defined(DUX_OPT_NO_CONSOLE)

DUK_INTERNAL_DECL duk_errcode_t dux_console_init(duk_context *ctx);
#define dux_console_tick(ctx)   (DUX_TICK_RET_JOBLESS)

#else   /* !DUX_OPT_NO_CONSOLE */

#define dux_console_init(ctx)   (DUK_ERR_NONE)
#define dux_console_tick(ctx)   (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_CONSOLE */
#endif  /* !DUX_CONSOLE_H_INCLUDED */
