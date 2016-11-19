#ifndef DUX_PROCESS_H_
#define DUX_PROCESS_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void dux_process_init(duk_context *ctx);
extern duk_bool_t dux_process_is_exit(duk_context *ctx, duk_int_t *code);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_PROCESS_H_ */
