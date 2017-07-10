#ifndef DUX_PERIDOT_H_INCLUDED
#define DUX_PERIDOT_H_INCLUDED

#if defined(DUX_USE_BOARD_PERIDOT)

#include "dux_peridot_gpio.h"
#include "dux_peridot_i2c.h"
#include "dux_peridot_spi.h"
#include "dux_peridot_servo.h"

/*
 * Constants
 */

#define DUX_PERIDOT_PIN_MIN 0
#define DUX_PERIDOT_PIN_MAX 27

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_init(duk_context *ctx);
#define DUX_INIT_PERIDOT    dux_peridot_init,
#define DUX_TICK_PERIDOT

DUK_INTERNAL_DECL duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index);
DUK_INTERNAL_DECL duk_int_t dux_get_peridot_pin_by_key(duk_context *ctx, duk_idx_t index, ...);

#else   /* DUX_USE_BOARD_PERIDOT */

#define DUX_INIT_PERIDOT
#define DUX_TICK_PERIDOT

#endif  /* !DUX_USE_BOARD_PERIDOT */
#endif  /* !DUX_PERIDOT_H_INCLUDED */
