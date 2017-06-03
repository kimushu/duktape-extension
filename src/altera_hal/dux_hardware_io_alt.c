#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && defined(__nios2__) && defined(__hal__)

#include "../dux_internal.h"
#include <io.h>

DUK_INTERNAL duk_uint_t dux_hardware_read(volatile const duk_uint_t *ptr)
{
	return IORD_32DIRECT(ptr, 0);
}

DUK_INTERNAL void dux_hardware_write(volatile duk_uint_t *ptr, duk_uint_t val)
{
	IOWR_32DIRECT(ptr, 0, val);
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && __nios2__ && __hal__ */
