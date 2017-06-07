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
#include "dux_modules.h"
#include "dux_promise.h"
#include "dux_thrpool.h"
#include "node/dux_node.h"
#include "hw/dux_hardware.h"
#include "peridot/dux_peridot.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* DUX_INTERNAL_H_INCLUDED */
