#ifndef DUX_PATH_H_INCLUDED
#define DUX_PATH_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_PATH)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_path_init(duk_context *ctx);
DUK_INTERNAL_DECL const char *dux_path_basename(duk_context *ctx, const char *path, const char *ext);
DUK_INTERNAL_DECL const char *dux_path_dirname(duk_context *ctx, const char *path);
DUK_INTERNAL_DECL const char *dux_path_normalize(duk_context *ctx, const char *path);
DUK_INTERNAL_DECL const char *dux_path_relative(duk_context *ctx, const char *from, const char *to);

#define DUX_INIT_PATH   dux_path_init,
#define DUX_TICK_PATH

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_PATH */

#define DUX_INIT_PATH
#define DUX_TICK_PATH

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_PATH */
#endif  /* !DUX_PATH_H_INCLUDED */
