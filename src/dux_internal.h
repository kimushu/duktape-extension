/*
 * Top level header file for internal source files
 */

#ifndef DUX_INTERNAL_H_INCLUDED
#define DUX_INTERNAL_H_INCLUDED

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DUX_VERSION_STRING  "0.1.0"

#if defined(__nios2__) && defined(__hal__)
#include <system.h>
#endif  /* __nios2__ && __hal__ */

/*
 * Sub modules
 */

#include "dux_basis.h"
#include "dux_promise.h"
#include "dux_thrpool.h"

#include "node/dux_console.h"
#include "node/dux_process.h"
#include "node/dux_timer.h"
#include "node/dux_util.h"

#include "hw/dux_hardware_io.h"
#include "hw/dux_paraio.h"
#include "hw/dux_i2ccon.h"
#include "hw/dux_spicon.h"

#include "peridot/dux_peridot.h"
#include "peridot/dux_peridot_gpio.h"
#include "peridot/dux_peridot_i2c.h"
#include "peridot/dux_peridot_spi.h"
#include "peridot/dux_peridot_servo.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* DUX_INTERNAL_H_INCLUDED */