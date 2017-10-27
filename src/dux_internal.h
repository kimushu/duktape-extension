/*
 * Top level header file for internal source files
 */

#ifndef DUX_INTERNAL_H_INCLUDED
#define DUX_INTERNAL_H_INCLUDED

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__nios2__) && defined(__hal__)
#include <system.h>
#endif  /* __nios2__ && __hal__ */

/*
 * Sub modules
 */

#include "dux_config.h"
#include "dukext.h"
#include "dux_basis.h"
#include "dux_modules.h"
#include "dux_promise.h"
#include "dux_thrpool.h"
#include "node/dux_node.h"
#include "hw/dux_hardware.h"
#include "peridot/dux_peridot.h"
#include "node/dux_immediate.h"
#include "packages/delay.h"
#include "packages/sprintf.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* DUX_INTERNAL_H_INCLUDED */
