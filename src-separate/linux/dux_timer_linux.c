#if !defined(DUX_OPT_NO_TIMER) && defined(__linux__)

#include "../dux_internal.h"
#include <time.h>

DUK_INTERNAL duk_uint_t dux_timer_current(void)
{
	struct timespec tp;
	if (clock_gettime(CLOCK_MONOTONIC, &tp) != 0)
	{
		return 0;
	}
	return (duk_uint_t)(tp.tv_sec * 1000 + tp.tv_nsec / 1000000);
}

#endif  /* !DUX_OPT_NO_TIMER && __linux__ */
