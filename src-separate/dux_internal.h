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

/*
 * Sub modules
 */

#include "dux_basis.h"
#include "dux_thrpool.h"
#include "node/dux_console.h"
#include "node/dux_process.h"
#include "node/dux_timer.h"
#include "node/dux_util.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* DUX_INTERNAL_H_INCLUDED */
