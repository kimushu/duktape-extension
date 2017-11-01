#ifndef DUX_WORK_H_INCLUDED
#define DUX_WORK_H_INCLUDED

#if !defined(DUX_OPT_NO_WORK)

/*
 * Type definitions
 */

typedef struct
{
}
dux_work_t;

typedef void (*dux_work_finalizer)(duk_context *ctx, dux_work_t *req);
typedef duk_int_t (*dux_work_cb)(dux_work_t *req);
typedef duk_int_t (*dux_after_work_cb)(duk_context *ctx, dux_work_t *req);

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_bool_t dux_work_aborting(dux_work_t *req);
DUK_INTERNAL_DECL void dux_queue_work(duk_context *ctx, const dux_work_t *req, duk_size_t req_size, dux_work_cb work_cb, dux_after_work_cb after_work_cb, duk_idx_t after_nargs, dux_work_finalizer finalizer);

DUK_INTERNAL_DECL duk_errcode_t dux_work_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_work_tick(duk_context *ctx);
#define DUX_INIT_WORK       dux_work_init,
#define DUX_TICK_WORK       dux_work_tick,

#else   /* !DUX_OPT_NO_WORK */

#define DUX_INIT_WORK
#define DUX_TICK_WORK

#endif  /* DUX_OPT_NO_WORK */
#endif  /* !DUX_WORK_H_INCLUDED */
