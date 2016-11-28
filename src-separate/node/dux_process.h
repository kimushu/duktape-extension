#ifndef DUX_PROCESS_H_INCLUDED
#define DUX_PROCESS_H_INCLUDED

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

#endif  /* DUX_PROCESS_H_INCLUDED */
