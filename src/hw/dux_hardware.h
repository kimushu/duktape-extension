#ifndef DUX_HARDWARE_H_INCLUDED
#define DUX_HARDWARE_H_INCLUDED

#if !defined(DUX_OPT_NO_HARDWARE_MODULES)

#include "dux_hardware_io.h"
#include "dux_paraio.h"
#include "dux_i2ccon.h"
#include "dux_spicon.h"

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_hardware_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_hardware_tick(duk_context *ctx);
#define DUX_INIT_HARDWARE   dux_hardware_init,
#define DUX_TICK_HARDWARE   dux_hardware_tick,

#else   /* !DUX_OPT_NO_HARDWARE_MODULES */

#define DUX_INIT_HARDWARE
#define DUX_TICK_HARDWARE

#endif  /* DUX_OPT_NO_HARDWARE_MODULES */
#endif  /* !DUX_HARDWARE_H_INCLUDED */
