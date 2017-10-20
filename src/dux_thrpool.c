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
 *    <ArrayBuffer dux_thrpool_pool:thrpool> = {
 *      <Pointer 1>: <job object 1>,
 *           :
 *      <Pointer N>: <job object N>
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

	void *self;

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
			duk_push_error_object(ctx, DUK_ERR_ERROR, "failed to create a new thread");
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
	/* [ bufobj(thrpool) bool ] */
	dux_thrpool_link *link;
	dux_thrpool_pool *pool;
	dux_thrpool_thread *thread;
	duk_uint_t tidx;
	duk_bool_t heap_destroy = duk_get_boolean(ctx, 1);

	pool = (dux_thrpool_pool *)duk_get_buffer_data(ctx, 0, NULL);
	if (!pool)
	{
		return 0;
	}

	if (!heap_destroy)
	{
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

	pool->self = NULL;

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
	for (; pool; pool = pool->next_pool)
	{
		void *self = pool->self;
		if (!self)
		{
			continue;
		}
		duk_push_heapptr(ctx, self);
		/* [ ... obj(thrpool) ] */

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

			/* [ ... obj(thrpool) ] */
			duk_push_c_function(ctx, job->completer, 2);
			/* [ ... obj(thrpool) func ] */
			duk_push_pointer(ctx, job);
			/* [ ... obj(thrpool) func pointer ] */
			duk_get_prop(ctx, -3);
			/* [ ... obj(thrpool) func job ] */
			duk_push_pointer(ctx, job);
			/* [ ... obj(thrpool) func job pointer ] */
			duk_del_prop(ctx, -4);
			/* [ ... obj(thrpool) func job ] */
			duk_push_int(ctx, job->result);
			/* [ ... obj(thrpool) func job int ] */
			if (duk_pcall(ctx, 2) != 0)
			{
				/* [ ... obj(thrpool) err ] */
				dux_report_error(ctx);
			}
			/* [ ... obj(thrpool) retval/err ] */
			duk_pop(ctx);
			/* [ ... obj(thrpool) ] */
		}

		/* [ ... obj(thrpool) ] */
		duk_pop(ctx);
		/* [ ... ] */
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
		(void)duk_range_error(ctx,
				"max_threads must be equal or larger than min_threads");
		// never returns
	}

	if (max_threads > MAX_THREADS)
	{
		(void)duk_range_error(ctx,
				"max_threads cannot exceed %u", MAX_THREADS);
		// never returns
	}

	size = sizeof(dux_thrpool_pool) + sizeof(*pool->threads) * max_threads;
	pool = (dux_thrpool_pool *)duk_push_fixed_buffer(ctx, size);
	memset(pool, 0, size);
	/* [ ... buf ] */
	duk_push_buffer_object(ctx, -1, 0, size, DUK_BUFOBJ_ARRAYBUFFER);
	/* [ ... buf bufobj ] */
	duk_replace(ctx, -2);
	/* [ ... bufobj ] */

	pool->self = duk_get_heapptr(ctx, -1);
	pool->min_threads = min_threads;
	pool->max_threads = max_threads;

	sem_init(&pool->sem, 0, 0);
	pthread_mutex_init(&pool->lock, NULL);

	for (tidx = 0; tidx < max_threads; ++tidx)
	{
		pool->threads[tidx].index = tidx;
	}

	/* Set finalizer */
	duk_push_c_function(ctx, thrpool_finalize, 2);
	duk_set_finalizer(ctx, -2);
	/* [ ... bufobj(thrpool) ] */

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
	/* [ ... bufobj(thrpool) ] */
}

/* Queue job to thread pool */
DUK_INTERNAL void dux_thrpool_queue(duk_context *ctx,
                                    duk_idx_t pool_index,
                                    dux_thrpool_worker_t worker,
                                    dux_thrpool_completer_t completer)
{
	/* [ ... bufobj(thrpool) ... job ] */
	/*       ^pool_index         ^top  */

	duk_size_t num_blocks;
	dux_thrpool_pool *pool;
	dux_thrpool_job *job;
	duk_uarridx_t index;
	dux_thrpool_block *block;
	int pended;

	/* Get pool data */
	pool_index = duk_normalize_index(ctx, pool_index);
	pool = (dux_thrpool_pool *)duk_require_buffer(ctx, pool_index, NULL);

	/* Check job */
	if (!duk_is_array(ctx, -1))
	{
		(void)duk_type_error(ctx, "job object must be an array");
		// never returns
	}
	num_blocks = duk_get_length(ctx, -1);

	/* Add job data */
	duk_push_fixed_buffer(ctx, sizeof(*job) + sizeof(*block) * num_blocks);
	/* [ ... bufobj(thrpool) ... job buf ] */
	job = (dux_thrpool_job *)duk_get_buffer(ctx, -1, NULL);
	duk_put_prop_string(ctx, -2, DUX_IPK_THRPOOL_JOB);
	/* [ ... bufobj(thrpool) ... job ] */

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
		/* [ ... bufobj(thrpool) ... job any ] */
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
			block->pointer = duk_get_pointer(ctx, -1);
			block->length = 0;
		}
		else
		{
			// others
			block->pointer = NULL;
			block->length = 0;
		}
		duk_pop(ctx);
		/* [ ... bufobj(thrpool) ... job ] */
	}

	duk_push_pointer(ctx, job);
	/* [ ... bufobj(thrpool) ... job pointer ] */
	duk_swap_top(ctx, -2);
	/* [ ... bufobj(thrpool) ... pointer job ] */
	duk_put_prop(ctx, pool_index);
	/* [ ... bufobj(thrpool) ... ] */

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

	/* [ ... bufobj(thrpool) ... ] */
}

#endif  /* !DUX_OPT_NO_THRPOOL */
