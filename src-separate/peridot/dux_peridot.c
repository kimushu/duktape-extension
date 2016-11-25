#include "dux_peridot.h"
#include "system.h"
#include "peridot_swi_regs.h"
#include <string.h>

/*
 * C function entry of getter for Peridot.start_led (self replace)
 */
static duk_ret_t peridot_startled(duk_context *ctx)
{
	duk_push_this(ctx);
	/* [ this ] */
	duk_get_global_string(ctx, "ParallelIO");
	duk_push_uint(ctx, 1);									/* width */
	duk_push_uint(ctx, PERIDOT_SWI_RSTSTS_LED_OFST);		/* offset */
	duk_push_uint(ctx, IOADDR_PERIDOT_SWI_RSTSTS(SWI_BASE));/* val_ptr */
	duk_push_uint(ctx, 0);									/* dir_ptr */
	duk_push_uint(ctx, PERIDOT_SWI_RSTSTS_LED_MSK);			/* dir_val */
	duk_push_uint(ctx, 0);									/* pol_val */
	duk_new(ctx, 6);
	/* [ this obj(pio) ] */
	duk_push_string(ctx, "start_led");
	duk_dup(ctx, -2);
	/* [ this obj(pio) "start_led" obj(pio) ] */
	duk_def_prop(ctx, -4, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE);
	/* [ this obj(pio) ] */
	return 1;	/* return obj; */
}

/*
 * Initialize Peridot object
 */
void dux_peridot_init(duk_context *ctx)
{
	/* [ ... ] */
	duk_push_object(ctx);
	/* [ ... obj ] */
	duk_push_string(ctx, "start_led");
	duk_push_c_function(ctx, peridot_startled, 0);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_SET_ENUMERABLE);
	/* [ ... obj ] */
	duk_put_global_string(ctx, "Peridot");
	/* [ ... ] */
}

/*
 * Get Peridot pin assign from object
 */
duk_int_t dux_get_peridot_pin(duk_context *ctx, duk_idx_t index, const char *key)
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
		char *dest, *src;
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
		const char *end;
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

