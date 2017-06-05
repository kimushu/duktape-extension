#if !defined(DUX_OPT_NO_TIMER) && defined(__WIN32__)

#include "../dux_internal.h"
#include <windows.h>

DUK_INTERNAL void dux_timer_arch_init(void)
{
    timeBeginPeriod(1);
}

DUK_INTERNAL duk_uint_t dux_timer_arch_current(void)
{
    return timeGetTime();
}

#endif  /* !DUX_OPT_NO_TIMER && __WIN32__ */
