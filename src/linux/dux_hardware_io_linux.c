#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && defined(__linux__)

#include "../dux_internal.h"

DUK_INTERNAL duk_uint_t dux_hardware_read(volatile const duk_uint_t *ptr)
{
	return *ptr;
}

DUK_INTERNAL void dux_hardware_write(volatile duk_uint_t *ptr, duk_uint_t val)
{
	*ptr = val;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && __linux__ */
