#ifndef DUX_TIMER_H_INCLUDED
#define DUX_TIMER_H_INCLUDED

/*
 * Constants
 */

enum
{
	DUX_TIMER_ONESHOT = (1 << 0),
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

#endif /* DUX_TIMER_H_INCLUDED */
