#if !defined(DUX_OPT_NO_NODEJS_MODULES) && !defined(DUX_OPT_NO_EVENTS)
#include "../dux_internal.h"

DUK_LOCAL const char DUX_KEY_EVENTS[] = "_events";
DUK_LOCAL const char DUX_KEY_EVENTSCOUNT[] = "_eventsCount";
DUK_LOCAL const char DUX_KEY_MAXLISTENERS[] = "_maxListeners";
DUK_LOCAL const char DUX_KEY_LISTENER[] = "listener";
DUK_LOCAL const char DUX_IPK_DEFMAXLISTENERS[] = "evDefMax";

/**
 * Get listener function
 */
DUK_LOCAL void events_get_function(duk_context *ctx, duk_idx_t idx, duk_bool_t *is_once)
{
    duk_bool_t once = 0;
    /* [ ... listener ... ] */
    if (!duk_is_callable(ctx, idx)) {
        once = 1;
        idx = duk_normalize_index(ctx, idx);
        duk_get_prop_string(ctx, idx, DUX_KEY_LISTENER);
        /* [ ... func_wrapper ... func ] */
        duk_replace(ctx, idx);
    }
    /* [ ... func ... ] */
    if (is_once) {
        *is_once = once;
    }
}

/**
 * Update events count
 */
DUK_LOCAL void events_update_count(duk_context *ctx, duk_idx_t this_idx, duk_int_t diff)
{
    /* [ ... this ... ] */
    this_idx = duk_normalize_index(ctx, this_idx);
    duk_get_prop_string(ctx, this_idx, DUX_KEY_EVENTSCOUNT);
    duk_push_uint(ctx, duk_require_uint(ctx, -1) + diff);
    duk_put_prop_string(ctx, this_idx, DUX_KEY_EVENTSCOUNT);
    duk_pop(ctx);
}

/**
 * Constructor of EventEmitter class
 */
DUK_LOCAL duk_ret_t events_constructor(duk_context *ctx)
{
    /* [  ] */
    duk_push_this(ctx);
    /* [ this ] */
    duk_push_object(ctx);
    duk_put_prop_string(ctx, 0, DUX_KEY_EVENTS);
    duk_push_uint(ctx, 0);
    duk_put_prop_string(ctx, 0, DUX_KEY_EVENTSCOUNT);
    duk_push_undefined(ctx);
    duk_put_prop_string(ctx, 0, DUX_KEY_MAXLISTENERS);
    return 0;
}

/**
 * Common implementation of on/once/prepend(Once)Listener
 */
DUK_LOCAL duk_ret_t events_proto_common_add(duk_context *ctx, int once, int prepend)
{
    /* [ key func ] */
    duk_require_callable(ctx, 1);

    if (once) {
        /* Convert function to wrapped object if "once" event */
        duk_push_object(ctx);
        /* [ key func func_wrapper ] */
        duk_swap(ctx, 1, 2);
        /* [ key func_wrapper func ] */
        duk_put_prop_string(ctx, 1, DUX_KEY_LISTENER);
        /* [ key func_wrapper ] */
    }

    duk_push_this(ctx);
    /* [ key listener this ] */
    if (!duk_get_prop_string(ctx, 2, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key listener this obj ] */
    duk_dup(ctx, 0);
    if (!duk_get_prop(ctx, 3)) {
        /* New event */

        /* [ key listener this obj undefined:4 ] */
        duk_dup(ctx, 0);
        duk_dup(ctx, 1);
        duk_put_prop(ctx, 3);
        /* [ key listener this obj undefined:4 ] */
        events_update_count(ctx, 2, 1);
        duk_pop_2(ctx);
        /* [ key listener this ] */
        /* No maxListeners check */
        return 1;
    }

    duk_size_t length;
    if (!duk_is_array(ctx, 4)) {
        /* An existing event with one listener */

        /* [ key listener this obj listener:4 ] */
        duk_push_array(ctx);
        duk_dup(ctx, 0);
        /* [ key listener this obj listener:4 arr_new:5 key:6 ] */
        duk_swap(ctx, 4, 6);
        /* [ key listener this obj key:4 arr_new:5 listener:6 ] */
        duk_put_prop_index(ctx, 5, prepend ? 1 : 0);
        /* [ key listener this obj key:4 new_arr:5 ] */
        duk_dup(ctx, 1);
        duk_put_prop_index(ctx, 5, prepend ? 0 : 1);
        /* [ key listener this obj key:4 arr_new:5 ] */
        duk_put_prop(ctx, 3);
        /* [ key listener this obj ] */
        length = 2;
    } else {
        /* An existing event with two or more listeners */
        duk_uarridx_t src_index, dest_index = 0;

        /* [ key listener this obj arr_src:4 ] */
        duk_push_array(ctx);
        duk_dup(ctx, 0);
        /* [ key listener this obj arr_src:4 arr_dest:5 key:6 ] */
        duk_swap(ctx, 4, 6);
        /* [ key listener this obj key:4 arr_dest:5 arr_src:6 ] */
        if (prepend) {
            duk_dup(ctx, 1);
            duk_put_prop_index(ctx, 6, dest_index++);
        }
        length = duk_get_length(ctx, 6);
        for (src_index = 0; src_index < length; ++src_index) {
            duk_get_prop_index(ctx, 6, src_index);
            duk_put_prop_index(ctx, 5, dest_index++);
        }
        if (!prepend) {
            duk_dup(ctx, 1);
            duk_put_prop_index(ctx, 5, dest_index++);
        }
        duk_pop(ctx);
        /* [ key listener this obj key:4 arr_dest:5 ] */
        duk_put_prop(ctx, 3);
        /* [ key listener this obj ] */
        ++length;
    }
    duk_pop(ctx);
    /* [ key listener this ] */

    duk_uint_t max_listeners;
    /* [ key listener this ] */
    duk_get_prop_string(ctx, 2, DUX_KEY_MAXLISTENERS);
    if (duk_is_undefined(ctx, 3)) {
        /* [ key listener this undefined ] */
        duk_get_prop_string(ctx, 2, "constructor");
        /* [ key listener this undefined constructor:4 ] */
        duk_get_prop_string(ctx, 4, DUX_IPK_DEFMAXLISTENERS);
        /* [ key listener this undefined constructor:4 uint:5 ] */
        max_listeners = duk_get_uint(ctx, 5);
        duk_pop_3(ctx);
        /* [ key listener this ] */
    } else {
        /* [ key listener this uint ] */
        max_listeners = duk_get_uint(ctx, 3);
        duk_pop(ctx);
        /* [ key listener this ] */
    }
    if (length > max_listeners) {
        duk_push_sprintf(ctx,
            "Possible EventEmitter memory leak detected (%d '%s' listeners)",
            length, duk_safe_to_string(ctx, 0)
        );
        /* [ key listener this str ] */
        dux_report_warning(ctx);
        duk_pop(ctx);
    }
    return 1;
}

/**
 * Entry of emit()
 */
DUK_LOCAL duk_ret_t events_proto_emit(duk_context *ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);
    duk_bool_t emitted = 0;

    /* [ key ... ] */
    if (nargs == 0) {
        goto finish;
    }

    duk_push_this(ctx);
    /* [ key ... this ] */
    if (!duk_get_prop_string(ctx, -1, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key ... this obj ] */
    duk_dup(ctx, 0);
    if (!duk_get_prop(ctx, -2)) {
        /* No listener */
        /* [ key ... this obj undefined ] */
        goto finish;
    }

    if (!duk_is_array(ctx, -1)) {
        /* Single listener */
        /* [ key ... this obj listener ] */
        if (!duk_is_callable(ctx, -1)) {
            /* Once listener */
            duk_dup(ctx, 0);
            /* [ key ... this obj func_wrapper key ] */
            duk_del_prop(ctx, -3);
            /* [ key ... this obj func_wrapper ] */
            duk_get_prop_string(ctx, -1, DUX_KEY_LISTENER);
            /* [ key ... this obj func_wrapper func ] */
        }
        /* [ key ... this ... func ] */
        duk_swap(ctx, 0, -1);
        /* [ func ... this ... key ] */
        duk_set_top(ctx, nargs + 1);
        /* [ func ... this ] */
        duk_insert(ctx, 1);
        /* [ func this ... ] */
        emitted = 1;
        if (duk_pcall_method(ctx, nargs - 1) != 0) {
            /* [ err ] */
            /* FIXME */
        }
        /* [ retval/err ] */
        goto finish;
    }

    /* Multiple listeners */
    /* [ key ... this obj arr ] */
    duk_size_t once_listeners;
    duk_size_t length;
    duk_uarridx_t src_index;
    duk_uarridx_t dest_index;

    once_listeners = 0;
    length = duk_get_length(ctx, -1);
    for (src_index = 0; src_index < length; ++src_index) {
        duk_idx_t arg_index;

        duk_get_prop_index(ctx, -1, src_index);
        /* [ key ... this obj arr listener ] */
        if (!duk_is_callable(ctx, -1)) {
            /* Once listener */
            if (!duk_get_prop_string(ctx, -1, DUX_KEY_LISTENER)) {
                /* Already flag'd (listener is emitting event recursively!) */
                /* [ key ... this obj arr func_wrapper undefined ] */
                duk_pop_2(ctx);
                /* [ key ... this obj arr ] */
                continue;
            }
            ++once_listeners;
            duk_del_prop_string(ctx, -2, DUX_KEY_LISTENER); /* Flag for delete */
            duk_replace(ctx, -2);
        }
        /* [ key ... this obj arr func ] */
        duk_push_this(ctx);
        for (arg_index = 1; arg_index < nargs; ++arg_index) {
            duk_dup(ctx, arg_index);
        }
        /* [ key ... this obj arr func this ... ] */
        emitted = 1;
        if (duk_pcall_method(ctx, nargs - 1) != 0) {
            /* [ key ... this obj arr err ] */
            /* FIXME */
            duk_pop(ctx);
            break;
        }
        duk_pop(ctx);
    }

    if (once_listeners == 0) {
        goto finish;
    }

    /* Clear finished once listeners */
    duk_set_top(ctx, 1);
    duk_push_this(ctx);
    /* [ key this ] */
    /* Re-get event obj */
    if (!duk_get_prop_string(ctx, 1, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key this obj ] */
    duk_dup(ctx, 0);
    if (!duk_get_prop(ctx, 2)) {
        /* No event (All listeners maybe removed in callbacks) */
        goto finish;
    }

    /* [ key this obj listener/arr ] */
    if (!duk_is_array(ctx, 3)) {
        /* Single listener */
        goto finish;
    }

    /* [ key this obj arr ] */
    length = duk_get_length(ctx, 3);
    for (src_index = dest_index = 0; src_index < length; ++src_index) {
        duk_get_prop_index(ctx, 3, src_index);
        /* [ key this obj arr listener:4 ] */
        if (!duk_is_callable(ctx, 4)) {
            /* [ key this obj arr func_wrapper:4 ] */
            if (!duk_has_prop_string(ctx, 4, DUX_KEY_LISTENER)) {
                /* Remove finished once listener */
                duk_pop(ctx);
                /* [ key this obj arr ] */
                continue;
            }
        }
        if (dest_index < src_index) {
            duk_put_prop_index(ctx, 3, dest_index++);
        } else {
            duk_pop(ctx);
        }
        /* [ key this obj arr ] */
    }

    if (dest_index == 0) {
        /* No listener remaining */
        duk_swap(ctx, 0, 3);
        /* [ arr this obj key ] */
        duk_del_prop(ctx, 2);
        /* [ arr this obj ] */
        events_update_count(ctx, 1, -1);
    } else if (dest_index == 1) {
        /* Only one listener remaining */
        duk_get_prop_index(ctx, 3, 0);
        /* [ key this obj arr listener:4 ] */
        duk_swap(ctx, 0, 3);
        /* [ arr this obj key listener:4 ] */
        duk_put_prop(ctx, 2);
        /* [ arr this obj ] */
    } else if (dest_index < src_index) {
        /* Some once listeners has been removed */
        duk_set_length(ctx, 3, dest_index);
    }

finish:
    /* [ ... ] */
    duk_push_boolean(ctx, emitted);
    return 1;
}

/**
 * Entry of eventNames()
 */
DUK_LOCAL duk_ret_t events_proto_eventNames(duk_context *ctx)
{
    duk_uarridx_t index;

    /* [  ] */
    duk_push_this(ctx);
    /* [ this ] */
    if (!duk_get_prop_string(ctx, 0, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ this obj ] */
    duk_push_array(ctx);
    /* [ this obj arr ] */
    duk_enum(ctx, 1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    /* [ this obj arr enum ] */
    index = 0;
    while (duk_next(ctx, 3, 0)) {
        /* [ this obj arr enum key ] */
        duk_put_prop_index(ctx, 2, index++);
        /* [ this obj arr enum ] */
    }
    duk_pop(ctx);
    /* [ this obj arr ] */
    return 1;
}

/**
 * Entry of getMaxListeners()
 */
DUK_LOCAL duk_ret_t events_proto_getMaxListeners(duk_context *ctx)
{
    /* [  ] */
    duk_push_this(ctx);
    /* [ this ] */
    if (!duk_get_prop_string(ctx, 0, DUX_KEY_MAXLISTENERS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ this number/undefined ] */
    return 1;
}

/**
 * Entry of listenerCount()
 */
DUK_LOCAL duk_ret_t events_proto_listenerCount(duk_context *ctx)
{
    /* [ key ] */
    duk_push_this(ctx);
    /* [ key this ] */
    if (!duk_get_prop_string(ctx, 1, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key this obj ] */
    duk_swap(ctx, 0, 2);
    /* [ obj this key ] */
    if (!duk_get_prop(ctx, 0)) {
        /* [ obj this undefined ] */
        duk_push_uint(ctx, 0);
        /* [ obj this undefined uint ] */
    } else if(!duk_is_array(ctx, 2)) {
        /* [ obj this listener ] */
        duk_push_uint(ctx, 1);
        /* [ obj this listener uint ] */
    } else {
        /* [ obj this arr ] */
        duk_push_uint(ctx, duk_get_length(ctx, 2));
        /* [ obj this arr uint ] */
    }
    /* [ ... uint ] */
    return 1;
}

/**
 * Entry of listeners()
 */
DUK_LOCAL duk_ret_t events_proto_listeners(duk_context *ctx)
{
    duk_uarridx_t index;
    duk_size_t length;

    /* [ key ] */
    duk_push_this(ctx);
    /* [ key this ] */
    if (!duk_get_prop_string(ctx, 1, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key this obj ] */
    duk_swap(ctx, 0, 2);
    /* [ obj this key ] */
    if (!duk_get_prop(ctx, 0)) {
        /* No listener */

        /* [ obj this undefined ] */
        duk_push_array(ctx);
        /* [ obj this undefined arr_copy ] */
        return 1;
    }

    /* [ obj this listener/arr ] */
    if (!duk_is_array(ctx, 2)) {
        /* Single listener */

        /* [ obj this listener ] */
        duk_push_array(ctx);
        /* [ obj this listener arr_copy ] */
        duk_swap(ctx, 2, 3);
        /* [ obj this arr_copy listener ] */
        duk_put_prop_index(ctx, 2, 0);
        /* [ obj this arr_copy ] */
        return 1;
    }

    /* Multiple listeners */
    /* [ obj this arr ] */
    duk_push_array(ctx);
    /* [ obj this arr arr_copy ] */
    length = duk_get_length(ctx, 2);
    for (index = 0; index < length; ++index) {
        duk_get_prop_index(ctx, 2, index);
        /* [ obj this arr arr_copy listener:4 ] */
        if (!duk_is_callable(ctx, 4)) {
            /* [ obj this arr arr_copy func_wrapper:4 ] */
            duk_get_prop_string(ctx, 4, DUX_KEY_LISTENER);
            /* [ obj this arr arr_copy func_wrapper:4 func:5 ] */
            duk_put_prop_index(ctx, 3, index);
            /* [ obj this arr arr_copy func_wrapper:4 ] */
            duk_pop(ctx);
            /* [ obj this arr arr_copy ] */
        } else {
            /* [ obj this arr arr_copy func:4 ] */
            duk_put_prop_index(ctx, 3, index);
            /* [ obj this arr arr_copy ] */
        }
    }
    /* [ obj this arr arr_copy ] */
    return 1;
}

/**
 * Entry of on()
 */
DUK_LOCAL duk_ret_t events_proto_on(duk_context *ctx)
{
    return events_proto_common_add(ctx, 0, 0);
}

/**
 * Entry of once()
 */
DUK_LOCAL duk_ret_t events_proto_once(duk_context *ctx)
{
    return events_proto_common_add(ctx, 1, 0);
}

/**
 * Entry of prependListener()
 */
DUK_LOCAL duk_ret_t events_proto_prependListener(duk_context *ctx)
{
    return events_proto_common_add(ctx, 0, 1);
}

/**
 * Entry of prependOnceListener()
 */
DUK_LOCAL duk_ret_t events_proto_prependOnceListener(duk_context *ctx)
{
    return events_proto_common_add(ctx, 1, 1);
}

/**
 * Entry of removeAllListeners()
 */
DUK_LOCAL duk_ret_t events_proto_removeAllListeners(duk_context *ctx)
{
    /* [ undefined/key ] */
    duk_push_this(ctx);
    /* [ undefined/key this ] */
    if (!duk_get_prop_string(ctx, 1, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ undefined/key this obj ] */

    if (duk_is_undefined(ctx, 0)) {
        /* Remove all events */

        /* [ undefined this obj ] */
        duk_push_object(ctx);
        duk_put_prop_string(ctx, 1, DUX_KEY_EVENTS);
        duk_push_uint(ctx, 0);
        duk_put_prop_string(ctx, 1, DUX_KEY_EVENTSCOUNT);
        duk_pop(ctx);
        /* [ undefined this ] */
        return 1;
    }

    /* Remove listeners of specified event */
    /* [ key this obj ] */
    duk_dup(ctx, 0);
    /* [ key this obj key ] */
    if (duk_has_prop(ctx, 2)) {
        /* Remove one event */
        /* [ key this obj ] */
        duk_swap(ctx, 0, 2);
        /* [ obj this key ] */
        duk_del_prop(ctx, 0);
        /* [ obj this ] */
        events_update_count(ctx, 1, -1);
        return 1;
    }

    /* No such event */
    /* [ key this obj ] */
    duk_pop(ctx);
    /* [ key this ] */
    return 1;
}

/**
 * Entry of removeListener()
 */
DUK_LOCAL duk_ret_t events_proto_removeListener(duk_context *ctx)
{
    duk_uarridx_t index, sub_index;
    duk_size_t length;

    /* [ key func ] */
    duk_push_this(ctx);
    /* [ key func this ] */
    if (!duk_get_prop_string(ctx, 2, DUX_KEY_EVENTS)) {
        return DUK_RET_TYPE_ERROR;
    }
    /* [ key func this obj ] */
    duk_dup(ctx, 0);
    /* [ key func this obj key:4 ] */
    if (!duk_get_prop(ctx, 3)) {
        /* No such event */
        /* [ key func this obj undefined:4 ] */
        duk_pop_2(ctx);
        return 1;
    }

    if (duk_is_array(ctx, 4)) {
        /* Multiple listeners */
        /* [ key func this obj arr:4 ] */
        length = duk_get_length(ctx, 4);
        for (index = 0; index < length; ++index) {
            duk_get_prop_index(ctx, 4, index);
            /* [ key func this obj arr:4 listener:5 ] */
            if (!duk_is_callable(ctx, 5)) {
                duk_get_prop_string(ctx, 5, DUX_KEY_LISTENER);
                /* [ key func this obj arr:4 func_wrapper:5 func:6 ] */
                if (duk_strict_equals(ctx, 1, 6)) {
                    /* Remove this */
                    duk_pop_2(ctx);
                    goto remove_from_array;
                }
            } else if (duk_strict_equals(ctx, 1, 5)) {
                /* Remove this */
                duk_pop(ctx);
remove_from_array:
                /* [ key func this obj arr:4 ] */
                if (length == 2) {
                    /* Convert to non-array */
                    duk_dup(ctx, 0);
                    /* [ key func this obj arr:4 key:5 ] */
                    duk_get_prop_index(ctx, 4, 1 - index);
                    /* [ key func this obj arr:4 key:5 listener:6 ] */
                    duk_put_prop(ctx, 3);
                    /* [ key func this obj arr:4 ] */
                    break;
                }

                /* Update array */
                duk_dup(ctx, 0);
                duk_push_array(ctx);
                /* [ key func this obj arr:4 key:5 arr_new:6 ] */
                for (sub_index = 0; sub_index < index; ++sub_index) {
                    duk_get_prop_index(ctx, 4, sub_index);
                    duk_put_prop_index(ctx, 6, sub_index);
                }
                for (sub_index = index + 1; sub_index < length; ++sub_index) {
                    duk_get_prop_index(ctx, 4, sub_index);
                    duk_put_prop_index(ctx, 6, sub_index - 1);
                }
                duk_put_prop(ctx, 3);
                /* [ key func this obj arr:4 ] */
                break;
            }
            duk_pop(ctx);
            /* [ key func this obj arr:4 ] */
        }   // for (index)
        duk_pop_2(ctx);
        /* [ key func this ] */
        return 1;
    }

    /* Single listener */
    /* [ key func this obj listener:4 ] */
    if (!duk_is_callable(ctx, 4)) {
        if (duk_get_prop_string(ctx, 4, DUX_KEY_LISTENER)) {
            /* [ key func this obj func_wrapper:4 func:5 ] */
            if (duk_strict_equals(ctx, 1, 5)) {
                duk_pop(ctx);
                goto remove_single;
            }
        }
        /* [ key func this obj func_wrapper:4 func/undefined:5 ] */
        duk_pop_3(ctx);
        /* [ key func this ] */
        return 1;
    }

    /* [ key func this obj func:4 ] */
    if (duk_strict_equals(ctx, 1, 4)) {
        /* Remove this */
remove_single:
        /* [ key func this obj any:4 ] */
        duk_dup(ctx, 0);
        /* [ key func this obj any:4 key:5 ] */
        duk_del_prop(ctx, 3);
        /* [ key func this obj any:4 ] */
        events_update_count(ctx, 2, -1);
    }
    duk_pop_2(ctx);
    /* [ key func this ] */
    return 1;
}

/**
 * Entry of setMaxListeners()
 */
DUK_LOCAL duk_ret_t events_proto_setMaxListeners(duk_context *ctx)
{
    duk_int_t value;

    /* [ number ] */
    value = duk_require_int(ctx, 0);
    if (value < 0) {
        return DUK_RET_TYPE_ERROR;
    }
    duk_push_this(ctx);
    duk_swap(ctx, 0, 1);
    /* [ this number ] */
    if (!duk_has_prop_string(ctx, 0, DUX_KEY_MAXLISTENERS)) {
        return DUK_RET_TYPE_ERROR;
    }
    duk_put_prop_string(ctx, 0, DUX_KEY_MAXLISTENERS);
    /* [ this ] */
    return 1;
}

/**
 * Getter of defaultMaxListeners
 */
DUK_LOCAL duk_ret_t events_defaultMaxListeners_getter(duk_context *ctx)
{
    /* [  ] */
    duk_push_this(ctx);
    /* [ constructor ] */
    duk_get_prop_string(ctx, 0, DUX_IPK_DEFMAXLISTENERS);
    /* [ constructor uint ] */
    return 1;
}

/**
 * Setter of defaultMaxListeners
 */
DUK_LOCAL duk_ret_t events_defaultMaxListeners_setter(duk_context *ctx)
{
    /* [ uint ] */
    duk_require_uint(ctx, 0);
    duk_push_this(ctx);
    /* [ uint constructor ] */
    duk_swap(ctx, 0, 1);
    /* [ constructor uint ] */
    duk_put_prop_string(ctx, 0, DUX_IPK_DEFMAXLISTENERS);
    /* [ constructor ] */
    return 0;
}

/**
 * List of methods for ProceEventEmitterss object
 */
DUK_LOCAL duk_function_list_entry events_proto_funcs[] = {
    { "addListener", events_proto_on, 2 },
    { "emit", events_proto_emit, DUK_VARARGS },
    { "eventNames", events_proto_eventNames, 0 },
    { "getMaxListeners", events_proto_getMaxListeners, 0 },
    { "listenerCount", events_proto_listenerCount, 1 },
    { "listeners", events_proto_listeners, 0 },
    { "on", events_proto_on, 2 },
    { "once", events_proto_once, 2 },
    { "prependListener", events_proto_prependListener, 2 },
    { "prependOnceListener", events_proto_prependOnceListener, 2 },
    { "removeAllListeners", events_proto_removeAllListeners, 1 },
    { "removeListener", events_proto_removeListener, 2 },
    { "setMaxListeners", events_proto_setMaxListeners, 1 },
    { NULL, NULL, 0 }
};

/**
 * List of properties for EventEmitter object
 */
DUK_LOCAL dux_property_list_entry events_props[] = {
    { "defaultMaxListeners", events_defaultMaxListeners_getter, events_defaultMaxListeners_setter },
    { NULL, NULL, NULL }
};

/**
 * Entry of events module
 */
DUK_LOCAL duk_errcode_t events_entry(duk_context *ctx)
{
    /* [ require module exports ] */
    dux_push_named_c_constructor(ctx,
        "EventEmitter", events_constructor, 0,
        NULL, events_proto_funcs,
        events_props, NULL);
    /* [ require module exports constructor ] */
    duk_dup(ctx, 3);
    /* [ require module exports constructor constructor ] */
    duk_put_prop_string(ctx, 3, "EventEmitter");
    /* [ require module exports constructor ] */
    duk_push_uint(ctx, 10);
    duk_put_prop_string(ctx, -2, DUX_IPK_DEFMAXLISTENERS);
    /* [ require module exports constructor ] */
    duk_put_prop_string(ctx, 1, "exports");
    /* [ require module exports ] */
    return DUK_ERR_NONE;
}

/**
 * Initialize events module
 */
DUK_INTERNAL duk_errcode_t dux_events_init(duk_context *ctx)
{
    return dux_modules_register(ctx, "events", events_entry);
}

#endif  /* !DUX_OPT_NO_NODEJS_MODULES && !DUX_OPT_NO_EVENTS */
