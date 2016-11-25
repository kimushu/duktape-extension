#ifndef DUX_PERIDOT_H_
#define DUX_PERIDOT_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DUX_PERIDOT_PIN_MIN 0
#define DUX_PERIDOT_PIN_MAX 27

extern void dux_peridot_init(duk_context *ctx);
extern duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index, const char *key);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_PERIDOT_H_ */
