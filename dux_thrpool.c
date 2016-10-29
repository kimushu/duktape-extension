/*
 * Native C functions:
 *    void dux_push_thrpool(duk_context *ctx,
 *                          duk_uint_t min_threads, duk_uint_t max_threads);
 *    void dux_thrpool_queue(duk_context *ctx, duk_idx_t pool_index,
 *                           dux_thrpool_worker_t worker,
 *                           dux_thrpool_completer_t completer);
 *    duk_uint_t dux_thrpool_tick(duk_context *ctx);
 *
 * Internal data structure:
 *    heap_stash[DUX_THRPOOL_LINK] = <PlainBuffer dux_thrpool_link_t>;
 *    <thrpool object> = {
 *      buf: <PlainBuffer dux_thrpool_t>,
 *      thread: <Duktape.Thread>
 *    };
 */

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include "dux_thrpool.h"
#include "dux_common.h"

extern void top_level_error(duk_context *ctx);

static const char *const DUX_THRPOOL_LINK = "dux_thrpool.link";
static const char *const DUX_THRPOOL_JOB = "\xff" "dux_thrpool.job";

static const char *const DUX_THRPOOL_BUF = "buf";
static const char *const DUX_THRPOOL_THREAD = "thread";

enum
{
	THRPOOL_STATE_DEAD = 0,
	THRPOOL_STATE_BOOTING,
	THRPOOL_STATE_IDLE,
	THRPOOL_STATE_WORKING,
	THRPOOL_STATE_FINISHED,
	THRPOOL_STATE_ERROR,
};

enum
{
	THRPOOL_REQ_NONE = 0,
	THRPOOL_REQ_EXIT,
};

typedef struct dux_thrpool_thread_t
{
	duk_uint16_t index;
	duk_uint8_t state;
	duk_uint8_t request;
	pthread_t tid;
}
dux_thrpool_thread_t;

typedef struct dux_thrpool_job_t
{
	struct dux_thrpool_t *pool;
	struct dux_thrpool_job_t *prev_job, *next_job;
	duk_int_t result;
	dux_thrpool_worker_t worker;
	dux_thrpool_completer_t completer;
	duk_size_t num_blocks;
	dux_thrpool_block_t blocks[0];
}
dux_thrpool_job_t;

typedef struct dux_thrpool_t
{
	/* Link list */
	struct dux_thrpool_t *prev_pool, *next_pool;

	/*
	 * Callback context
	 * [ obj(callback) ]
	 */
	duk_context *cb_ctx;

	duk_uint8_t min_threads;	/* Number of minimum threads */
	duk_uint8_t max_threads;	/* Number of maximum threads */
	duk_uint8_t live_threads;	/* Number of live threads */
	duk_uint32_t thread_mask;	/* Bit mask of live threads */

	sem_t sem;					/* Worker trigger */
	pthread_mutex_t lock;		/* Lock object for link lists */
	dux_thrpool_job_t *pend_head, *pend_tail;
	dux_thrpool_job_t *done_head, *done_tail;

	dux_thrpool_thread_t threads[0];
}
dux_thrpool_t;

typedef struct dux_thrpool_link_t
{
	dux_thrpool_t *head, *tail;
}
dux_thrpool_link_t;

#define MAX_THREADS	(sizeof(((dux_thrpool_t *)0)->thread_mask) * 8)

/*
 * Get link list of thread pools
 */
static dux_thrpool_link_t *thrpool_get_link(duk_context *ctx)
{
	/* [ ... ] */
	dux_thrpool_link_t *link;

	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_THRPOOL_LINK);
	/* [ ... stash buf/undefined ] */
	link = (dux_thrpool_link_t *)duk_get_buffer(ctx, -1, NULL);

	if (link)
	{
		/* [ ... stash buf ] */
		duk_pop_2(ctx);
		/* [ ... ] */
		return link;
	}

	/* [ ... stash undefined ] */
	duk_push_fixed_buffer(ctx, sizeof(*link));
	/* [ ... stash undefined buf ] */
	link = (dux_thrpool_link_t *)duk_get_buffer(ctx, -1, NULL);
	link->head = NULL;
	link->tail = NULL;
	duk_put_prop_string(ctx, -3, DUX_THRPOOL_LINK);
	/* [ ... stash undefined ] */
	duk_pop_2(ctx);
	/* [ ... ] */
	return link;
}

/*
 * Worker thread entry
 */
static void *thrpool_thread(dux_thrpool_thread_t *thread)
{
	dux_thrpool_t *pool;
	pool = (dux_thrpool_t *)((uintptr_t)thread - (sizeof(*pool) + sizeof(*thread) * thread->index));

	for (;;)
	{
		dux_thrpool_job_t *job, *next_job, *prev_job;

		thread->state = THRPOOL_STATE_IDLE;
		if (sem_wait(&pool->sem) != 0)
		{
			thread->state = THRPOOL_STATE_ERROR;
			return NULL;
		}
		if (thread->request == THRPOOL_REQ_EXIT)
		{
			thread->state = THRPOOL_STATE_FINISHED;
			return NULL;
		}

		pthread_mutex_lock(&pool->lock);
		job = pool->pend_head;
		if (job)
		{
			next_job = pool->pend_head = job->next_job;
			if (next_job)
			{
				next_job->prev_job = NULL;
			}
			else
			{
				pool->pend_tail = NULL;
			}

			job->prev_job = NULL;
			job->next_job = NULL;
		}
		pthread_mutex_unlock(&pool->lock);

		if (!job)
		{
			continue;
		}

		thread->state = THRPOOL_STATE_WORKING;
		job->result = (*job->worker)(&job->blocks[0], job->num_blocks);

		pthread_mutex_lock(&pool->lock);
		prev_job = job->prev_job = pool->done_tail;
		pool->done_tail = job;
		if (prev_job)
		{
			prev_job->next_job = job;
		}
		else
		{
			pool->done_head = job;
		}
		pthread_mutex_unlock(&pool->lock);
	}
}

/*
 * Increase worker thread
 */
static void thrpool_increase(duk_context *ctx, dux_thrpool_t *pool)
{
	dux_thrpool_thread_t *thread;
	duk_uint_t tidx;

	/* Cleanup finished threads */
	for (tidx = 0, thread = pool->threads;
		 tidx < pool->max_threads;
		 ++tidx, ++thread)
	{
		if (thread->state >= THRPOOL_STATE_FINISHED)
		{
			pthread_join(thread->tid, NULL);
			thread->state = THRPOOL_STATE_DEAD;
			--pool->live_threads;
		}
	}

	/* Increase active thread */
	for (tidx = 0, thread = pool->threads;
		 tidx < pool->max_threads;
		 ++tidx, ++thread)
	{
		if (thread->state != THRPOOL_STATE_DEAD)
		{
			continue;
		}

		thread->state = THRPOOL_STATE_BOOTING;
		thread->request = THRPOOL_REQ_NONE;
		if (pthread_create(&thread->tid, NULL, (void *(*)(void *))thrpool_thread, thread) != 0)
		{
			thread->state = THRPOOL_STATE_DEAD;
			duk_push_error_object(ctx, DUK_ERR_INTERNAL_ERROR, "failed to create a new thread");
			top_level_error(ctx);
			duk_pop(ctx);
			return;
		}

		++pool->live_threads;
		return;
	}
}

/*
 * C function entry for thread pool finalization
 */
static duk_ret_t thrpool_finalize(duk_context *ctx)
{
	/* [ obj(thrpool) ] */
	dux_thrpool_link_t *link;
	dux_thrpool_t *pool;
	dux_thrpool_thread_t *thread;
	duk_uint_t tidx;

	pool = (dux_thrpool_t *)duk_get_buffer_data(ctx, -1, NULL);
	if (!pool)
	{
		return 0;
	}

	link = thrpool_get_link(ctx);

	/* Remove from pool chain */
	if (pool->prev_pool)
	{
		pool->prev_pool->next_pool = pool->next_pool;
	}
	else
	{
		link->head = pool->next_pool;
	}

	if (pool->next_pool)
	{
		pool->next_pool->prev_pool = pool->prev_pool;
	}
	else
	{
		link->tail = pool->prev_pool;
	}

	/* Stop all worker threads */
	for (tidx = 0, thread = pool->threads;
		 tidx < pool->max_threads;
		 ++tidx, ++thread)
	{
		thread->request = THRPOOL_REQ_EXIT;
	}

	/* Signal all threads */
	for (tidx = 0; tidx < pool->live_threads; ++tidx)
	{
		sem_post(&pool->sem);
	}

	/* Wait for all threads */
	for (tidx = 0, thread = pool->threads;
		 tidx < pool->max_threads;
		 ++tidx, ++thread)
	{
		if (thread->state != THRPOOL_STATE_DEAD)
		{
			pthread_join(thread->tid, NULL);
			thread->state = THRPOOL_STATE_DEAD;
		}
	}

	/* Destroy pending jobs (callback never been invoked) */
	duk_remove(pool->cb_ctx, -1);
	pool->cb_ctx = NULL;
	duk_del_prop_string(ctx, -1, DUX_THRPOOL_THREAD);

	/* Destroy sync objects */
	pthread_mutex_destroy(&pool->lock);
	sem_destroy(&pool->sem);

	/* Destroy pool */
	duk_free(ctx, pool);

	return 0;
}

/*
 * Initialize thread pool
 */
void dux_thrpool_init(duk_context *ctx)
{
	/* do nothing */
}

/*
 * Push new thread pool object
 */
void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads)
{
	/* [ ... ] */
	dux_thrpool_link_t *link;
	dux_thrpool_t *pool;
	duk_size_t size;
	duk_uint_t tidx;

	if (min_threads > max_threads)
	{
		duk_error(ctx, DUK_ERR_RANGE_ERROR,
				"max_threads must be equal or larger than min_threads");
		// never returns
	}

	if (max_threads > MAX_THREADS)
	{
		duk_error(ctx, DUK_ERR_RANGE_ERROR,
				"max_threads cannot exceed %u", MAX_THREADS);
		// never returns
	}

	size = sizeof(dux_thrpool_t) + sizeof(*pool->threads) * max_threads;
	duk_push_object(ctx);
	/* [ ... obj ] */
	duk_push_fixed_buffer(ctx, size);
	/* [ ... obj buf ] */
	pool = (dux_thrpool_t *)duk_get_buffer(ctx, -1, NULL);
	memset(pool, 0, size);
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_BUF);
	/* [ ... obj(thrpool) ] */

	pool->min_threads = min_threads;
	pool->max_threads = max_threads;

	sem_init(&pool->sem, 0, 0);
	pthread_mutex_init(&pool->lock, NULL);

	for (tidx = 0; tidx < max_threads; ++tidx)
	{
		pool->threads[tidx].index = tidx;
	}

	/* Set finalizer */
	duk_push_c_function(ctx, thrpool_finalize, 1);
	duk_set_finalizer(ctx, -2);
	/* [ ... obj(thrpool) ] */

	/* Create Duktape thread */
	duk_push_thread(ctx);
	pool->cb_ctx = duk_get_context(ctx, -1);
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_THREAD);
	/* [ ... obj(thrpool) ] */

	duk_push_object(pool->cb_ctx);
	/* cb_ctx: [ obj ] */

	/* Add to link list */
	link = thrpool_get_link(ctx);
	pool->prev_pool = link->tail;
	link->tail = pool;

	for (tidx = 0; tidx < min_threads; ++tidx)
	{
		thrpool_increase(ctx, pool);
	}
	/* [ ... obj(thrpool) ] */
}

/* Queue job to thread pool */
void dux_thrpool_queue(duk_context *ctx,
                       duk_idx_t pool_index,
                       dux_thrpool_worker_t worker,
                       dux_thrpool_completer_t completer)
{
	/* [ ... obj(thrpool) ... job ] */
	/*       ^pool_index      ^top  */

	duk_size_t num_blocks;
	dux_thrpool_t *pool;
	dux_thrpool_job_t *job;
	duk_uarridx_t index;
	dux_thrpool_block_t *block;
	int pended;

	/* Get pool data */
	duk_get_prop_string(ctx, pool_index, DUX_THRPOOL_BUF);
	pool = (dux_thrpool_t *)duk_require_buffer(ctx, -1, NULL);
	duk_pop(ctx);

	/* Check job */
	if (!duk_is_array(ctx, -1))
	{
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "job object must be an array");
		// never returns
	}
	num_blocks = duk_get_length(ctx, -1);

	/* Add job data */
	duk_push_fixed_buffer(ctx, sizeof(*job) + sizeof(*block) * num_blocks);
	/* [ ... obj(thrpool) ... job buf ] */
	job = (dux_thrpool_job_t *)duk_get_buffer(ctx, -1, NULL);
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_JOB);
	/* [ ... obj(thrpool) ... job ] */

	job->pool = pool;
	job->result = -1;
	job->worker = worker;
	job->completer = completer;
	job->num_blocks = num_blocks;

	block = &job->blocks[0];
	for (index = 0; index < num_blocks; ++index, ++block)
	{
		void *pointer;
		duk_size_t length;

		duk_get_prop_index(ctx, -1, index);
		/* [ ... obj(thrpool) ... job any ] */
		if ((pointer = (void *)duk_get_lstring(ctx, -1, &length)) ||
			(pointer = duk_get_buffer_data(ctx, -1, &length)))
		{
			// string / plain buffer / Buffer object
			block->pointer = pointer;
			block->length = length;
		}
		else
		{
			block->pointer = NULL;
			block->length = 0;
		}
		duk_pop(ctx);
		/* [ ... obj(thrpool) ... job ] */
	}

	duk_push_pointer(pool->cb_ctx, job);
	duk_xmove_top(pool->cb_ctx, ctx, 1);
	/* [ ... obj(thrpool) ... ] */
	duk_put_prop(pool->cb_ctx, -3);

	pthread_mutex_lock(&pool->lock);
	if (pool->pend_tail)
	{
		pool->pend_tail->next_job = job;
		pool->pend_tail = job;
	}
	else
	{
		pool->pend_head = pool->pend_tail = job;
	}
	pthread_mutex_unlock(&pool->lock);

	sem_post(&pool->sem);
	sem_getvalue(&pool->sem, &pended);

	if ((pended > 0) && (pool->live_threads < pool->max_threads))
	{
		thrpool_increase(ctx, pool);
	}

	/* [ ... obj(thrpool) ... ] */
}

/*
 * Tick handler for thread pool
 */
duk_uint_t dux_thrpool_tick(duk_context *ctx)
{
	/* [ ... ] */
	dux_thrpool_t *pool;
	duk_uint_t processed = 0;

	sched_yield();

	pool = thrpool_get_link(ctx)->head;
	while (pool)
	{
		duk_context *cb_ctx = pool->cb_ctx;
		++processed;

		for (;;)
		{
			dux_thrpool_job_t *job;

			pthread_mutex_lock(&pool->lock);
			job = pool->done_head;
			if (job)
			{
				pool->done_head = job->next_job;
				if (pool->done_head)
				{
					pool->done_head->prev_job = NULL;
				}
				else
				{
					pool->done_tail = NULL;
				}
			}
			pthread_mutex_unlock(&pool->lock);

			if (!job)
			{
				break;
			}

			/* cb_ctx: [ obj ] */
			duk_push_c_function(cb_ctx, job->completer, 2);
			/* cb_ctx: [ obj func ] */
			duk_push_pointer(cb_ctx, job);
			duk_get_prop(cb_ctx, -2);
			/* cb_ctx: [ obj func job ] */
			duk_push_pointer(cb_ctx, job);
			duk_del_prop(cb_ctx, -2);
			/* cb_ctx: [ obj func job ] */
			duk_push_int(cb_ctx, job->result);
			/* cb_ctx: [ obj func job int ] */
			if (duk_pcall(cb_ctx, 2) != 0)
			{
				/* cb_ctx: [ obj err ] */
				top_level_error(cb_ctx);
			}
			/* cb_ctx: [ obj retval/err ] */
			duk_pop(cb_ctx);
			/* cb_ctx: [ obj ] */
		}

		pool = pool->next_pool;
	}

	/* [ ... ] */
	return processed;
}

