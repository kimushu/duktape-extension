#ifndef DUX_STREAM_H_
#define DUX_STREAM_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DUX_STREAM_FLAG_OBJECT_MODE = (1 << 0),
    DUX_STREAM_FLAG_ENDED       = (1 << 1),
};

typedef struct dux_stream_writable_data_s {
    duk_uint_t corked;
    duk_uint_t flags;
    duk_uint_t highWaterMark;
    duk_uint_t length;
    duk_ret_t (*write)(duk_context *ctx);
    duk_ret_t (*writev)(duk_context *ctx);
} dux_stream_writable_data;

typedef struct dux_stream_readable_data_s {
} dux_stream_readable_data;

extern void dux_push_stream_constructor(duk_context *ctx);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_STREAM_H_ */
