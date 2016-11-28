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
#ifndef DUX_OPT_NO_PROCESS
#  include "node/dux_process.h"
#endif
#ifndef DUX_OPT_NO_UTIL
#  include "node/dux_util.h"
#endif
#ifndef DUX_OPT_NO_TIMER
#  include "dux_timer.h"
#endif

/*
 * Table definition (after including all sub-modules)
 */
typedef struct dux_context_table
{
#ifndef DUX_OPT_NO_PROCESS
	dux_process_context process;
#endif
#ifndef DUX_OPT_NO_UTIL
	/* dux_util_context util; */
#endif
#ifndef DUX_OPT_NO_TIMER
	dux_timer_context timer;
#endif
}
dux_context_table;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* DUX_INTERNAL_H_INCLUDED */
