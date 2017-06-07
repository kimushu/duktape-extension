#ifndef DUX_PROCESS_H_INCLUDED
#define DUX_PROCESS_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_PROCESS)

/*
 * Structures
 */

typedef struct dux_process_data
{
	duk_context *tick_context;
	duk_bool_t force_exit;
	duk_bool_t exit_valid;
	duk_int_t exit_code;
}
dux_process_data;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_process_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_process_tick(duk_context *ctx);
#define DUX_INIT_PROCESS    dux_process_init,
#define DUX_TICK_PROCESS    dux_process_tick,

#else   /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_PROCESS */

#define DUX_INIT_PROCESS
#define DUX_TICK_PROCESS

#endif  /* DUX_OPT_NO_NODEJS_MODULES || DUX_OPT_NO_PROCESS */
#endif  /* !DUX_PROCESS_H_INCLUDED */
