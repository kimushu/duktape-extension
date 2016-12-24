#include "dux_internal.h"
#include <stdio.h>

DUK_LOCAL const char DUX_IPK_TABLE[]  = DUX_IPK("bTable");
DUK_LOCAL const char DUX_IPK_STORE[]  = DUX_IPK("bStore");

/*
 * Initialize Duktape extension modules
 */
DUK_EXTERNAL duk_errcode_t dux_initialize(duk_context *ctx)
{
#define INIT(module) \
	do { \
		duk_errcode_t result = dux_##module##_init(ctx); \
		if (result != DUK_ERR_NONE) \
		{ \
			return result; \
		} \
	} while (0)

	INIT(promise);
	INIT(thrpool);

	INIT(console);
	INIT(process);
	INIT(timer);
	INIT(util);

	INIT(paraio);
	INIT(i2ccon);

	INIT(peridot);

#undef INIT

	return DUK_ERR_NONE;
}

/*
 * Tick handler
 */
DUK_EXTERNAL duk_bool_t dux_tick(duk_context *ctx)
{
	duk_int_t result = 0;

#define TICK(module) \
	do { \
		result |= dux_##module##_tick(ctx); \
		if (result & DUX_TICK_RET_ABORT) \
		{ \
			return 0; \
		} \
	} while (0)

	TICK(promise);
	TICK(thrpool);

	TICK(console);
	TICK(process);
	TICK(timer);
	TICK(util);

	TICK(paraio);
	TICK(i2ccon);

	TICK(peridot);

#undef TICK

	return (result & DUX_TICK_RET_CONTINUE) ? 1 : 0;
}

/*
 * Top level error reporter
 */
DUK_INTERNAL void dux_report_error(duk_context *ctx)
{
	/* TODO */
	fprintf(stderr, "Uncaught error: %s\n", duk_safe_to_string(ctx, -1));
}

/*
 * Empty function for Object.create
 */
DUK_LOCAL duk_ret_t dux_no_operation(duk_context *ctx)
{
	return 0;
}

/*
 * Push inherited prototype
 */
DUK_INTERNAL void dux_push_inherited_object(duk_context *ctx, duk_idx_t super_idx)
{
	/* [ ... super ... constructor ] */
	/*  super_idx^     ^top          */
	super_idx = duk_normalize_index(ctx, super_idx);
	duk_push_c_function(ctx, dux_no_operation, 0);
	/* [ ... super ... constructor func ] */
	duk_get_prop_string(ctx, super_idx, "prototype");
	/* [ ... super ... constructor func super_proto ] */
	duk_put_prop_string(ctx, -2, "prototype");
	/* [ ... super ... constructor func ] */
	duk_new(ctx, 0);
	/* [ ... super ... constructor inherited ] */
	if (0)
	{
		duk_dup(ctx, -2);
		/* [ ... super ... constructor inherited constructor ] */
		duk_put_prop_string(ctx, -2, "constructor");
		/* [ ... super ... constructor inherited ] */
	}
}

