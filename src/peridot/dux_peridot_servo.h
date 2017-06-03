#ifndef DUX_PERIDOT_SERVO_H_INCLUDED
#define DUX_PERIDOT_SERVO_H_INCLUDED
#if defined(DUX_USE_BOARD_PERIDOT)

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SERVO)

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_servo_init(duk_context *ctx);
#define dux_peridot_servo_tick(ctx) (DUX_TICK_RET_JOBLESS)

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SERVO */

#define dux_peridot_servo_init(ctx) (DUK_ERR_NONE)
#define dux_peridot_servo_tick(ctx) (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_SERVO */

#endif  /* DUX_USE_BOARD_PERIDOT */
#endif  /* DUX_PERIDOT_SERVO_H_INCLUDED */
