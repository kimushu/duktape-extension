#ifndef DUX_MODULES_H_INCLUDED
#define DUX_MODULES_H_INCLUDED

typedef duk_ret_t (*dux_module_entry)(duk_context *ctx);

DUK_INTERNAL_DECL duk_errcode_t dux_modules_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_errcode_t dux_modules_register(duk_context *ctx, const char *name, dux_module_entry entry);

#define DUX_INIT_MODULES    dux_modules_init,
#define DUX_TICK_MODULES

#endif  /* !DUX_MODULES_H_INCLUDED */
