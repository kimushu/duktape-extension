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
 * Typedefs
 */
typedef duk_ret_t (*dux_file_reader)(duk_context *ctx, const char *path);

/*
 * Structures
 */
typedef struct dux_file_accessor_s {
    dux_file_reader reader;
} dux_file_accessor;

/*
 * Initialization
 */
DUK_EXTERNAL_DECL duk_errcode_t dux_initialize(duk_context *ctx, const dux_file_accessor *file_accessor);

/*
 * Tick handling
 */
DUK_EXTERNAL_DECL duk_bool_t dux_tick(duk_context *ctx);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* !DUX_ALL_H_INCLUDED */
