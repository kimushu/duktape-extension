#ifndef DUX_PERIDOT_SPI_H_INCLUDED
#define DUX_PERIDOT_SPI_H_INCLUDED
#if defined(DUX_USE_BOARD_PERIDOT)

#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SPI)

DUK_INTERNAL_DECL duk_errcode_t dux_peridot_spi_init(duk_context *ctx);
#define DUX_INIT_PERIDOT_SPI    dux_peridot_spi_init,
#define DUX_TICK_PERIDOT_SPI

#else   /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SPI */

#define DUX_INIT_PERIDOT_SPI
#define DUX_TICK_PERIDOT_SPI

#endif  /* DUX_OPT_NO_HARDWARE_MODULES || DUX_OPT_NO_SPI */

#endif  /* DUX_USE_BOARD_PERIDOT */
#endif  /* DUX_PERIDOT_SPI_H_INCLUDED */
