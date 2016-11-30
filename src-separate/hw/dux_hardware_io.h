#ifndef DUX_HARDWARE_IO_H_INCLUDED
#define DUX_HARDWARE_IO_H_INCLUDED
#if !defined(DUX_OPT_NO_HARDWARE_MODULES)

DUK_INTERNAL_DECL duk_uint_t dux_hardware_read(volatile const duk_uint_t *ptr);
DUK_INTERNAL_DECL void dux_hardware_write(volatile duk_uint_t *ptr, duk_uint_t val);

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES */
#endif  /* !DUX_HARDWARE_IO_H_INCLUDED */
