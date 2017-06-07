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
#include <stdarg.h>

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
	duk_get_global_string(ctx, "require");
	duk_push_string(ctx, "hardware");
	duk_call(ctx, 1);
	/* [ key hardware ] */
	if (!duk_get_prop_string(ctx, 1, "ParallelIO")) {
	{
		/* [ key hardware undefined ] */
		return 0; /* return undefined */
	}
	duk_remove(ctx, 1);
	/* [ key constructor ] */
	duk_push_uint(ctx, PERIDOT_SWI_RSTSTS_LED_OFST);            /* offset */
	duk_push_uint(ctx, PERIDOT_SWI_RSTSTS_LED_WIDTH);           /* width */
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
	duk_def_prop(ctx, 0, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_FORCE);
	/* [ this obj ] */
	return 1; /* return obj */
}

/*
 * Entry of Peridot module
 */
DUK_LOCAL duk_errcode_t peridot_entry(duk_context *ctx)
{
	/* [ require module exports ] */
	duk_push_string(ctx, "startLed");
	duk_push_c_function(ctx, peridot_startLed_getter, 1 /* with key */);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_SET_ENUMERABLE);

	return dux_invoke_initializers(ctx,
		DUX_INIT_PERIDOT_GPIO
		DUX_INIT_PERIDOT_I2C
		DUX_INIT_PERIDOT_SPI
		DUX_INIT_PERIDOT_SERVO
		NULL
	);
}

/*
 * Initialize Peridot object
 */
DUK_INTERNAL duk_errcode_t dux_peridot_init(duk_context *ctx)
{
	return dux_modules_register(ctx, "peridot", peridot_entry);
}

/*
 * Get Peridot pin assign from value
 */
DUK_INTERNAL duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index)
{
	const char *str;
	duk_int_t pin;

	str = duk_get_string(ctx, index);
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
		if (*end != '\0')
		{
			return DUK_RET_TYPE_ERROR;
		}
	}
	else if (duk_is_number(ctx, index))
	{
		pin = duk_get_int(ctx, index);
	}
	else
	{
		return DUK_RET_TYPE_ERROR;
	}

	if ((pin < DUX_PERIDOT_PIN_MIN) || (pin > DUX_PERIDOT_PIN_MAX))
	{
		return DUK_RET_RANGE_ERROR;
	}

	return pin;
}

/*
 * Copy string with conversion to upper case
 */
DUK_LOCAL int strcpy_toupper(char *dest, const char *src)
{
	int converted = 0;
	char ch, cch;
	for (; (ch = *src++) != '\0';)
	{
		cch = toupper((int)ch);
		if (cch != ch)
		{
			++converted;
		}
		*dest++ = cch;
	}
	*dest = '\0';
	return converted;
}

/*
 * Get Peridot pin assign from object
 */
DUK_INTERNAL duk_int_t dux_get_peridot_pin_by_key(duk_context *ctx, duk_idx_t index, ...)
{
	duk_int_t pin;
	const char *key;
	va_list keys;

	pin = DUK_RET_ERROR;
	key = NULL;

	va_start(keys, index);
	for (;;)
	{
		if (key)
		{
			const char *lkey = key;
			char *ukey = alloca(strlen(lkey) + 1);
			key = NULL;
			if (!strcpy_toupper(ukey, lkey))
			{
				continue;
			}
			if (!duk_get_prop_string(ctx, index, ukey))
			{
				duk_pop(ctx);
				continue;
			}
		}
		else
		{
			key = va_arg(keys, const char *);
			if (!key)
			{
				break;
			}
			if (!duk_get_prop_string(ctx, index, key))
			{
				duk_pop(ctx);
				continue;
			}
		}

		if (pin >= 0)
		{
			duk_pop(ctx);
			pin = DUK_RET_TYPE_ERROR;
			break;
		}

		pin = dux_get_peridot_pin(ctx, -1);
		duk_pop(ctx);
		if (pin < 0)
		{
			break;
		}
	}
	va_end(keys);

	return pin;
}

#endif  /* DUX_USE_BOARD_PERIDOT */
