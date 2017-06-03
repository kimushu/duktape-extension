#include "dux_stream.h"
#include "dux_common.h"
#include "dux_private.h"
#include "dux_thrpool.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>

extern void top_level_error(duk_context *ctx);

static const char *const IPK_FD   = "\xff" "kFd";
static const char *const IPK_POOL = "\xff" "kPl";

enum
{
	STREAM_BLKIDX_FD = 0,
	STREAM_BLKIDX_BUFFER,
	STREAM_BLKIDX_ENCODING,
	STREAM_BLKIDX_CALLBACK,
	STREAM_BLKIDX_THIS,
	STREAM_NUM_BLOCKS,
};

static duk_ret_t stream_finalizer(duk_context *ctx)
{
	/* [ obj ] */
	if (duk_get_prop_string(ctx, 0, IPK_FD))
	{
		/* [ obj int ] */
		int fd = duk_get_int(ctx, 1);
		if (fd >= 0)
		{
			close(fd);
		}
	}
	return 0;
}

static duk_ret_t stream_constructor(duk_context *ctx)
{
	if (duk_is_number(ctx, 0))
	{
		/* (1) [ number undefined ] */
		int fd;
		duk_push_this(ctx);
		/* [ number undefined this ] */
		fd = duk_require_int(ctx, 0);
		if (fd < 0)
		{
			return DUK_RET_RANGE_ERROR;
		}
		duk_push_int(ctx, fd);
		/* [ number undefined this int ] */
		duk_put_prop_string(ctx, 2, IPK_FD);
		/* [ number undefined this ] */
	}
	else
	{
		/* (2) [ string number ] */
		const char *path = duk_require_string(ctx, 0);
		int flags = duk_require_int(ctx, 1);
		int fd = open(path, flags);
		if (fd < 0)
		{
			return DUK_RET_API_ERROR;
		}
		duk_push_this(ctx);
		/* [ string number this ] */
		duk_push_int(ctx, fd);
		/* [ string number this int ] */
		duk_put_prop_string(ctx, 2, IPK_FD);
		/* [ string number this ] */
		duk_push_c_function(ctx, stream_finalizer, 1);
		/* [ string number this finalizer ] */
		duk_set_finalizer(ctx, 2);
		/* [ string number this ] */
	}

	/* [ any any this ] */
	dux_push_thrpool(ctx, 1, 1);
	/* [ any any this thrpool ] */
	duk_put_prop_string(ctx, 2, IPK_POOL);
	/* [ any any this ] */
	return 0;
}

static duk_ret_t stream_proto_on(duk_context *ctx)
{
	/* [ string func ] */
	return DUK_RET_UNSUPPORTED_ERROR;
}

static duk_ret_t stream_proto_read(duk_context *ctx)
{
	/* [ size/undefined ] */
	return DUK_RET_UNSUPPORTED_ERROR;
}

/*
 * Thread pool worker for Stream.prototype.write
 */
duk_int_t stream_write_worker(const dux_thrpool_block_t *blocks, duk_size_t num_blocks)
{
	int fd;
	const char *ptr;
	ssize_t len;
	ssize_t written;

	if (num_blocks != STREAM_NUM_BLOCKS)
	{
		return -EBADF;
	}

	fd = blocks[STREAM_BLKIDX_FD].uint;
	ptr = (const char *)blocks[STREAM_BLKIDX_BUFFER].pointer;
	len = blocks[STREAM_BLKIDX_BUFFER].length;

	while (len > 0)
	{
		written = write(fd, ptr, len);
		if (written < 0)
		{
			int result = errno;

			if ((result == EAGAIN) || (result == EWOULDBLOCK))
			{
				sched_yield();
				continue;
			}

			return -result;
		}

		ptr += written;
		len -= written;
	}

	return 0;
}

/*
 * Thread pool completer for Stream.prototype.write
 */
duk_ret_t stream_write_completer(duk_context *ctx)
{
	/* [ job int ] */
	duk_int_t ret = duk_require_int(ctx, 1);
	duk_get_prop_index(ctx, 0, STREAM_BLKIDX_CALLBACK);
	/* [ job int func ] */

	if (ret == 0)
	{
		// Succeeded
		if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS)
		{
			/* [ job int err ] */
			top_level_error(ctx);
		}
		/* [ job int retval/err ] */
	}
	else
	{
		// Failed
		duk_push_error_object(ctx, DUK_ERR_API_ERROR, "write failed (ret=%d)", ret);
		/* [ job int func err ] */
		top_level_error(ctx);
	}

	return 0;
}

static duk_ret_t stream_proto_write(duk_context *ctx)
{
	/* [ string/buffer string/func/undefined func/undefined ] */

	if (!duk_is_string(ctx, 1) && !duk_is_null_or_undefined(ctx, 1))
	{
		duk_push_undefined(ctx);
		duk_replace(ctx, 2);
		duk_swap(ctx, 1, 2);
	}
	/* [ string/buffer string/undefined func/undefined ] */

	duk_push_array(ctx);
	/* [ string/buffer string/undefined func/undefined arr ] */
	duk_insert(ctx, 0);
	/* [ arr string/buffer string/undefined func/undefined ] */
	duk_put_prop_index(ctx, 0, STREAM_BLKIDX_CALLBACK);
	/* [ arr string/buffer string/undefined ] */
	duk_put_prop_index(ctx, 0, STREAM_BLKIDX_ENCODING);
	/* [ arr string/buffer ] */
	duk_put_prop_index(ctx, 0, STREAM_BLKIDX_BUFFER);
	/* [ arr ] */
	duk_push_this(ctx);
	/* [ arr this ] */
	duk_get_prop_string(ctx, 1, IPK_FD);
	/* [ arr this int ] */
	duk_put_prop_index(ctx, 0, STREAM_BLKIDX_FD);
	/* [ arr this ] */
	duk_get_prop_string(ctx, 1, IPK_POOL);
	/* [ arr this thrpool ] */
	duk_insert(ctx, 0);
	/* [ thrpool arr this ] */
	duk_put_prop_index(ctx, 1, STREAM_BLKIDX_THIS);
	/* [ thrpool arr ] */
	dux_thrpool_queue(ctx, 0, stream_write_worker, stream_write_completer);
	/* [ thrpool ] */

	duk_push_true(ctx);
	return 1;	/* return true; */
}

static duk_ret_t stream_proto_fd_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	duk_get_prop_string(ctx, 0, IPK_FD);
	/* [ this int ] */
	duk_require_int(ctx, 1);
	return 1;	/* return int; */
}

static duk_ret_t stream_proto_readable_getter(duk_context *ctx)
{
	/* [  ] */
	int fd;
	int flags;

	duk_push_this(ctx);
	/* [ this ] */
	duk_get_prop_string(ctx, 0, IPK_FD);
	/* [ this int ] */
	fd = duk_require_int(ctx, 1);
	if (fd < 0)
	{
		return DUK_RET_API_ERROR;
	}

	flags = fcntl(fd, F_GETFL);
	duk_push_boolean(ctx, ((flags + 1) & 1));
	/* [ this int bool ] */
	return 1;	/* return bool; */
}

static duk_ret_t stream_proto_writable_getter(duk_context *ctx)
{
	/* [  ] */
	int fd;
	int flags;

	duk_push_this(ctx);
	/* [ this ] */
	duk_get_prop_string(ctx, 0, IPK_FD);
	/* [ this int ] */
	fd = duk_require_int(ctx, 1);
	if (fd < 0)
	{
		return DUK_RET_API_ERROR;
	}

	flags = fcntl(fd, F_GETFL);
	duk_push_boolean(ctx, ((flags + 1) & 2));
	/* [ this int bool ] */
	return 1;	/* return bool; */
}

static const duk_function_list_entry stream_proto_funcs[] = {
	{ "on", stream_proto_on, 2 },
	{ "read", stream_proto_read, 1 },
	{ "write", stream_proto_write, 3 },
	{ NULL, NULL, 0 }
};

static const dux_property_list_entry stream_proto_props[] = {
	{ "fd", stream_proto_fd_getter, NULL },
	{ "readable", stream_proto_readable_getter, NULL },
	{ "writable", stream_proto_writable_getter, NULL },
	{ NULL, NULL, NULL }
};

void dux_push_stream_constructor(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	if (!duk_get_prop_string(ctx, -1, DUX_INTRINSIC_STREAM))
	{
		/* [ ... stash undefined ] */
		duk_pop(ctx);
		/* [ ... stash ] */
		duk_push_c_function(ctx, stream_constructor, 2);
		dux_push_named_c_constructor(ctx, "Stream", stream_constructor, 2,
				NULL, stream_proto_funcs, NULL, stream_proto_props);
		duk_dup_top(ctx);
		/* [ ... stash constructor constructor ] */
		duk_put_prop_string(ctx, -3, DUX_INTRINSIC_STREAM);
		/* [ ... stash constructor ] */
	}
	/* [ ... stash constructor ] */
	duk_remove(ctx, -2);
	/* [ ... constructor ] */
}

