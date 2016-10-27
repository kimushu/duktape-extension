#ifndef DUX_TIMER_H_
#define DUX_TIMER_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dux_timer_t
{
	duk_uint_t id;
	duk_uint_t flags;

	duk_uint_t interval;
	duk_uint_t time_start;
	duk_uint_t time_prev;
	duk_uint_t time_next;
}
dux_timer_t;

enum
{
	DUX_TIMER_ONESHOT = (1 << 0),
};

extern void dux_timer_init(duk_context *ctx);
extern duk_uint_t dux_timer_tick(duk_context *ctx);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_TIMER_H_ */
