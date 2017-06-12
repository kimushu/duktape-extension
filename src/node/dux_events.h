#ifndef DUX_EVENTS_H_INCLUDED
#define DUX_EVENTS_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_EVENTS)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_events_init(duk_context *ctx);
#define DUX_INIT_EVENTS     dux_events_init,
#define DUX_TICK_EVENTS

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_EVENTS */

#define DUX_INIT_EVENTS
#define DUX_TICK_EVENTS

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_EVENTS */
#endif  /* !DUX_EVENTS_H_INCLUDED */
