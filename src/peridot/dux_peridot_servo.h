#ifndef DUX_PERIDOT_SERVO_H_INCLUDED
#define DUX_PERIDOT_SERVO_H_INCLUDED
#if defined(DUX_USE_BOARD_PERIDOT)

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SERVO)

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_servo_init(duk_context *ctx);
#define DUX_INIT_PERIDOT_SERVO  dux_peridot_servo_init,
#define DUX_TICK_PERIDOT_SERVO

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SERVO */

#define DUX_INIT_PERIDOT_SERVO
#define DUX_TICK_PERIDOT_SERVO

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_SERVO */

#endif  /* DUX_USE_BOARD_PERIDOT */
#endif  /* DUX_PERIDOT_SERVO_H_INCLUDED */
