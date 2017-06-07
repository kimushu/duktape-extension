#ifndef DUX_PERIDOT_I2C_H_INCLUDED
#define DUX_PERIDOT_I2C_H_INCLUDED
#if defined(DUX_USE_BOARD_PERIDOT)

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_I2C)

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_i2c_init(duk_context *ctx);
#define DUX_INIT_PERIDOT_I2C    dux_peridot_i2c_init,
#define DUX_TICK_PERIDOT_I2C

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_I2C */

#define DUX_INIT_PERIDOT_I2C
#define DUX_TICK_PERIDOT_I2C

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_I2C */

#endif  /* DUX_USE_BOARD_PERIDOT */
#endif  /* DUX_PERIDOT_I2C_H_INCLUDED */
