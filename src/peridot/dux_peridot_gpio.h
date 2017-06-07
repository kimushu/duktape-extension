#ifndef DUX_PERIDOT_GPIO_H_INCLUDED
#define DUX_PERIDOT_GPIO_H_INCLUDED
#if defined(DUX_USE_BOARD_PERIDOT)

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_GPIO)

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_gpio_init(duk_context *ctx);
#define DUX_INIT_PERIDOT_GPIO   dux_peridot_gpio_init,
#define DUX_TICK_PERIDOT_GPIO

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_GPIO */

#define DUX_INIT_PERIDOT_GPIO
#define DUX_TICK_PERIDOT_GPIO

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_GPIO */

#endif  /* DUX_USE_BOARD_PERIDOT */
#endif  /* DUX_PERIDOT_GPIO_H_INCLUDED */
