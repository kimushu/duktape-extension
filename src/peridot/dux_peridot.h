#ifndef DUX_PERIDOT_H_INCLUDED
#define DUX_PERIDOT_H_INCLUDED

#if defined(DUX_USE_BOARD_PERIDOT)

/*
 * Constants
 */

#define DUX_PERIDOT_PIN_MIN 0
#define DUX_PERIDOT_PIN_MAX 27

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_init(duk_context *ctx);
#define dux_peridot_tick(ctx)   (DUX_TICK_RET_JOBLESS)

DUK_INTERNAL_DECL duk_bool_t dux_push_peridot_stash(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index);
DUK_INTERNAL_DECL duk_int_t dux_get_peridot_pin_by_key(duk_context *ctx, duk_idx_t index, ...);

#else   /* DUX_USE_BOARD_PERIDOT */

#define dux_peridot_init(ctx)   (DUK_ERR_NONE)
#define dux_peridot_tick(ctx)   (DUX_TICK_RET_JOBLESS)

#endif  /* !DUX_USE_BOARD_PERIDOT */
#endif  /* !DUX_PERIDOT_H_INCLUDED */
