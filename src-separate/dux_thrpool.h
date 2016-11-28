#ifndef DUX_THRPOOL_H_INCLUDED
#define DUX_THRPOOL_H_INCLUDED

#if !defined(DUX_OPT_NO_THRPOOL)

/*
 * Type definitions
 */

struct dux_thrpool_block;
typedef duk_int_t (*dux_thrpool_worker_t)(const struct dux_thrpool_block *blocks, duk_size_t num_blocks);
typedef duk_c_function dux_thrpool_completer_t;

/*
 * Structures
 */

typedef struct dux_thrpool_block
{
	union
	{
		void *pointer;
		duk_uint_t uint;
	};
	duk_size_t length;
}
dux_thrpool_block;

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_thrpool_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_thrpool_tick(duk_context *ctx);

DUK_INTERNAL_DECL void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads);
DUK_INTERNAL_DECL void dux_thrpool_queue(duk_context *ctx, duk_idx_t pool_index,
		dux_thrpool_worker_t worker, dux_thrpool_completer_t completer);

#else   /* DUX_OPT_NO_THRPOOL */

#define dux_thrpool_init(ctx)   (DUK_ERR_NONE)
#define dux_thrpool_tick(ctx)   (DUX_TICK_RET_JOBLESS)

#endif  /* DUX_OPT_NO_THRPOOL */
#endif  /* DUX_THRPOOL_H_INCLUDED */
