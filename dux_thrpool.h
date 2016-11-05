#ifndef DUX_THRPOOL_H_
#define DUX_THRPOOL_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dux_thrpool_block_t
{
	union
	{
		void *pointer;
		duk_uint_t uint;
	};
	duk_size_t length;
}
dux_thrpool_block_t;

typedef duk_int_t (*dux_thrpool_worker_t)(const dux_thrpool_block_t *blocks, duk_size_t num_blocks);
typedef duk_c_function dux_thrpool_completer_t;

extern void dux_thrpool_init(duk_context *ctx);
extern void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads);
extern void dux_thrpool_queue(duk_context *ctx, duk_idx_t pool_index,
		dux_thrpool_worker_t worker, dux_thrpool_completer_t completer);
extern duk_uint_t dux_thrpool_tick(duk_context *ctx);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_THRPOOL_H_ */
