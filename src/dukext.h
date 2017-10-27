#ifndef DUX_ALL_H_INCLUDED
#define DUX_ALL_H_INCLUDED

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Version number: (major*10000)+(minor*100)+(patch)
 */
#define DUX_VERSION         00100L

/*
 * Version string: major.minor.patch
 */
#define DUX_VERSION_STRING  "0.1.0"

/*
 * Initialization
 */
DUK_EXTERNAL_DECL duk_errcode_t dux_initialize(duk_context *ctx);

/*
 * Tick handling
 */
DUK_EXTERNAL_DECL duk_bool_t dux_tick(duk_context *ctx);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* !DUX_ALL_H_INCLUDED */
