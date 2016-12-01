/*
 * ECMA objects:
 *    class PeridotGPIO extends ParallelIO {
 *      constructor(<uint> begin, <uint> width = 1) {
 *      }
 *    }
 *    global.Peridot.GPIO = PeridotGPIO;
 */
#if defined(DUX_USE_BOARD_PERIDOT)
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_GPIO)
#include "../dux_internal.h"
#include <system.h>
#include <peridot_pfc_interface.h>
#include <peridot_pfc_interface_regs.h>

/*
 * Manipulator functions for ParallelIO (multi bank)
 */

DUK_LOCAL duk_ret_t gpio_manip_multi_read_input(duk_context *ctx, void *param,
                                                duk_uint_t bits, duk_uint_t *result)
{
	duk_uint_t value = 0;
	alt_u32 mask = (1 << PERIDOT_PFC_BANK_WIDTH) - 1;
	alt_u32 bank;

	for (bank = 0; bank < PERIDOT_PFC_BANK_COUNT; ++bank)
	{
		if (bits & mask)
		{
			value |= (peridot_pfc_interface_direct_input_bank(bank) <<
				(bank * PERIDOT_PFC_BANK_WIDTH)) & mask;
		}
		mask <<= PERIDOT_PFC_BANK_WIDTH;
	}
	*result = value;
	return 0;
}

DUK_LOCAL duk_ret_t gpio_manip_multi_write_output(duk_context *ctx, void *param,
                                                  duk_uint_t set, duk_uint_t clear,
                                                  duk_uint_t toggle)
{
	alt_u32 bank;
	for (bank = 0; bank < PERIDOT_PFC_BANK_COUNT; ++bank)
	{
		if ((set | clear | toggle) & ((1 << PERIDOT_PFC_BANK_WIDTH) - 1))
		{
			peridot_pfc_interface_direct_output_bank(bank, set, clear, toggle);
		}
		set >>= PERIDOT_PFC_BANK_WIDTH;
		clear >>= PERIDOT_PFC_BANK_WIDTH;
		toggle >>= PERIDOT_PFC_BANK_COUNT;
	}
	return 0;
}

DUK_LOCAL duk_ret_t gpio_manip_multi_config_input(duk_context *ctx, void *param,
                                                  duk_uint_t bits, duk_uint_t enabled)
{
	return 0;
}

DUK_LOCAL duk_ret_t gpio_manip_multi_config_output(duk_context *ctx, void *param,
                                                   duk_uint_t bits, duk_uint_t enabled)
{
	alt_u32 bank;
	alt_u32 disabled = ~enabled & bits;
	enabled &= bits;

	for (bank = 0; bank < PERIDOT_PFC_BANK_COUNT; ++bank)
	{
		if (disabled & ((1 << PERIDOT_PFC_BANK_WIDTH) - 1))
		{
			peridot_pfc_interface_select_output_bank(bank, disabled,
					PERIDOT_PFC_OUTPUT_PINX_HIZ);
		}
		if (enabled & ((1 << PERIDOT_PFC_BANK_WIDTH) - 1))
		{
			peridot_pfc_interface_select_output_bank(bank, enabled,
					PERIDOT_PFC_OUTPUT_PINX_DOUT);
		}
		enabled >>= PERIDOT_PFC_BANK_WIDTH;
		disabled >>= PERIDOT_PFC_BANK_WIDTH;
	}
	return 0;
}

DUK_LOCAL duk_ret_t gpio_manip_multi_read_config(duk_context *ctx, void *param,
                                                 duk_uint_t bits, duk_uint_t *input, duk_uint_t *output)
{
	alt_u32 pin;
	alt_u32 val_out = 0;

	*input = bits;
	for (pin = 0; bits != 0; ++pin, bits >>= 1)
	{
		if (bits & 1)
		{
			if (peridot_pfc_interface_get_output_selection(pin) !=
					PERIDOT_PFC_OUTPUT_PINX_HIZ)
			{
				val_out |= (1 << pin);
			}
		}
	}
	*output = val_out;
	return 0;
}

DUK_LOCAL duk_ret_t gpio_manip_multi_slice(duk_context *ctx, void *param,
                                           duk_uint_t bits,
                                           dux_paraio_manip const **new_manip,
                                           void **new_param)
{
	return 0;
}

/*
 * Manipulator for ParallelIO
 */

DUK_LOCAL const dux_paraio_manip gpio_manip_multi =
{
	.read_input    = gpio_manip_multi_read_input,
	.write_output  = gpio_manip_multi_write_output,
	.config_input  = gpio_manip_multi_config_input,
	.config_output = gpio_manip_multi_config_output,
	.read_config   = gpio_manip_multi_read_config,
	.slice         = gpio_manip_multi_slice,
};

/*
 * Entry of PeridotGPIO constructor
 */
DUK_LOCAL duk_ret_t gpio_constructor(duk_context *ctx)
{
	duk_uint_t begin;
	duk_uint_t width;

	/* [ uint uint/undefined ] */
	begin = duk_require_uint(ctx, 0);

	if (duk_is_null_or_undefined(ctx, 1))
	{
		width = 1;
	}
	else
	{
		width = duk_require_uint(ctx, 1);
	}

	if ((width == 0) ||
		(begin < DUX_PERIDOT_PIN_MIN) ||
		((begin + width - 1) > DUX_PERIDOT_PIN_MAX))
	{
		return DUK_RET_RANGE_ERROR;
	}

	duk_set_top(ctx, 0);
	duk_get_global_string(ctx, "ParallelIO");
	/* [ super ] */
	duk_push_this(ctx);
	/* [ super this ] */
	duk_push_uint(ctx, width);
	duk_push_uint(ctx, begin);
	duk_push_pointer(ctx, (void *)&gpio_manip_multi);
	duk_push_pointer(ctx, NULL);
	/* [ super this uint uint pointer pointer ] */
	duk_call_method(ctx, 4);
	duk_push_this(ctx);
	duk_get_prototype(ctx, -1);
	duk_dump_context_stderr(ctx);
	return 0; /* return this */
}

/*
 * Initialize PeridotGPIO submodule
 */
DUK_INTERNAL duk_errcode_t dux_peridot_gpio_init(duk_context *ctx)
{
	/* [ ... obj ] */
	duk_get_global_string(ctx, "ParallelIO");
	/* [ ... obj super ] */
	dux_push_inherited_named_c_constructor(
			ctx, -1, "PeridotGPIO", gpio_constructor, 2,
			NULL, NULL, NULL, NULL);
	/* [ ... obj super constructor ] */
	duk_put_prop_string(ctx, -3, "GPIO");
	/* [ ... obj super ] */
	duk_pop(ctx);
	/* [ ... obj ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_GPIO */
#endif  /* DUX_USE_BOARD_PERIDOT */
