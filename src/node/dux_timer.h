#ifndef DUX_TIMER_H_INCLUDED
#define DUX_TIMER_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_TIMER)

/*
 * Constants
 */

enum
{
	DUX_TIMER_STARTED = (1 << 0),
	DUX_TIMER_ONESHOT = (1 << 1),
	DUX_TIMER_UNREF   = (1 << 2),
};

/*
 * Structures
 */

typedef struct dux_timer_desc
{
	duk_uint_t id;
	duk_uint_t flags;

	duk_uint_t interval;
	duk_uint_t time_start;
	duk_uint_t time_prev;
	duk_uint_t time_next;
}
dux_timer_desc;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_timer_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_timer_tick(duk_context *ctx);
#define DUX_INIT_TIMER  dux_timer_init,
#define DUX_TICK_TIMER  dux_timer_tick,

DUK_INTERNAL_DECL void dux_timer_arch_init(void);
DUK_INTERNAL_DECL duk_uint_t dux_timer_arch_current(void);

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_TIMER */

#define DUX_INIT_TIMER
#define DUX_TICK_TIMER

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_TIMER */
#endif  /* !DUX_TIMER_H_INCLUDED */
