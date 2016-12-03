/*
 * ECMA objects:
 *   global.Peridot = {
 *     startLed: <ParallelIO>
 *   };
 *
 * Internal data structure:
 *   heap_stash[DUX_IPK_PERIDOT] = {
 *   };
 *
 * Native functions:
 *   duk_bool_t dux_push_peridot_stash(duk_context *ctx);
 *   duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index, const char *key);
 */
#if defined(DUX_USE_BOARD_PERIDOT)
#include "../dux_internal.h"
#include <system.h>
#include <peridot_swi_regs.h>
#include <string.h>
#include <ctype.h>

/*
 * Constants
 */

DUK_LOCAL const char DUX_IPK_PERIDOT[] = DUX_IPK("Peridot");

/*
 * Getter of Peridot.startLed (self replace)
 */
DUK_LOCAL duk_ret_t peridot_startLed_getter(duk_context *ctx)
{
	/* [ key ] */
	if (!duk_get_global_string(ctx, "ParallelIO"))
	{
		/* [ key undefined ] */
		return 0; /* return undefined */
	}
	/* [ key constructor ] */
	duk_push_uint(ctx, 1);                                      /* width */
	duk_push_uint(ctx, PERIDOT_SWI_RSTSTS_LED_OFST);            /* offset */
	duk_push_uint(ctx, 0);                                      /* polarity */
	duk_push_pointer(ctx, (void *)&dux_paraio_manip_rw);        /* manip */
	duk_push_pointer(ctx, IOADDR_PERIDOT_SWI_RSTSTS(SWI_BASE)); /* pointer */
	duk_new(ctx, 5);
	/* [ key obj ] */
	duk_push_this(ctx);
	/* [ key obj this ] */
	duk_swap(ctx, 0, 2);
	/* [ this obj key ] */
	duk_dup(ctx, 1);
	/* [ this obj key obj ] */
	duk_def_prop(ctx, 0, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE |
			DUK_DEFPROP_CLEAR_WRITABLE);
	/* [ this obj ] */
	return 1; /* return obj */
}

/*
 * Initialize Peridot object
 */
DUK_INTERNAL duk_errcode_t dux_peridot_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	duk_push_object(ctx);
	/* [ ... stash obj ] */
	duk_put_prop_string(ctx, -2, DUX_IPK_PERIDOT);
	/* [ ... stash ] */
	duk_pop(ctx);
	/* [ ... ] */
	duk_push_object(ctx);
	/* [ ... obj ] */
	duk_push_string(ctx, "startLed");
	duk_push_c_function(ctx, peridot_startLed_getter, 1 /* with key */);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_SET_ENUMERABLE);

#define INIT(submodule) \
	do { \
		duk_errcode_t result = dux_peridot_##submodule##_init(ctx); \
		if (result != DUK_ERR_NONE) \
		{ \
			duk_pop(ctx); \
			return result; \
		} \
	} while (0)

	INIT(gpio);
	INIT(i2c);

#undef INIT

	/* [ ... obj ] */
	duk_put_global_string(ctx, "Peridot");
	/* [ ... ] */
	return DUK_ERR_NONE;
}

/*
 * Push stash object for Peridot
 */
DUK_INTERNAL duk_bool_t dux_push_peridot_stash(duk_context *ctx)
{
	duk_bool_t result;

	/* [ ... ] */
	duk_push_heap_stash(ctx);
	/* [ ... stash ] */
	result = duk_get_prop_string(ctx, -1, DUX_IPK_PERIDOT);
	/* [ ... stash obj ] */
	duk_remove(ctx, -2);
	/* [ ... obj ] */
	return result;
}

/*
 * Get Peridot pin assign from object
 */
DUK_INTERNAL duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index, const char *key)
{
	duk_int_t pin;
	const char *str;

	if (!key)
	{
		duk_dup(ctx, index);
	}
	else if (!duk_get_prop_string(ctx, index, key))
	{
		duk_pop(ctx);
		char *ukey = alloca(strlen(key) + 1);
		char *dest;
		const char *src;
		for (dest = ukey, src = key;;)
		{
			if ((*dest++ = toupper(*src++)) == '\0')
			{
				break;
			}
		}

		if (!duk_get_prop_string(ctx, index, ukey))
		{
			duk_pop(ctx);
			return -1;
		}
	}

	str = duk_get_string(ctx, -1);
	if (str)
	{
		// String form
		// "D10"
		// "d10"
		// "10"
		char *end;
		if ((str[0] == 'D') || (str[0] == 'd'))
		{
			++str;
		}
		pin = strtol(str, &end, 10);
		goto check;
	}
	else if (duk_is_number(ctx, -1))
	{
		pin = (duk_int_t)duk_get_uint(ctx, -1);
		goto check;
	}

	duk_pop(ctx);
	return -1;

check:
	duk_pop(ctx);
	if ((pin < DUX_PERIDOT_PIN_MIN) || (pin > DUX_PERIDOT_PIN_MAX))
	{
		return -1;
	}
	return pin;
}

#endif  /* DUX_USE_BOARD_PERIDOT */
