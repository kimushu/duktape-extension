#ifndef DUX_PARALLELIO_H_INCLUDED
#define DUX_PARALLELIO_H_INCLUDED

#if !defined(DUX_OPT_NO_PARALLELIO)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_parallelio_init(duk_context *ctx);
#define dux_parallelio_tick(ctx)    (DUX_TICK_RET_JOBLESS)

#else   /* !DUX_OPT_NO_PARALLELIO */

#define dux_parallelio_init(ctx)    (DUK_ERR_NONE)
#define dux_parallelio_tick(ctx)    (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_PARALLELIO */
#endif  /* !DUX_PARALLELIO_H_INCLUDED */
