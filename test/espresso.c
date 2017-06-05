#include "duktape.h"
#include <stdio.h>
#include <stdarg.h>

static const char ESPRESSO_DATA[] = "\xff" "espData";
static const char ESPRESSO_ROOT[] = "\xff" "espRoot";

#define INDENT 4
#define FLAG_SKIP	0x40000000

typedef struct {
	int depth;
	void *current_suite;
	int index;
	int tests;
	int passed;
	int skipped;
	int failed;
} espresso_data;

static espresso_data *espresso_get_data(duk_context *ctx)
{
	espresso_data *data;

	duk_get_global_string(ctx, ESPRESSO_DATA);
	data = (espresso_data *)duk_require_buffer(ctx, -1, NULL);
	duk_pop(ctx);

	return data;
}

static duk_ret_t espresso_define_suite(duk_context *ctx, int flags)
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
	duk_push_int(ctx, flags);
	duk_put_prop_string(ctx, 3, "flags");
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

static duk_ret_t espresso_describe(duk_context *ctx)
{
	return espresso_define_suite(ctx, 0);
}

static duk_ret_t espresso_xdescribe(duk_context *ctx)
{
	return espresso_define_suite(ctx, FLAG_SKIP);
}

static duk_ret_t espresso_define_test(duk_context *ctx, int flags)
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
	duk_push_int(ctx, flags);
	duk_put_prop_string(ctx, 3, "flags");
	duk_put_prop_index(ctx, 2, duk_get_length(ctx, 2));
	/* [ description callback arr(suite) ] */
	return 0;
}

static duk_ret_t espresso_it(duk_context *ctx)
{
	return espresso_define_test(ctx, 0);
}

static duk_ret_t espresso_xit(duk_context *ctx)
{
	return espresso_define_test(ctx, FLAG_SKIP);
}

static duk_ret_t espresso_next(duk_context *ctx);

static duk_ret_t espresso_next_resolved(duk_context *ctx)
{
	duk_set_top(ctx, 0);
	duk_push_undefined(ctx);
	/* [ undefined ] */
	return espresso_next(ctx);
}

static duk_ret_t espresso_next_rejected(duk_context *ctx)
{
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
	duk_int_t flags;
	duk_int_t result;

	/* [ value ] */
	if (!data->current_suite) {
		/* Test has done */
		return DUK_RET_ERROR;
	}

done:
	if (data->index >= 0) {
		/* End of previous test */
		printf("%*s  => ", data->depth * INDENT, "");
		if (duk_is_undefined(ctx, 0)) {
			++data->passed;
			printf("OK\n\n");
		} else {
			++data->failed;
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
		duk_get_prop_string(ctx, 1, "flags");
		/* [ arr(parent) arr(child) str int ] */
		flags = duk_require_int(ctx, 3);
		printf("%*s* %s\n", data->depth * INDENT, "",
			duk_safe_to_string(ctx, 2));
		if (flags & FLAG_SKIP) {
			printf("%*s  => SKIPPED\n\n", data->depth * INDENT, "");
			goto retry;
		}
		data->current_suite = duk_require_heapptr(ctx, 1);
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
			if (data->passed > 0) {
				printf(" %5d test%s passed\n", data->passed, data->passed >= 2 ? "s" : "");
			}
			if (data->skipped > 0) {
				printf(" %5d test%s skipped\n", data->skipped, data->skipped >= 2 ? "s" : "");
			}
			if (data->failed > 0) {
				printf(" %5d test%s failed\n", data->failed, data->failed >= 2 ? "s" : "");
			}
			printf("\n");
			return 0;
		}
		duk_get_prop_string(ctx, 0, "index");
		/* [ arr(suite) undefined pointer int ] */
		data->index = duk_require_int(ctx, 3);
		--data->depth;
		goto retry;
	}

	/* [ arr(suite) obj ] */
	++data->tests;
	duk_get_prop_string(ctx, 1, "name");
	duk_get_prop_string(ctx, 1, "flags");
	flags = duk_require_int(ctx, 3);
	duk_pop(ctx);
	/* [ arr(suite) obj str ] */
	printf("%*s- %s\n", data->depth * INDENT, "", duk_safe_to_string(ctx, 2));
	if (flags & FLAG_SKIP) {
		++data->skipped;
		printf("%*s  => SKIPPED\n\n", data->depth * INDENT, "");
		goto retry;
	}
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
	duk_int_t result;

	duk_push_current_function(ctx);
	result = duk_get_magic(ctx, 1);
	duk_set_magic(ctx, 1, result + 1);
	duk_pop(ctx);

	if (result > 0) {
		/* Called twice or more */
		return 0;
	}

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
	{ "describe", espresso_describe, 2 },
	{ "xdescribe", espresso_xdescribe, 2 },
	{ "it", espresso_it, 2 },
	{ "xit", espresso_xit, 2 },
	{ "run", espresso_run, 0 },
	{ NULL, NULL, 0 }
};

static void assert_do_throw(duk_context *ctx, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if (duk_is_undefined(ctx, -1)) {
		duk_pop(ctx);
		duk_push_vsprintf(ctx, format, args);
	}
	va_end(args);
	duk_error(ctx, DUK_ERR_ERROR, "assertion failed: %s", duk_safe_to_string(ctx, -1));
}

static duk_ret_t assert_is_ok(duk_context *ctx)
{
	/* [ value message ] */
	if (!duk_to_boolean(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is truthy", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_is_not_ok(duk_context *ctx)
{
	/* [ value message ] */
	if (duk_to_boolean(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is falsy", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_exists(duk_context *ctx)
{
	/* [ value message ] */
	if (duk_is_null_or_undefined(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is neither null nor undefined", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_not_exists(duk_context *ctx)
{
	/* [ value message ] */
	if (!duk_is_null_or_undefined(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is null or undefined", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_is_function(duk_context *ctx)
{
	/* [ value message ] */
	if (!duk_is_callable(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is a function", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_is_not_function(duk_context *ctx)
{
	/* [ value message ] */
	if (duk_is_callable(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is not a function", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_instanceof(duk_context *ctx)
{
	/* [ value constructor message ] */
	if (!duk_instanceof(ctx, 0, 1)) {
		assert_do_throw(ctx, "'%s' is an instance of '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_not_instanceof(duk_context *ctx)
{
	/* [ value constructor message ] */
	if (duk_instanceof(ctx, 0, 1)) {
		assert_do_throw(ctx, "'%s' is not an instance of '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_is_undefined(duk_context *ctx)
{
	/* [ value message ] */
	if (!duk_is_undefined(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is undefined", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_is_defined(duk_context *ctx)
{
	/* [ value message ] */
	if (duk_is_undefined(ctx, 0)) {
		assert_do_throw(ctx, "'%s' is not undefined", duk_safe_to_string(ctx, 0));
	}
	return 0;
}

static duk_ret_t assert_throws(duk_context *ctx)
{
	duk_int_t result;

	/* [ fn errorLike? errMsgMatcher message ] */
	if (duk_is_string(ctx, 1)) {
		duk_pop(ctx);
		duk_push_undefined(ctx);
		duk_insert(ctx, 1);
	}
	/* [ fn errorLike errMsgMatcher message ] */
	duk_swap(ctx, 0, 3);
	/* [ message errorLike errMsgMatcher fn ] */
	result = duk_pcall(ctx, 0);
	duk_swap(ctx, 0, 3);
	if (result == 0) {
		/* [ retval errorLike errMsgMatcher message ] */
		assert_do_throw(ctx, "fn throws");
	}
	/* [ err errorLike errMsgMatcher message ] */
	if (!duk_is_undefined(ctx, 1) && !duk_instanceof(ctx, 0, 1)) {
		assert_do_throw(ctx, "'%s' which fn throws is an instance of '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	if (!duk_is_undefined(ctx, 2)) {
		duk_dup(ctx, 0);
		duk_safe_to_string(ctx, 4);
		result = duk_strict_equals(ctx, 2, 4);
		duk_pop(ctx);
		if (!result) {
			assert_do_throw(ctx, "'%s' which fn throws matches '%s'",
				duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 2));
		}
	}
	return 0;
}

static duk_ret_t assert_does_not_throw(duk_context *ctx)
{
	duk_int_t result;

	/* [ fn errorLike string message ] */
	if (duk_is_string(ctx, 1)) {
		duk_pop(ctx);
		duk_push_undefined(ctx);
		duk_insert(ctx, 1);
	}
	duk_swap(ctx, 0, 3);
	/* [ message errorLike string fn ] */
	result = duk_pcall(ctx, 0);
	duk_swap(ctx, 0, 3);
	if (result == 0) {
		/* [ retval errorLike string message ] */
		return 0;
	}
	/* [ err errorLike string message ] */
	if (!duk_is_undefined(ctx, 1) && duk_instanceof(ctx, 0, 1)) {
		assert_do_throw(ctx, "'%s' which fn throws is not an instance of '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	if (!duk_is_undefined(ctx, 2)) {
		duk_dup(ctx, 0);
		duk_safe_to_string(ctx, 4);
		result = duk_strict_equals(ctx, 2, 4);
		duk_pop(ctx);
		if (result) {
			assert_do_throw(ctx, "'%s' which fn throws does not match '%s'",
				duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 2));
		}
	}
	return 0;
}

static duk_ret_t assert_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	if (!duk_equals(ctx, 0, 1)) {
		assert_do_throw(ctx, "expected '%s' to equal '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_not_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	if (duk_equals(ctx, 0, 1)) {
		assert_do_throw(ctx, "expected '%s' not to equal '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_strict_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	if (!duk_strict_equals(ctx, 0, 1)) {
		assert_do_throw(ctx, "expected '%s' to strictly equal '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_not_strict_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	if (duk_strict_equals(ctx, 0, 1)) {
		assert_do_throw(ctx, "expected '%s' not to strictly equal '%s'",
			duk_safe_to_string(ctx, 0), duk_safe_to_string(ctx, 1));
	}
	return 0;
}

static duk_ret_t assert_deep_equal_inner(duk_context *ctx, int not)
{
	/* [ message actual(root) expected(root) ... actual expected ] */
	if (!duk_is_object(ctx, -2) || !duk_is_object(ctx, -1)) {
		if ((duk_equals(ctx, -2, -1) == 0) ^ (not != 0)) {
failed:
			duk_dup(ctx, 0);
			assert_do_throw(ctx, "expected '%s' %sto deeply equal to '%s'",
				duk_safe_to_string(ctx, 1), not ? "not ": "",
				duk_safe_to_string(ctx, 2));
		}
		return 0;
	}
	duk_enum(ctx, -2, 0);
	/* [ message ... actual expected enum ] */
	while (duk_next(ctx, -1, 1)) {
		/* [ message ... actual expected enum key value ] */
		duk_swap(ctx, -1, -2);
		/* [ message ... actual expected enum value key ] */
		if ((duk_get_prop(ctx, -4) == 0) ^ (not != 0)) {
			goto failed;
		}
		/* [ message ... actual expected enum actual_value expected_value ] */
		assert_deep_equal_inner(ctx, not);
		duk_pop_2(ctx);
		/* [ message ... actual expected enum ] */
	}
	duk_pop(ctx);
	/* [ message ... actual expected ] */
	duk_enum(ctx, -1, 0);
	/* [ message ... actual expected enum ] */
	while (duk_next(ctx, -1, 0)) {
		/* [ message ... actual expected enum key ] */
		if ((duk_has_prop(ctx, -4) == 0) ^ (not != 0)) {
			goto failed;
		}
		/* [ message ... actual expected enum ] */
	}
	duk_pop(ctx);
	/* [ message ... actual expected ] */
	return 0;
}

static duk_ret_t assert_deep_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	duk_insert(ctx, 0);
	/* [ message actual expected ] */
	return assert_deep_equal_inner(ctx, 0);
}

static duk_ret_t assert_not_deep_equal(duk_context *ctx)
{
	/* [ actual expected message ] */
	duk_insert(ctx, 0);
	/* [ message actual expected ] */
	return assert_deep_equal_inner(ctx, 1);
}

static duk_ret_t assert_fulfill_assert(duk_context *ctx)
{
	int magic;

	/* [ value ] */
	duk_push_current_function(ctx);
	/* [ value func ] */
	duk_get_prop_string(ctx, 1, "message");
	/* [ value func message ] */
	magic = duk_get_magic(ctx, 1);
	if (magic == 0) {
		/* no value check required */
		return 0;
	}

	duk_get_prop_string(ctx, 1, "value");
	/* [ value func message expected ] */
	duk_replace(ctx, 1);
	/* [ value expected message ] */
	if (magic == 1) {
		return assert_deep_equal(ctx);
	} else {
		return assert_not_deep_equal(ctx);
	}
}

static duk_ret_t assert_unintended_rejection(duk_context *ctx)
{
	/* [ reason ] */
	duk_push_current_function(ctx);
	/* [ reason func ] */
	duk_get_prop_string(ctx, 1, "message");
	/* [ reason func message ] */
	assert_do_throw(ctx, "promise to be fulfilled but it was rejected with '%s'",
		duk_safe_to_string(ctx, 0));
}

static duk_ret_t assert_is_fulfilled(duk_context *ctx, int magic)
{
	/* [ promise value message ] */
	duk_get_prop_string(ctx, 0, "then");
	/* [ promise value message then ] */
	if (!duk_is_callable(ctx, 3)) {
		assert_do_throw(ctx, "expected '%s' is a thenable object",
			duk_safe_to_string(ctx, 0));
	}
	duk_insert(ctx, 0);
	/* [ then promise value message ] */
	duk_push_c_function(ctx, assert_unintended_rejection, 1);
	duk_insert(ctx, 2);
	/* [ then promise func value message:4 ] */
	duk_dup(ctx, 4);
	duk_put_prop_string(ctx, 2, "message");
	/* [ then promise func value message:4 ] */
	duk_push_c_function(ctx, assert_fulfill_assert, 1);
	duk_insert(ctx, 2);
	/* [ then promise func func value:4 message:5 ] */
	duk_put_prop_string(ctx, 2, "message");
	duk_put_prop_string(ctx, 2, "value");
	duk_set_magic(ctx, 2, magic);
	/* [ then promise func func ] */
	duk_call_method(ctx, 2);
	/* [ retval ] */
	return 1;
}

static duk_ret_t assert_is_fulfilled_any(duk_context *ctx)
{
	/* [ promise message ] */
	duk_push_undefined(ctx);
	duk_insert(ctx, 1);
	/* [ promise undefined message ] */
	return assert_is_fulfilled(ctx, 0);
}

static duk_ret_t assert_unintended_fulfill(duk_context *ctx)
{
	/* [ value ] */
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, 1, "message");
	/* [ value message ] */
	assert_do_throw(ctx, "promise to be rejected but it was fulfilled with '%s'",
		duk_safe_to_string(ctx, 0));
}

static duk_ret_t assert_rejection_assert(duk_context *ctx)
{
	duk_int_t result;

	/* [ reason ] */
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, 1, "errorLike");
	duk_get_prop_string(ctx, 1, "errMsgMatcher");
	duk_get_prop_string(ctx, 1, "message");
	/* [ reason func errorLike errMsgMatcher message:4 ] */
	if (!duk_is_undefined(ctx, 2) && !duk_instanceof(ctx, 0, 2)) {
		assert_do_throw(ctx, "promise to be rejected with an instance of '%s' but it was rejected with '%s'",
			duk_safe_to_string(ctx, 2), duk_safe_to_string(ctx, 0));
	}
	if (!duk_is_undefined(ctx, 3)) {
		duk_dup(ctx, 0);
		duk_safe_to_string(ctx, 5);
		result = duk_strict_equals(ctx, 3, 5);
		duk_pop(ctx);
		if (!result) {
			assert_do_throw(ctx, "promise to be rejected with a value which matches '%s' but it was rejected with '%s'",
				duk_safe_to_string(ctx, 3), duk_safe_to_string(ctx, 0));
		}
	}
	return 0;
}

static duk_ret_t assert_is_rejected(duk_context *ctx)
{
	/* [ promise errorLike? errMsgMatcher message ] */
	if (duk_is_string(ctx, 1)) {
		duk_pop(ctx);
		duk_push_undefined(ctx);
		duk_insert(ctx, 1);
	}
	/* [ promise errorLike errMsgMatcher message ] */
	duk_get_prop_string(ctx, 0, "then");
	/* [ promise errorLike errMsgMatcher message then:4 ] */
	if (!duk_is_callable(ctx, 4)) {
		assert_do_throw(ctx, "expected '%s' is a thenable object",
			duk_safe_to_string(ctx, 0));
	}
	duk_insert(ctx, 0);
	/* [ then promise errorLike errMsgMatcher message:4 ] */
	duk_push_c_function(ctx, assert_unintended_fulfill, 1);
	duk_dup(ctx, 4);
	duk_put_prop_string(ctx, 5, "message");
	/* [ then promise errorLike errMsgMatcher message:4 func:5 ] */
	duk_insert(ctx, 2);
	/* [ then promise func errorLike errMsgMatcher:4 message:5 ] */
	duk_push_c_function(ctx, assert_rejection_assert, 1);
	duk_insert(ctx, 3);
	/* [ then promise func func errorLike:4 errMsgMatcher:5 message:6 ] */
	duk_put_prop_string(ctx, 3, "message");
	duk_put_prop_string(ctx, 3, "errMsgMatcher");
	duk_put_prop_string(ctx, 3, "errorLike");
	/* [ then promise func func ] */
	duk_call_method(ctx, 2);
	/* [ retval ] */
	return 1;
}

static duk_ret_t assert_becomes(duk_context *ctx)
{
	return assert_is_fulfilled(ctx, 1);
}

static duk_ret_t assert_does_not_become(duk_context *ctx)
{
	return assert_is_fulfilled(ctx, 2);
}

static const duk_function_list_entry assert_funcs[] = {
	{ "isOk", assert_is_ok, 2 },
	{ "isNotOk", assert_is_not_ok, 2 },
	{ "exists", assert_exists, 2 },
	{ "notExists", assert_not_exists, 2 },
	{ "isFunction", assert_is_function, 2 },
	{ "isNotFunction", assert_is_not_function, 2 },
	{ "instanceOf", assert_instanceof, 3 },
	{ "notInstanceOf", assert_not_instanceof, 3 },
	{ "isUndefined", assert_is_undefined, 2 },
	{ "isDefined", assert_is_defined, 2 },
	{ "throws", assert_throws, 4 },
	{ "doesNotThrow", assert_does_not_throw, 4 },
	{ "equal", assert_equal, 3 },
	{ "notEqual", assert_not_equal, 3 },
	{ "strictEqual", assert_strict_equal, 3 },
	{ "notStrictEqual", assert_not_strict_equal, 3 },
	{ "deepEqual", assert_deep_equal, 3 },
	{ "notDeepEqual", assert_not_deep_equal, 3 },
	{ "isFulfilled", assert_is_fulfilled_any, 2 },
	{ "isRejected", assert_is_rejected, 4 },
	{ "becomes", assert_becomes, 3 },
	{ "doesNotBecome", assert_does_not_become, 3 },
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
	data->passed = 0;
	data->skipped = 0;
	data->failed = 0;
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

int espresso_is_finished(duk_context *ctx, int *passed, int *skipped, int *failed)
{
	espresso_data *data = espresso_get_data(ctx);
	if (passed) {
		*passed = data->passed;
	}
	if (skipped) {
		*skipped = data->skipped;
	}
	if (failed) {
		*failed = data->failed;
	}
	return (data->current_suite == NULL);
}

