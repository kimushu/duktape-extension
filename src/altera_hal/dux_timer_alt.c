#if !defined(DUX_OPT_NO_TIMER) && defined(__nios2__) && defined(__hal__)

#include "../dux_internal.h"
#include <sys/alt_alarm.h>

/*
 * Tick counter can be shared with multiple Duktape heaps
 * because there is only one HAL tick in one system.
 */
DUK_LOCAL duk_uint_t g_time_per_tick;
DUK_LOCAL duk_uint_t g_last_time;
DUK_LOCAL alt_u32 g_last_tick;

/*
 * Initialize time getter
 */
DUK_INTERNAL void dux_timer_arch_init(void)
{
	g_time_per_tick = alt_ticks_per_second() / 1000;
}

/*
 * Get current time (No Duktape dependent)
 */
DUK_INTERNAL duk_uint_t dux_timer_arch_current(void)
{
	alt_u32 new_tick = alt_nticks();
	alt_u32 ticks = new_tick - g_last_tick;
	g_last_tick = new_tick;
	return g_last_time += g_time_per_tick * ticks;
}

#endif  /* !DUX_OPT_NO_TIMER && __nios2__ && __hal__ */
