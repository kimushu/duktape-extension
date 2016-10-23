/*
 * [ How to use thread pool ]
 *
 * // (1) Prepare worker C function
 * duk_int_t worker(void *param)
 * {
 *   // This thread runs on worker thread.
 *   // Do not access any object in Duktape heap!
 * }
 *
 * // (2) Create thread pool instance
 * dux_push_thrpool(
 *   ctx,   // Duktape context
 *   1,     // Minimum number of threads
 *   3,     // Maximum number of threads
 * );
 *
 * // (3) Call thread pool to queue new job with 4 arguments
 * duk_push_pointer(ctx, worker);   // Worker function (int (*)(void *))
 * duk_push_pointer(ctx, data_ptr); // Pointer to data for worker
 * duk_push_uint(ctx, data_len);    // Length of data
 * duk_push_c_function(ctx, cb);    // Callback (callable object)
 * duk_call(ctx, 4);                // Queue job
 * // data_ptr can be freed after queueing job
 *
 * // (4) Wait for callback
 * for (;;) dux_tick(ctx);
 *
 */

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include "dux_thrpool.h"
#include "dux_common.h"

static const char *const DUX_THRPOOL_TICK = "\xff" "dux_thrpool.tick";

static const char *const DUX_THRPOOL_HEAD = "\xff" "dux_thrpool.head";
static const char *const DUX_THRPOOL_TAIL = "\xff" "dux_thrpool.tail";

static const char *const DUX_THRPOOL_DATA = "\xff" "data";
static const char *const DUX_THRPOOL_THREAD = "\xff" "thread";

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
	int (*worker)(void *);
	int result;

	char user_data[0];
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

#define MAX_THREADS	(sizeof(((dux_thrpool_t *)0)->thread_mask) * 8)

static void *thrpool_thread(dux_thrpool_thread_t *thread)
{
	dux_thrpool_t *pool;
	pool = (dux_thrpool_t *)((uintptr_t)thread - (sizeof(*pool) + sizeof(*thread) * thread->index));

	for (;;)
	{
		dux_thrpool_job_t *job;

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
			pool->pend_head = job->next_job;
			if (pool->pend_head)
			{
				pool->pend_head->prev_job = NULL;
			}
			else
			{
				pool->pend_tail = NULL;
			}
		}
		pthread_mutex_unlock(&pool->lock);

		if (!job)
		{
			continue;
		}

		thread->state = THRPOOL_STATE_WORKING;
		job->result = (*job->worker)(&job->user_data);

		pthread_mutex_lock(&pool->lock);
		job->prev_job = pool->done_tail;
		job->next_job = NULL;
		if (pool->done_tail)
		{
			pool->done_tail->next_job = job;
			pool->done_tail = job;
		}
		else
		{
			pool->done_head = pool->done_tail = job;
		}
		pthread_mutex_unlock(&pool->lock);
	}
}

static void thrpool_increase(dux_thrpool_t *pool)
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
			// TODO: cannot create thread
			thread->state = THRPOOL_STATE_DEAD;
			return;
		}

		++pool->live_threads;
		return;
	}
}

/*
 * C function entry for thread pool queueing
 */
static duk_ret_t thrpool_queue(duk_context *ctx)
{
	/* [ pointer pointer uint func ] */

	dux_thrpool_t *pool;
	dux_thrpool_job_t *job;
	int pended;
	int (*worker)(void *) = duk_require_pointer(ctx, 0);
	void *data_ptr = duk_require_pointer(ctx, 1);
	duk_uint_t data_len = duk_require_uint(ctx, 2);
	duk_require_callable(ctx, 3);

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, DUX_THRPOOL_DATA);
	pool = (dux_thrpool_t *)duk_require_pointer(ctx, -1);
	duk_pop_2(ctx);

	/* [ pointer pointer uint func ] */

	job = (dux_thrpool_job_t *)dux_calloc(ctx, sizeof(*job) + data_len);
	if (!job)
	{
		return DUK_RET_ALLOC_ERROR;
	}

	job->pool = pool;
	job->worker = worker;
	job->result = -1;
	if (data_ptr && (data_len > 0))
	{
		memcpy(&job->user_data, data_ptr, data_len);
	}

	duk_push_pointer(pool->cb_ctx, job);
	duk_xcopy_top(pool->cb_ctx, ctx, 1);
	duk_put_prop(pool->cb_ctx, -2);

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
		thrpool_increase(pool);
	}

	return 0;	/* return undefined; */
}

/*
 * C function entry for thread pool finalization
 */
static duk_ret_t thrpool_finalize(duk_context *ctx)
{
	/* [ func ] */
	dux_thrpool_t *pool;
	dux_thrpool_thread_t *thread;
	dux_thrpool_job_t *job;
	duk_uint_t tidx;

	duk_get_prop_string(ctx, -1, DUX_THRPOOL_DATA);
	pool = (dux_thrpool_t *)duk_get_pointer(ctx, -1);
	duk_pop(ctx);
	duk_del_prop_string(ctx, -1, DUX_THRPOOL_DATA);

	if (!pool)
	{
		return 0;
	}

	pool->cb_ctx = NULL;
	duk_del_prop_string(ctx, -1, DUX_THRPOOL_THREAD);

	duk_push_heap_stash(ctx);
	/* [ func stash ] */

	/* Remove from pool chain */
	if (pool->prev_pool)
	{
		pool->prev_pool->next_pool = pool->next_pool;
	}
	else
	{
		duk_push_pointer(ctx, pool->next_pool);
		duk_put_prop_string(ctx, -2, DUX_THRPOOL_HEAD);
	}

	if (pool->next_pool)
	{
		pool->next_pool->prev_pool = pool->prev_pool;
	}
	else
	{
		duk_push_pointer(ctx, pool->prev_pool);
		duk_put_prop_string(ctx, -2, DUX_THRPOOL_TAIL);
	}

	duk_pop(ctx);
	/* [ func ] */

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
	for (job = pool->pend_head; job;)
	{
		dux_thrpool_job_t *next_job = job->next_job;
		duk_free(ctx, job);
		job = next_job;
	}
	for (job = pool->done_head; job;)
	{
		dux_thrpool_job_t *next_job = job->next_job;
		duk_free(ctx, job);
		job = next_job;
	}

	/* Destroy sync objects */
	pthread_mutex_destroy(&pool->lock);
	sem_destroy(&pool->sem);

	/* Destroy pool */
	duk_free(ctx, pool);

	return 0;
}

/*
 * Tick handler for thread pool
 */
static duk_ret_t thrpool_tick(duk_context *ctx)
{
	dux_thrpool_t *pool;

	sched_yield();

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_THRPOOL_HEAD);
	pool = duk_get_pointer(ctx, -1);
	duk_pop_2(ctx);
	/* [ ... ] */

	while (pool)
	{
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
			duk_push_pointer(pool->cb_ctx, job);
			duk_get_prop(pool->cb_ctx, -2);
			/* cb_ctx: [ obj func ] */
			duk_push_pointer(pool->cb_ctx, job);
			duk_del_prop(pool->cb_ctx, -2);
			duk_push_pointer(pool->cb_ctx, &job->user_data);
			/* cb_ctx: [ obj func ptr ] */
			duk_call(pool->cb_ctx, 1);
			duk_pop(pool->cb_ctx);
			/* cb_ctx: [ obj ] */
			duk_free(ctx, job);
		}

		pool = pool->next_pool;
	}

	return 0;
}

/*
 * Push new thread pool object
 */
void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads)
{
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
	pool = (dux_thrpool_t *)dux_calloc(ctx, size);
	if (!pool)
	{
		duk_error(ctx, DUK_ERR_ALLOC_ERROR, "not enough memory");
		// never returns
	}

	pool->min_threads = min_threads;
	pool->max_threads = max_threads;

	sem_init(&pool->sem, 0, 0);
	pthread_mutex_init(&pool->lock, NULL);

	for (tidx = 0; tidx < max_threads; ++tidx)
	{
		pool->threads[tidx].index = tidx;
	}

	duk_push_c_function(ctx, thrpool_queue, 4);

	/* Set finalizer */
	duk_push_c_function(ctx, thrpool_finalize, 1);
	duk_set_finalizer(ctx, -2);

	/* Create Duktape thread */
	duk_push_thread(ctx);
	pool->cb_ctx = duk_get_context(ctx, -1);
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_THREAD);

	/* Associate pool data */
	duk_push_pointer(ctx, pool);
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_DATA);

	/* Add to link list */
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, DUX_THRPOOL_TAIL);
	/* [ ... func stash ptr/undef ] */
	pool->prev_pool = (dux_thrpool_t *)duk_get_pointer(ctx, -1);
	duk_pop(ctx);
	duk_push_pointer(ctx, pool);
	if (pool->prev_pool)
	{
		pool->prev_pool->next_pool = pool;
	}
	else
	{
		duk_dup_top(ctx);
		duk_put_prop_string(ctx, -3, DUX_THRPOOL_HEAD);

		duk_push_c_function(ctx, thrpool_tick, 0);
		dux_register_tick(ctx, DUX_THRPOOL_TICK);
	}
	/* [ ... func stash ptr ] */
	duk_put_prop_string(ctx, -2, DUX_THRPOOL_TAIL);
	duk_pop(ctx);
	/* [ ... func ] */

	for (tidx = 0; tidx < min_threads; ++tidx)
	{
		thrpool_increase(pool);
	}
}

