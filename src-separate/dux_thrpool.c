/*
 * Native C functions:
 *    void dux_push_thrpool(duk_context *ctx,
 *                          duk_uint_t min_threads, duk_uint_t max_threads);
 *    void dux_thrpool_queue(duk_context *ctx, duk_idx_t pool_index,
 *                           dux_thrpool_worker_t worker,
 *                           dux_thrpool_completer_t completer);
 *
 * Internal data structure:
 *    heap_stash[DUX_IPK_THRPOOL] = new PlainBuffer(dux_thrpool_link);
 *    <thrpool object> = {
 *      B: <PlainBuffer dux_thrpool_pool>,
 *      T: <Duktape.Thread>
 *    };
 *    <job object> = new Array();
 *    <job object>[DUX_IPK_THRPOOL_JOB] = new PlainBuffer(dux_thrpool_job);
 */

#if !defined(DUX_OPT_NO_THRPOOL)
#include "dux_internal.h"
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_THRPOOL[]      = DUX_IPK("Thrpool");
DUK_LOCAL const char DUX_IPK_THRPOOL_JOB[]  = DUX_IPK("tpJob");
DUK_LOCAL const char THRPOOL_KEY_BUF[]      = "B";
DUK_LOCAL const char THRPOOL_KEY_THREAD[]   = "T";

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

/*
 * Structures
 */

typedef struct dux_thrpool_thread
{
	duk_uint16_t index;
	duk_uint8_t state;
	duk_uint8_t request;
	pthread_t tid;
}
dux_thrpool_thread;

typedef struct dux_thrpool_job
{
	struct dux_thrpool_pool *pool;
	struct dux_thrpool_job *prev_job, *next_job;
	duk_int_t result;
	dux_thrpool_worker_t worker;
	dux_thrpool_completer_t completer;
	duk_size_t num_blocks;
	dux_thrpool_block blocks[0];
}
dux_thrpool_job;

typedef struct dux_thrpool_pool
{
	/* Link list */
	struct dux_thrpool_pool *prev_pool, *next_pool;

	/*
	 * Callback context
	 * [ obj(callback) ]
	 */
	duk_context *cb_ctx;

	duk_uint8_t min_threads;    /* Number of minimum threads */
	duk_uint8_t max_threads;    /* Number of maximum threads */
	duk_uint8_t live_threads;   /* Number of live threads */
	duk_uint32_t thread_mask;   /* Bit mask of live threads */

	sem_t sem;                  /* Worker trigger */
	pthread_mutex_t lock;       /* Lock object for link lists */
	dux_thrpool_job *pend_head, *pend_tail;
	dux_thrpool_job *work_head, *work_tail;
	dux_thrpool_job *done_head, *done_tail;

	dux_thrpool_thread threads[0];
}
dux_thrpool_pool;

typedef struct dux_thrpool_link
{
	dux_thrpool_pool *head, *tail;
}
dux_thrpool_link;

#define MAX_THREADS	(sizeof(((dux_thrpool_pool *)0)->thread_mask) * 8)

/*
 * Get link list of thread pools
 */
DUK_LOCAL dux_thrpool_link *thrpool_get_link(duk_context *ctx)
{
	dux_thrpool_link *link;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_get_prop_string(ctx, -1, DUX_IPK_THRPOOL);
	/* [ ... stash buf/undefined ] */
	link = (dux_thrpool_link *)duk_get_buffer(ctx, -1, NULL);

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
	link = (dux_thrpool_link *)duk_get_buffer(ctx, -1, NULL);
	link->head = NULL;
	link->tail = NULL;
	duk_put_prop_string(ctx, -3, DUX_IPK_THRPOOL);
	/* [ ... stash undefined ] */
	duk_pop_2(ctx);
	/* [ ... ] */
	return link;
}

/*
 * Worker thread entry
 */
DUK_LOCAL void *thrpool_thread(dux_thrpool_thread *thread)
{
	dux_thrpool_pool *pool;
	pool = (dux_thrpool_pool *)((uintptr_t)thread - (sizeof(*pool) + sizeof(*thread) * thread->index));

	for (;;)
	{
		dux_thrpool_job *job, *next_job, *prev_job;

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
			/* Remove job from pending chain */
			next_job = pool->pend_head = job->next_job;
			if (next_job)
			{
				next_job->prev_job = NULL;
			}
			else
			{
				pool->pend_tail = NULL;
			}

			/* Append job to working chain */
			prev_job = job->prev_job = pool->work_tail;
			pool->work_tail = job;
			if (prev_job)
			{
				prev_job->next_job = job;
			}
			else
			{
				pool->work_head = job;
			}
			job->next_job = NULL;
		}
		pthread_mutex_unlock(&pool->lock);

		if (!job)
		{
			continue;
		}

		/* Do work */
		thread->state = THRPOOL_STATE_WORKING;
		job->result = (*job->worker)(&job->blocks[0], job->num_blocks);

		pthread_mutex_lock(&pool->lock);
		/* Remove job from working chain */
		prev_job = job->prev_job;
		if (prev_job)
		{
			prev_job->next_job = job->next_job;
		}
		else
		{
			pool->work_head = job->next_job;
		}
		next_job = job->next_job;
		if (next_job)
		{
			next_job->prev_job = job->prev_job;
		}
		else
		{
			pool->work_tail = job->prev_job;
		}

		/* Append job to done chain */
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
DUK_LOCAL void thrpool_increase(duk_context *ctx, dux_thrpool_pool *pool)
{
	dux_thrpool_thread *thread;
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
			dux_report_error(ctx);
			duk_pop(ctx);
			return;
		}

		++pool->live_threads;
		return;
	}
}

/*
 * Entry of thrpool's finalizer
 */
DUK_LOCAL duk_ret_t thrpool_finalize(duk_context *ctx)
{
	/* [ obj(thrpool) ] */
	dux_thrpool_link *link;
	dux_thrpool_pool *pool;
	dux_thrpool_thread *thread;
	duk_uint_t tidx;

	pool = (dux_thrpool_pool *)duk_get_buffer_data(ctx, -1, NULL);
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
	duk_del_prop_string(ctx, -1, THRPOOL_KEY_THREAD);

	/* Destroy sync objects */
	pthread_mutex_destroy(&pool->lock);
	sem_destroy(&pool->sem);

	return 0;
}

/*
 * Initialize thread pool
 */
DUK_INTERNAL duk_errcode_t dux_thrpool_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_del_prop_string(ctx, -1, DUX_IPK_THRPOOL);
	duk_pop(ctx);
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Tick handler for thread pool
 */
DUK_INTERNAL duk_int_t dux_thrpool_tick(duk_context *ctx)
{
	/* [ ... ] */
	dux_thrpool_pool *pool;
	duk_int_t result = DUX_TICK_RET_JOBLESS;

	sched_yield();

	pool = thrpool_get_link(ctx)->head;
	while (pool)
	{
		duk_context *cb_ctx = pool->cb_ctx;

		for (;;)
		{
			dux_thrpool_job *job;

			pthread_mutex_lock(&pool->lock);
			if (pool->pend_head || pool->work_head)
			{
				result = DUX_TICK_RET_CONTINUE;
			}
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
			result = DUX_TICK_RET_CONTINUE;

			/* cb_ctx: [ obj ] */
			duk_push_c_function(cb_ctx, job->completer, 2);
			/* cb_ctx: [ obj func ] */
			duk_push_pointer(cb_ctx, job);
			/* cb_ctx: [ obj func ptr ] */
			duk_get_prop(cb_ctx, 0);
			/* cb_ctx: [ obj func job ] */
			duk_push_pointer(cb_ctx, job);
			/* cb_ctx: [ obj func job ptr ] */
			duk_del_prop(cb_ctx, 0);
			/* cb_ctx: [ obj func job ] */
			duk_push_int(cb_ctx, job->result);
			/* cb_ctx: [ obj func job int ] */
			if (duk_pcall(cb_ctx, 2) != 0)
			{
				/* cb_ctx: [ obj err ] */
				dux_report_error(cb_ctx);
			}
			/* cb_ctx: [ obj retval/err ] */
			duk_pop(cb_ctx);
			/* cb_ctx: [ obj ] */
		}

		pool = pool->next_pool;
	}

	/* [ ... ] */
	return result;
}

/*
 * Push new thread pool object
 */
DUK_INTERNAL void dux_push_thrpool(duk_context *ctx, duk_uint_t min_threads, duk_uint_t max_threads)
{
	dux_thrpool_link *link;
	dux_thrpool_pool *pool;
	duk_size_t size;
	duk_uint_t tidx;

	/* [ ... ] */
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

	size = sizeof(dux_thrpool_pool) + sizeof(*pool->threads) * max_threads;
	duk_push_object(ctx);
	/* [ ... obj ] */
	duk_push_fixed_buffer(ctx, size);
	/* [ ... obj buf ] */
	pool = (dux_thrpool_pool *)duk_get_buffer(ctx, -1, NULL);
	memset(pool, 0, size);
	duk_put_prop_string(ctx, -2, THRPOOL_KEY_BUF);
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
	duk_put_prop_string(ctx, -2, THRPOOL_KEY_THREAD);
	/* [ ... obj(thrpool) ] */

	duk_push_object(pool->cb_ctx);
	/* cb_ctx: [ obj ] */

	/* Add to link list */
	link = thrpool_get_link(ctx);
	if ((pool->prev_pool = link->tail) != NULL)
	{
		link->tail->next_pool = pool;
	}
	else
	{
		link->head = pool;
	}
	link->tail = pool;

	for (tidx = 0; tidx < min_threads; ++tidx)
	{
		thrpool_increase(ctx, pool);
	}
	/* [ ... obj(thrpool) ] */
}

/* Queue job to thread pool */
DUK_INTERNAL void dux_thrpool_queue(duk_context *ctx,
                                    duk_idx_t pool_index,
                                    dux_thrpool_worker_t worker,
                                    dux_thrpool_completer_t completer)
{
	/* [ ... obj(thrpool) ... job ] */
	/*       ^pool_index      ^top  */

	duk_size_t num_blocks;
	dux_thrpool_pool *pool;
	dux_thrpool_job *job;
	duk_uarridx_t index;
	dux_thrpool_block *block;
	int pended;

	/* Get pool data */
	duk_get_prop_string(ctx, pool_index, THRPOOL_KEY_BUF);
	pool = (dux_thrpool_pool *)duk_require_buffer(ctx, -1, NULL);
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
	job = (dux_thrpool_job *)duk_get_buffer(ctx, -1, NULL);
	duk_put_prop_string(ctx, -2, DUX_IPK_THRPOOL_JOB);
	/* [ ... obj(thrpool) ... job ] */

	job->pool = pool;
	job->result = -1;
	job->worker = worker;
	job->completer = completer;
	job->num_blocks = num_blocks;

	block = &job->blocks[0];
	for (index = 0; index < num_blocks; ++index, ++block)
	{
		dux_thrpool_block blk;

		duk_get_prop_index(ctx, -1, index);
		/* [ ... obj(thrpool) ... job any ] */
		if ((blk.pointer = (void *)duk_get_lstring(ctx, -1, &blk.length)) ||
			(blk.pointer = duk_get_buffer_data(ctx, -1, &blk.length)))
		{
			// string / plain buffer / Buffer object
			block->pointer = blk.pointer;
			block->length = blk.length;
		}
		else if (duk_is_number(ctx, -1))
		{
			// uint
			block->uint = duk_get_uint(ctx, -1);
			block->length = 0;
		}
		else if (duk_is_pointer(ctx, -1))
		{
			// pointer
			block->uint = duk_get_pointer(ctx, -1);
			block->length = 0;
		}
		else
		{
			// others
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

#endif  /* !DUX_OPT_NO_THRPOOL */
