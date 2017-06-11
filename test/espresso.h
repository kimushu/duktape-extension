#ifndef __ESPRESSO_H__
#define __ESPRESSO_H__

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void espresso_init(duk_context *ctx);
extern int espresso_tick(duk_context *ctx, int *passed, int *skipped, int *failed);
extern int espresso_exit_handler(duk_context *ctx);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* __ESPRESSO_H__ */
