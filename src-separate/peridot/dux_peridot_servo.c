/*
 * ECMA objects:
 *    class PeridotServo {
 *      constructor(pin) {
 *      }
 *
 *      static enableAll() {
 *      }
 *
 *      static disableAll() {
 *      }
 *
 *      get rawValue()      { return <uint>; }
 *      set rawValue(<uint>){}
 *
 *      get pin()           { return <uint>; }
 *    }
 *    global.Peridot.Servo = PeridotServo;
 */
#if defined(DUX_USE_BOARD_PERIDOT)
#if !defined(DUX_OPT_NO_HARDWARE_MODULES) && !defined(DUX_OPT_NO_SERVO)
#include "../dux_internal.h"
#include <system.h>
#include <peridot_servo.h>

/*
 * Constants
 */
DUK_LOCAL const char DUX_IPK_PERIDOT_SERVO_PIN[] = DUX_IPK("bSrvPin");
DUK_LOCAL const char DUX_IPK_PERIDOT_SERVO_CFG[] = DUX_IPK("bSrvCfg");

enum
{
	SERVO_CFG_FLOATMODE = 0,
	SERVO_CFG_MINVALUE,
	SERVO_CFG_MAXVALUE,
	SERVO_CFG_MINRAWVALUE,
	SERVO_CFG_MAXRAWVALUE,
	SERVO_CFG_SATURATED,
	SERVO_CFG_VALUE,
};

/*
 * Entry of PeridotServo.enableAll()
 */
DUK_LOCAL duk_ret_t servo_enableAll(duk_context *ctx)
{
	/* [  ] */
	peridot_servo_enable_all();

	return 0; /* return undefined; */
}

/*
 * Entry of PeridotServo.disableAll()
 */
DUK_LOCAL duk_ret_t servo_disableAll(duk_context *ctx)
{
	/* [  ] */
	peridot_servo_disable_all();

	return 0; /* return undefined; */
}

#if 0
/*
 * Getter of PeridotServo.prototype.floatMode
 */
DUK_LOCAL duk_ret_t servo_proto_floatMode_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_FLOATMODE);
	return 1; /* return <bool>; */
}

/*
 * Setter of PeridotServo.prototype.floatMode
 */
DUK_LOCAL duk_ret_t servo_proto_floatMode_setter(duk_context *ctx)
{
	/* [ bool ] */
	duk_push_this(ctx);
	/* [ bool this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ bool this arr ] */
	duk_push_boolean(ctx, duk_require_boolean(ctx, 0));
	duk_get_prop_index(ctx, 2, SERVO_CFG_FLOATMODE);
	return 0; /* return undefined; */
}

/*
 * Getter of PeridotServo.prototype.maxRawValue
 */
DUK_LOCAL duk_ret_t servo_proto_maxRawValue_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_MAXRAWVALUE);
	return 1; /* return <int>; */
}

/*
 * Setter of PeridotServo.prototype.maxRawValue
 */
DUK_LOCAL duk_ret_t servo_proto_maxRawValue_setter(duk_context *ctx)
{
	/* [ int ] */
	duk_push_this(ctx);
	/* [ int this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ int this arr ] */
	duk_push_int(ctx, duk_require_int(ctx, 0));
	duk_get_prop_index(ctx, 2, SERVO_CFG_MAXRAWVALUE);
	return 0; /* return undefined; */
}

/*
 * Getter of PeridotServo.prototype.minRawValue
 */
DUK_LOCAL duk_ret_t servo_proto_minRawValue_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_MINRAWVALUE);
	return 1; /* return <int>; */
}

/*
 * Setter of PeridotServo.prototype.minRawValue
 */
DUK_LOCAL duk_ret_t servo_proto_minRawValue_setter(duk_context *ctx)
{
	/* [ int ] */
	duk_push_this(ctx);
	/* [ int this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ int this arr ] */
	duk_push_int(ctx, duk_require_int(ctx, 0));
	duk_get_prop_index(ctx, 2, SERVO_CFG_MINRAWVALUE);
	return 0; /* return undefined; */
}

/*
 * Getter of PeridotServo.prototype.maxValue
 */
DUK_LOCAL duk_ret_t servo_proto_maxValue_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_MAXVALUE);
	return 1; /* return <number>; */
}

/*
 * Setter of PeridotServo.prototype.maxValue
 */
DUK_LOCAL duk_ret_t servo_proto_maxValue_setter(duk_context *ctx)
{
	/* [ number ] */
	duk_push_this(ctx);
	/* [ number this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ number this arr ] */
	duk_push_number(ctx, duk_require_number(ctx, 0));
	duk_get_prop_index(ctx, 2, SERVO_CFG_MAXVALUE);
	return 0; /* return undefined; */
}

/*
 * Getter of PeridotServo.prototype.minValue
 */
DUK_LOCAL duk_ret_t servo_proto_minValue_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_MINVALUE);
	return 1; /* return <number>; */
}

/*
 * Setter of PeridotServo.prototype.minValue
 */
DUK_LOCAL duk_ret_t servo_proto_minValue_setter(duk_context *ctx)
{
	/* [ number ] */
	duk_push_this(ctx);
	/* [ number this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ number this arr ] */
	duk_push_number(ctx, duk_require_number(ctx, 0));
	duk_get_prop_index(ctx, 2, SERVO_CFG_MINVALUE);
	return 0; /* return undefined; */
}
#endif

/*
 * Getter of PeridotServo.prototype.pin
 */
DUK_LOCAL duk_ret_t servo_proto_pin_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUX_IPK_PERIDOT_SERVO_PIN))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this int ] */
	return 1; /* return <int>; */
}

/*
 * Getter of PeridotServo.prototype.rawValue
 */
DUK_LOCAL duk_ret_t servo_proto_rawValue_getter(duk_context *ctx)
{
	duk_int_t pin;
	duk_int_t value;

	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUX_IPK_PERIDOT_SERVO_PIN))
	{
		return DUK_RET_TYPE_ERROR;
	}
	/* [ this int ] */
	pin = duk_require_int(ctx, 1);
	value = peridot_servo_get_value(pin);
	if (value < 0)
	{
		return DUK_RET_API_ERROR;
	}
	duk_push_int(ctx, value);
	return 1; /* return <int>; */
}

/*
 * Setter of PeridotServo.prototype.rawValue
 */
DUK_LOCAL duk_ret_t servo_proto_rawValue_setter(duk_context *ctx)
{
	duk_int_t pin;
	duk_int_t value;

	/* [ int(value) ] */
	duk_push_this(ctx);
	/* [ int(value) this ] */
	if (!duk_get_prop_string(ctx, 1, DUX_IPK_PERIDOT_SERVO_PIN))
	{
		return DUK_RET_TYPE_ERROR;
	}
	/* [ int(value) this int ] */
	pin = duk_require_int(ctx, 2);
	value = duk_require_int(ctx, 0);
	if (value < 0)
	{
		value = 0;
	}
	else if (value > 255)
	{
		value = 255;
	}
	if (peridot_servo_set_value(pin, value) < 0)
	{
		return DUK_RET_API_ERROR;
	}
	return 0; /* return undefined; */
}

#if 0
/*
 * Getter of PeridotServo.prototype.saturated
 */
DUK_LOCAL duk_ret_t servo_proto_saturated_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_SATURATED);
	return 1; /* return <bool>; */
}

/*
 * Getter of PeridotServo.prototype.value
 */
DUK_LOCAL duk_ret_t servo_proto_value_getter(duk_context *ctx)
{
	/* [  ] */
	duk_push_this(ctx);
	/* [ this ] */
	if (!duk_get_prop_string(ctx, 0, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ this arr ] */
	duk_get_prop_index(ctx, 1, SERVO_CFG_VALUE);
	return 1; /* return <number>; */
}

/*
 * Setter of PeridotServo.prototype.value
 */
DUK_LOCAL duk_ret_t servo_proto_value_setter(duk_context *ctx)
{
	duk_int_t maxR, minR, newR;

	/* [ number ] */
	duk_push_this(ctx);
	/* [ number this ] */
	if (!duk_get_prop_string(ctx, 1, DUK_IPK_PERIDOT_SERVO_CFG))
	{
		return DUK_ERR_TYPE_ERROR;
	}
	/* [ number this arr ] */
	duk_get_prop_index(ctx, 2, SERVO_CFG_FLOATMODE);
	duk_get_prop_index(ctx, 2, SERVO_CFG_MAXVALUE);
	duk_get_prop_index(ctx, 2, SERVO_CFG_MINVALUE);
	duk_get_prop_index(ctx, 2, SERVO_CFG_MAXRAWVALUE);
	duk_get_prop_index(ctx, 2, SERVO_CFG_MINRAWVALUE);
	/* [ number this arr bool:3 number:4 number:5 int:6 int:7 ] */
	maxR = duk_require_int(ctx, 6);
	minR = duk_require_int(ctx, 7);

#define CALC(type, getter) \
	type curV, maxV, minV; \
	curV = getter(ctx, 0); \
	maxV = getter(ctx, 4); \
	minV = getter(ctx, 5); \
	if (curV < minV) { \
		curV = minV; \
	} else if (curV > maxV) { \
		curV = maxV; \
	} \


	if (duk_require_boolean(ctx, 3))
	{
		CALC(double, duk_require_number);
	}
	else
	{
		CALC(duk_int_t, duk_require_int);
	}
	return 0; /* return undefined; */
}
#endif

/*
 * Entry of PeridotServo constructor
 */
DUK_LOCAL duk_ret_t servo_constructor(duk_context *ctx)
{
	duk_int_t pin;
	int result;

	/* [ obj(pin) ] */
	if (!duk_is_constructor_call(ctx))
	{
		return DUK_ERR_TYPE_ERROR;
	}

	pin = dux_get_peridot_pin(ctx, 0);
	if (pin < 0)
	{
		return pin;
	}

	result = peridot_servo_configure_pwm(pin, 0);
	if (result == -ENOTSUP)
	{
		return DUK_ERR_UNSUPPORTED_ERROR;
	}
	else if (result < 0)
	{
		return DUK_ERR_API_ERROR;
	}

	duk_push_this(ctx);
	duk_push_int(ctx, pin);
	/* [ obj(pin) this int ] */
	duk_put_prop_string(ctx, 1, DUX_IPK_PERIDOT_SERVO_PIN);
	/* [ obj(pin) this ] */

	return 0; /* return this; */
}

/*
 * List of class methods
 */
DUK_LOCAL const duk_function_list_entry servo_funcs[] = {
	{ "enableAll", servo_enableAll, 0 },
	{ "disableAll", servo_disableAll, 0 },
	{ NULL, NULL, 0 }
};

/*
 * List of prototype properties
 */
DUK_LOCAL const dux_property_list_entry servo_proto_props[] = {
//	{ "floatMode", servo_proto_floatMode_getter, servo_proto_floatMode_setter },
//	{ "maxRawValue", servo_proto_maxRawValue_getter, servo_proto_maxRawValue_setter },
//	{ "minRawValue", servo_proto_minRawValue_getter, servo_proto_minRawValue_setter },
//	{ "maxValue", servo_proto_maxValue_getter, servo_proto_maxValue_setter },
//	{ "minValue", servo_proto_minValue_getter, servo_proto_minValue_setter },
	{ "pin", servo_proto_pin_getter, NULL },
	{ "rawValue", servo_proto_rawValue_getter, servo_proto_rawValue_setter },
//	{ "saturated", servo_proto_saturated_getter, NULL },
//	{ "value", servo_proto_value_getter, servo_proto_value_setter },
	{ NULL, NULL, NULL }
};

/*
 * Initialize PeridotServo module
 */
DUK_INTERNAL duk_errcode_t dux_peridot_servo_init(duk_context *ctx)
{
	/* [ ... obj ] */
	dux_push_named_c_constructor(
			ctx, "PeridotServo", servo_constructor, 1,
			servo_funcs, NULL, NULL, servo_proto_props);
	/* [ ... obj constructor ] */
	duk_put_prop_string(ctx, -2, "Servo");
	/* [ ... obj ] */
	return DUK_ERR_NONE;
}

#endif  /* !DUX_OPT_NO_HARDWARE_MODULES && !DUX_OPT_NO_SERVO */
#endif  /* DUX_USE_BOARD_PERIDOT */
