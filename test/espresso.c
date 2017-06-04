#include "duktape.h"
#include <stdio.h>

static const char ESPRESSO_DATA[] = "\xff" "espData";
static const char ESPRESSO_ROOT[] = "\xff" "espRoot";

#define INDENT 4

typedef struct {
	int depth;
	void *current_suite;
	int index;
	int tests;
	int ok;
	int ng;
} espresso_data;

static espresso_data *espresso_get_data(duk_context *ctx)
{
	espresso_data *data;

	duk_get_global_string(ctx, ESPRESSO_DATA);
	data = (espresso_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop(ctx);

	return data;
}

static duk_ret_t espresso_suite(duk_context *ctx)
{
	espresso_data *data;
	void *parent_suite;

	/* [ description callback ] */
	data = espresso_get_data(ctx);
	if (data->index >= 0) {
		duk_error(ctx, DUK_ERR_ERROR, "describe(): espresso test has been run");
	}
	parent_suite = data->current_suite;
	duk_push_heapptr(ctx, parent_suite);
	/* [ description callback arr(parent) ] */
	duk_push_array(ctx);
	/* [ description callback arr(parent) arr(child) ] */
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, 3, "name");
	duk_push_pointer(ctx, parent_suite);
	duk_put_prop_string(ctx, 3, "parent");
	duk_push_int(ctx, duk_get_length(ctx, 2));
	duk_put_prop_string(ctx, 3, "index");
	data->current_suite = duk_require_heapptr(ctx, 3);
	++data->depth;
	/* [ description callback arr(parent) arr(child) ] */
	duk_put_prop_index(ctx, 2, duk_get_length(ctx, 2));
	/* [ description callback arr(parent) ] */
	duk_pop(ctx);
	/* [ description callback ] */
	duk_call(ctx, 0);
	/* [ description retval ] */
	--data->depth;
	data->current_suite = parent_suite;
	return 0;
}

static duk_ret_t espresso_test(duk_context *ctx)
{
	espresso_data *data;

	/* [ expectation callback ] */
	data = espresso_get_data(ctx);
	if (data->index >= 0) {
		duk_error(ctx, DUK_ERR_ERROR, "it(): espresso test has been run");
	}
	duk_push_heapptr(ctx, data->current_suite);
	/* [ description callback arr(suite) obj ] */
	duk_push_object(ctx);
	/* [ description callback arr(suite) obj ] */
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, 3, "name");
	duk_dup(ctx, 1);
	duk_put_prop_string(ctx, 3, "body");
	duk_put_prop_index(ctx, 2, duk_get_length(ctx, 2));
	++data->tests;
	/* [ description callback arr(suite) ] */
	return 0;
}

static duk_ret_t espresso_next(duk_context *ctx);

static duk_ret_t espresso_next_resolved(duk_context *ctx)
{
	duk_int_t result;

	/* [ value ] */
	duk_push_current_function(ctx);
	result = duk_get_magic(ctx, 1);
	duk_set_magic(ctx, 1, result + 1);

	if (result > 0) {
		/* Called twice or more */
		return 0;
	}

	duk_set_top(ctx, 0);
	duk_push_undefined(ctx);
	/* [ undefined ] */
	return espresso_next(ctx);
}

static duk_ret_t espresso_next_rejected(duk_context *ctx)
{
	duk_int_t result;

	/* [ reason ] */
	duk_push_current_function(ctx);
	result = duk_get_magic(ctx, 1);
	duk_set_magic(ctx, 1, result + 1);
	duk_pop(ctx);

	if (result > 0) {
		/* Called twice or more */
		return 0;
	}

	if (duk_is_undefined(ctx, 0)) {
		/* [ undefined ] */
		duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "rejected with undefined reason");
		duk_replace(ctx, 0);
	}
	/* [ err ] */
	return espresso_next(ctx);
}

static duk_ret_t espresso_next_inner(duk_context *ctx, espresso_data *data)
{
	int async;
	duk_int_t result;

	/* [ value ] */

	duk_push_current_function(ctx);
	result = duk_get_magic(ctx, 1);
	duk_set_magic(ctx, 1, result + 1);
	duk_pop(ctx);

	if (result > 0) {
		/* Called twice or more */
		return 0;
	}

	if (!data->current_suite) {
		/* Test has done */
		return DUK_RET_ERROR;
	}

done:
	if (data->index >= 0) {
		/* End of previous test */
		printf("%*s  => ", data->depth * INDENT, "");
		if (duk_is_undefined(ctx, 0)) {
			++data->ok;
			printf("OK\n\n");
		} else {
			++data->ng;
			printf("NG (%s)\n\n", duk_safe_to_string(ctx, 0));
		}
	}

retry:
	++data->index;
	duk_set_top(ctx, 0);
	duk_push_heapptr(ctx, data->current_suite);
	/* [ arr(suite) ] */
	duk_get_prop_index(ctx, 0, data->index);

	if (duk_is_array(ctx, 1)) {
		/* [ arr(parent) arr(child) ] */
		duk_get_prop_string(ctx, 1, "name");
		/* [ arr(parent) arr(child) str ] */
		data->current_suite = duk_require_heapptr(ctx, 1);
		printf("%*s* %s\n", data->depth * INDENT, "", duk_safe_to_string(ctx, 2));
		++data->depth;
		data->index = -1;
		goto retry;
	}

	if (duk_is_undefined(ctx, 1)) {
		/* [ arr(suite) undefined ] */
		duk_get_prop_string(ctx, 0, "parent");
		/* [ arr(suite) undefined pointer ] */
		data->current_suite = duk_get_pointer(ctx, 2);
		if (!data->current_suite) {
			/* all tests done */
			printf("----------------------------------------------------------------\n");
			printf(" %5d tests passed\n %5d tests failed\n\n", data->ok, data->ng);
			return 0;
		}
		duk_get_prop_string(ctx, 0, "index");
		/* [ arr(suite) undefined pointer int ] */
		data->index = duk_require_int(ctx, 3);
		--data->depth;
		goto retry;
	}

	/* [ arr(suite) obj ] */
	duk_get_prop_string(ctx, 1, "name");
	/* [ arr(suite) obj str ] */
	printf("%*s- %s\n", data->depth * INDENT, "", duk_safe_to_string(ctx, 2));
	duk_get_prop_string(ctx, 1, "body");
	/* [ arr(suite) obj str func(body) ] */
	duk_get_prop_string(ctx, 3, "length");
	async = (duk_get_int(ctx, 4) >= 1) ? 1 : 0;
	duk_pop(ctx);
	duk_push_c_function(ctx, espresso_next, 1);
	duk_replace(ctx, 2);
	/* [ arr(suite) obj func(next) func(body) ] */
	if (async) {
		duk_dup(ctx, 2);
		result = duk_pcall(ctx, 1);
	} else {
		result = duk_pcall(ctx, 0);
	}
	if (result != 0) {
		/* [ arr(suite) obj func(next) err ] */
		duk_call(ctx, 1);
		return 0;
	}

	/* [ arr(suite) obj func(next) retval ] */
	if (!duk_is_null_or_undefined(ctx, 3)) {
		duk_get_prop_string(ctx, 3, "then");
		if (duk_is_callable(ctx, 4)) {
			/* async test by Thenable object */
			async = 2;
			/* [ arr(suite) obj func(next) thenable func(then):4 ] */
			duk_swap(ctx, 3, 4);
			/* [ arr(suite) obj func(next) func(then) thenable:4 ] */
			duk_push_c_function(ctx, espresso_next_resolved, 1);
			duk_push_c_function(ctx, espresso_next_rejected, 1);
			duk_copy(ctx, 6, 2);
			result = duk_pcall_method(ctx, 2);
			if (result != 0) {
				/* [ arr(suite) obj func(rejected) err ] */
				duk_call(ctx, 1);
				return 0;
			}
		}
	}

	if (async) {
		return 0;
	}

	/* sync test succeeded */
	duk_set_top(ctx, 0);
	duk_push_undefined(ctx);
	goto done;
}

static duk_ret_t espresso_next(duk_context *ctx)
{
	return espresso_next_inner(ctx, espresso_get_data(ctx));
}

static duk_ret_t espresso_run(duk_context *ctx)
{
	espresso_data *data;

	/* [  ] */
	data = espresso_get_data(ctx);
	if (data->index >= 0) {
		duk_error(ctx, DUK_ERR_ERROR, "run(): espresso test has been run");
	}
	if (data->depth > 0) {
		duk_error(ctx, DUK_ERR_ERROR, "run(): cannot be called inside suite");
	}

	printf("----------------------------------------------------------------\n");
	data->index = -1;
	duk_push_c_function(ctx, espresso_next, 1);
	duk_push_undefined(ctx);
	/* [ func undefined ] */
	duk_call(ctx, 1);
	return 0;
}

static const duk_function_list_entry espresso_funcs[] = {
	{ "describe", espresso_suite, 2 },
	{ "it", espresso_test, 2 },
	{ "run", espresso_run, 0 },
	{ NULL, NULL, 0 }
};

static void assert_do_throw(duk_context *ctx)
{
	duk_error(ctx, DUK_ERR_ERROR, "assertion failed: %s", duk_safe_to_string(ctx, -1));
}

static duk_ret_t assert_is_ok(duk_context *ctx)
{
	/* [ expression message ] */
	if (!duk_to_boolean(ctx, 0)) {
		assert_do_throw(ctx);
	}
	return 0;
}

static duk_ret_t assert_is_not_ok(duk_context *ctx)
{
	/* [ expression message ] */
	if (duk_to_boolean(ctx, 0)) {
		assert_do_throw(ctx);
	}
	return 0;
}

static duk_ret_t assert_is_function(duk_context *ctx)
{
	/* [ value message ] */
	if (!duk_is_callable(ctx, 0)) {
		assert_do_throw(ctx);
	}
	return 0;
}

static const duk_function_list_entry assert_funcs[] = {
	{ "isOk", assert_is_ok, 2 },
	{ "isNotOk", assert_is_not_ok, 2 },
	{ "isFunction", assert_is_function, 2 },
	{ NULL, NULL, 0 }
};

void espresso_init(duk_context *ctx)
{
	void *root_suite;
	espresso_data *data;

	/* [ ... ] */
	duk_push_global_object(ctx);
	/* [ ... global ] */
	duk_push_array(ctx);
	/* [ ... global arr ] */
	root_suite = duk_require_heapptr(ctx, -1);
	duk_put_prop_string(ctx, -2, ESPRESSO_ROOT);
	/* [ ... global ] */
	duk_push_fixed_buffer(ctx, sizeof(espresso_data));
	/* [ ... global buf ] */
	data = (espresso_data *)duk_require_buffer(ctx, -1, NULL);
	data->depth = 0;
	data->current_suite = root_suite;
	data->index = -1;
	data->ok = 0;
	data->ng = 0;
	duk_put_prop_string(ctx, -2, ESPRESSO_DATA);
	/* [ ... global ] */
	duk_put_function_list(ctx, -1, espresso_funcs);
	duk_push_c_function(ctx, assert_is_ok, 2);
	/* [ ... global assert ] */
	duk_put_function_list(ctx, -1, assert_funcs);
	/* [ ... global assert ] */
	duk_put_prop_string(ctx, -2, "assert");
	/* [ ... global ] */
	duk_pop(ctx);
	/* [ ... ] */
}

int espresso_is_finished(duk_context *ctx, int *all_passed)
{
	espresso_data *data = espresso_get_data(ctx);
	if (all_passed) {
		*all_passed = (data->tests == data->ok);
	}
	return (data->current_suite == NULL);
}

