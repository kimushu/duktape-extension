#include "duktape.h"
#include "dukext.h"
#include "../src/dux_internal.h"
#include "espresso.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char CJS_PROLOGUE[] = "(function(require,module,exports){";
static const int CJS_PROLOGUE_LEN = sizeof(CJS_PROLOGUE) - 1;
static const char CJS_EPILOGUE[] = "})(require,m={exports:{}},m.exports)";
static const int CJS_EPILOGUE_LEN = sizeof(CJS_EPILOGUE) - 1;

static duk_context *g_ctx;

static duk_ret_t eval_mod_caller(duk_context *ctx)
{
	int result = -1;
	/* [ obj type data len|undefined ] */
	switch (duk_get_int(ctx, 1)) {
	case 0:
		dux_eval_module_file(ctx, duk_get_string(ctx, 2));
		break;
	case 1:
		dux_eval_module_file_noresult(ctx, duk_get_string(ctx, 2));
		break;
	case 2:
		dux_eval_module_string(ctx, duk_get_string(ctx, 2));
		break;
	case 3:
		dux_eval_module_string_noresult(ctx, duk_get_string(ctx, 2));
		break;
	case 4:
		dux_eval_module_lstring(ctx, duk_get_string(ctx, 2), duk_get_uint(ctx, 3));
		break;
	case 5:
		dux_eval_module_lstring_noresult(ctx, duk_get_string(ctx, 2), duk_get_uint(ctx, 3));
		break;
	case 10:
		result = dux_peval_module_file(ctx, duk_get_string(ctx, 2));
		break;
	case 11:
		result = dux_peval_module_file_noresult(ctx, duk_get_string(ctx, 2));
		break;
	case 12:
		result = dux_peval_module_string(ctx, duk_get_string(ctx, 2));
		break;
	case 13:
		result = dux_peval_module_string_noresult(ctx, duk_get_string(ctx, 2));
		break;
	case 14:
		result = dux_peval_module_lstring(ctx, duk_get_string(ctx, 2), duk_get_uint(ctx, 3));
		break;
	case 15:
		result = dux_peval_module_lstring_noresult(ctx, duk_get_string(ctx, 2), duk_get_uint(ctx, 3));
		break;
	}
	/* [ obj type data len|undefined (retval|err) ] */
	duk_push_int(ctx, result);
	duk_put_prop_string(ctx, 0, "result");
	duk_push_int(ctx, duk_get_top(ctx) - 4);
	duk_put_prop_string(ctx, 0, "pushed");
	if (duk_get_top(ctx) > 4) {
		duk_put_prop_string(ctx, 0, "retval");
	}
	duk_set_top(ctx, 1);
	return 1;
}

static void test_work_finalizer(duk_context *ctx, dux_work_t *req)
{
	unsigned char *buf = *((unsigned char **)req);
	buf[1] = 1;
}

static duk_int_t test_work_cb(dux_work_t *req)
{
	unsigned char *buf = *((unsigned char **)req);
	usleep(1000 * buf[0]);
	if (dux_work_aborting(req)) {
		return -1;
	}
	return buf[0];
}

static duk_int_t test_after_work_cb(duk_context *ctx, dux_work_t *req)
{
	/* [ int arg1 ... argN ] */
	/*       ^func           */
	duk_swap(ctx, 0, 1);
	/* [ func(arg1) int arg2 ... argN ] */
	return duk_pcall(ctx, duk_get_top(ctx) - 1);
}

static duk_ret_t queue_work_caller(duk_context *ctx)
{
	/* [ buf arg1 ... argN ] */
	/*       ^func           */
	unsigned char *buf = (unsigned char *)duk_require_buffer_data(ctx, 0, NULL);
	duk_remove(ctx, 0);
	/* [ arg1 ... argN ] */

	dux_queue_work(ctx, (dux_work_t *)&buf, sizeof(unsigned char *), test_work_cb, test_after_work_cb, duk_get_top(ctx), test_work_finalizer);
	return 0;
}

static duk_ret_t test_file_reader(duk_context *ctx, const char *path)
{
	static const char *maps[] = {
		"/mod1",
		"/mod1.js",
		"/mod1.json",
		"/json1.json",
		"/mod2.js",
		"/sub/mod3.js",
		"/mod4.js",
		NULL,
	};
	char name[256];
	FILE *fp;
	const char **item;
	void *buf;
	size_t len;

	for (item = maps; *item; ++item) {
		if (strcmp(*item, path) == 0) {
			goto found;
		}
	}
	duk_push_error_object(ctx, DUK_ERR_ERROR, "File not found: %s", path);
	return DUK_EXEC_ERROR;

found:
	strcpy(name, "fs");
	strcat(name, path);
	fp = fopen(name, "rb");
	if (!fp) {
		return duk_error(ctx, DUK_ERR_ERROR, "TEST PANIC: not found: %s", name);
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	buf = duk_push_fixed_buffer(ctx, len);
	fseek(fp, 0, SEEK_SET);
	fread(buf, 1, len, fp);
	duk_buffer_to_string(ctx, -1);
	return DUK_EXEC_SUCCESS;
}

static const dux_file_accessor file_accessor = {
	.reader = test_file_reader,
};

static void my_fatal(void *udata, const char *msg)
{
	fprintf(stderr, "**** Duktape Fatal Error (%p, %s) ****\n", udata, msg);
	duk_push_context_dump(g_ctx);
	fprintf(stderr, "%s\n", duk_safe_to_string(g_ctx, -1));
	exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	FILE *fp;
	int length;
	char *source, *src_body;
	duk_context *ctx;
	int test_done;
	int failed = 0;

	ctx = duk_create_heap(NULL, NULL, NULL, NULL, my_fatal);
	g_ctx = ctx;
	fprintf(stderr, "INFO: heap created\n");

	dux_initialize(ctx, &file_accessor);
	fprintf(stderr, "INFO: dux initialized\n");

	espresso_init(ctx);
	fprintf(stderr, "INFO: espresso test framework initialized\n");

	duk_push_c_function(ctx, eval_mod_caller, 4);
	duk_put_global_string(ctx, "__eval_mod_caller");

	duk_push_c_function(ctx, queue_work_caller, DUK_VARARGS);
	duk_put_global_string(ctx, "__queue_work_caller");

	for (i = 1; i < argc; ++i) {
		fp = fopen(argv[i], "rb");
		if(!fp) {
			fprintf(stderr, "cannot open js file\n");
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		source = (char *)malloc(length + CJS_PROLOGUE_LEN + CJS_EPILOGUE_LEN);
		if (!source) {
			fclose(fp);
			fprintf(stderr, "cannot allocate memory for source\n");
			exit(1);
		}
		memcpy(source, CJS_PROLOGUE, CJS_PROLOGUE_LEN);
		src_body = source + CJS_PROLOGUE_LEN;
		length = fread(src_body, 1, length, fp);
		memcpy(src_body + length, CJS_EPILOGUE, CJS_EPILOGUE_LEN);
		fclose(fp);

		fprintf(stderr, "INFO: eval %s\n", argv[i]);
		duk_eval_lstring_noresult(ctx, source, length + CJS_PROLOGUE_LEN + CJS_EPILOGUE_LEN);
		free(source);
	}

	fprintf(stderr, "INFO: run espresso\n");
	duk_eval_string_noresult(ctx, "run();");

	fprintf(stderr, "INFO: event loop starting\n");
	for (;;) {
		int stack_before, stack_after, tick_result;
		test_done = espresso_tick(ctx, NULL, NULL, &failed);
		stack_before = duk_get_top(ctx);
		tick_result = dux_tick(ctx);
		stack_after = duk_get_top(ctx);
		if (stack_before != stack_after) {
			fprintf(stderr, "ERROR: stack length changed in tick handler (%d -> %d)\n",
				stack_before, stack_after
			);
			break;
		}
		if (!tick_result) {
			if (!espresso_exit_handler(ctx)) {
				break;
			}
		} else if (test_done) {
			fprintf(stderr, "WARN: async operations left after completion of test\n");
			break;
		}
	}
	if (!test_done) {
		fprintf(stderr, "ERROR: async operations passed before completion of test\n");
	}
	fprintf(stderr, "INFO: event loop finished\n");
	g_ctx = NULL;
	duk_destroy_heap(ctx);
	fprintf(stderr, "INFO: heap destroyed\n");
	return (test_done && failed == 0) ? 0 : 1;
}

