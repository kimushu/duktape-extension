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
 * Evaluate file/source as a module
 */
DUK_EXTERNAL_DECL duk_ret_t dux_eval_module_raw(duk_context *ctx, const void *data, duk_size_t len, duk_int_t flags);
#define dux_eval_module_file(ctx, path) \
    ((void)dux_eval_module_raw((ctx), (path), 0, DUK_COMPILE_NOSOURCE))
#define dux_eval_module_file_noresult(ctx, path) \
    ((void)dux_eval_module_raw((ctx), (path), 0, DUK_COMPILE_NOSOURCE | DUK_COMPILE_NORESULT))
#define dux_eval_module_string(ctx, data) \
    ((void)dux_eval_module_raw((ctx), (data), 0, DUK_COMPILE_STRLEN | DUK_COMPILE_NOFILENAME))
#define dux_eval_module_string_noresult(ctx, data) \
    ((void)dux_eval_module_raw((ctx), (data), 0, DUK_COMPILE_STRLEN | DUK_COMPILE_NORESULT | DUK_COMPILE_NOFILENAME))
#define dux_eval_module_lstring(ctx, data, len) \
    ((void)dux_eval_module_raw((ctx), (data), (len), DUK_COMPILE_NOFILENAME))
#define dux_eval_module_lstring_noresult(ctx, data, len) \
    ((void)dux_eval_module_raw((ctx), (data), (len), DUK_COMPILE_NORESULT | DUK_COMPILE_NOFILENAME))
#define dux_peval_module_file(ctx, path) \
    (dux_eval_module_raw((ctx), (path), 0, DUK_COMPILE_NOSOURCE | DUK_COMPILE_SAFE))
#define dux_peval_module_file_noresult(ctx, path) \
    (dux_eval_module_raw((ctx), (path), 0, DUK_COMPILE_NOSOURCE | DUK_COMPILE_NORESULT | DUK_COMPILE_SAFE))
#define dux_peval_module_string(ctx, data) \
    (dux_eval_module_raw((ctx), (data), 0, DUK_COMPILE_STRLEN | DUK_COMPILE_NOFILENAME | DUK_COMPILE_SAFE))
#define dux_peval_module_string_noresult(ctx, data) \
    (dux_eval_module_raw((ctx), (data), 0, DUK_COMPILE_STRLEN | DUK_COMPILE_NORESULT | DUK_COMPILE_NOFILENAME | DUK_COMPILE_SAFE))
#define dux_peval_module_lstring(ctx, data, len) \
    (dux_eval_module_raw((ctx), (data), (len), DUK_COMPILE_NOFILENAME | DUK_COMPILE_SAFE))
#define dux_peval_module_lstring_noresult(ctx, data, len) \
    (dux_eval_module_raw((ctx), (data), (len), DUK_COMPILE_NORESULT | DUK_COMPILE_NOFILENAME | DUK_COMPILE_SAFE))

/*
 * Tick handling
 */
DUK_EXTERNAL_DECL duk_bool_t dux_tick(duk_context *ctx);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* !DUX_ALL_H_INCLUDED */
