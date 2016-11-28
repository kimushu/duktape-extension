#include "dux_internal.h"

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

	INIT(timer);
	INIT(process);
	INIT(util);

#undef INIT

	return DUK_ERR_NONE;
}

/*
 * Tick handler
 */
DUX_EXTENRAL duk_bool_t dux_tick(duk_context *ctx)
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

	TICK(timer);
	TICK(process);

#undef TICK

	return (result & DUX_TICK_RET_CONTINUE) ? 1 : 0;
}
