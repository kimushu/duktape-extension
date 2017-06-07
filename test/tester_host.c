#include "duktape.h"
#include "dux.h"
#include <stdio.h>

static duk_context *g_ctx;

extern void espresso_init(duk_context *ctx);
extern int espresso_is_finished(duk_context *ctx, int *passed, int *skipped, int *failed);

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
	char *source;
	duk_context *ctx;
	int test_done;
	int failed = 0;

	ctx = duk_create_heap(NULL, NULL, NULL, NULL, my_fatal);
	g_ctx = ctx;
	fprintf(stderr, "INFO: heap created\n");

	dux_initialize(ctx);
	fprintf(stderr, "INFO: dux initialized\n");

	espresso_init(ctx);
	fprintf(stderr, "INFO: espresso test framework initialized\n");

	for (i = 1; i < argc; ++i) {
		fp = fopen(argv[i], "rb");
		if(!fp) {
			fprintf(stderr, "cannot open js file\n");
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		source = (char *)malloc(length);
		if (!source) {
			fclose(fp);
			fprintf(stderr, "cannot allocate memory for source\n");
			exit(1);
		}
		length = fread(source, 1, length, fp);
		fclose(fp);

		fprintf(stderr, "INFO: eval %s\n", argv[i]);
		duk_eval_lstring_noresult(ctx, source, length);
		free(source);
	}

	fprintf(stderr, "INFO: run espresso\n");
	duk_eval_string_noresult(ctx, "run();");

	fprintf(stderr, "INFO: event loop starting\n");
	for (;;) {
		test_done = espresso_is_finished(ctx, NULL, NULL, &failed);
		if (!dux_tick(ctx)) {
			break;
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

