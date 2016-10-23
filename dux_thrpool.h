#ifndef DUX_THRPOOL_H_
#define DUX_THRPOOL_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_THRPOOL_H_ */
