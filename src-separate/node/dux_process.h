#ifndef DUX_PROCESS_H_INCLUDED
#define DUX_PROCESS_H_INCLUDED

#if !defined(DUX_OPT_NO_PROCESS)

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

#else   /* DUX_OPT_NO_PROCESS */

#define dux_process_init(ctx)   (DUK_ERR_NONE)
#define dux_process_tick(ctx)   (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_PROCESS */
#endif  /* DUX_PROCESS_H_INCLUDED */
