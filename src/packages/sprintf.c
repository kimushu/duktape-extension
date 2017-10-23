/**
 * @file sprintf.c
 * @brief sprintf function (like npm/sprintf-js)
 * @note Named arguments are not supported.
 */

#if defined(DUX_ENABLE_PACKAGE_SPRINTF)

#include "../dux_internal.h"
#include <string.h>

DUK_LOCAL duk_ret_t sprintf_body(duk_context *ctx)
{
    /* [ format ... ]       : if magic=0 */
    /* [ format arr(args) ] : if magic=1 */

    char temp[128];
    const char *format = duk_require_string(ctx, 0);
    char *output;
    int output_len = 0;
    int buflen = (strlen(format) + 1) * 2;
    if (buflen < sizeof(temp)) {
        buflen = sizeof(temp);
    }

    if (duk_get_current_magic(ctx)) {
        duk_uarridx_t i;
        duk_size_t len = duk_get_length(ctx, 1);
        for (i = 0; i < len; ++i) {
            duk_get_prop_index(ctx, 1, i);
        }
        output = (char *)duk_push_dynamic_buffer(ctx, buflen);
        duk_replace(ctx, 1);
    } else {
        output = (char *)duk_push_dynamic_buffer(ctx, buflen);
        duk_insert(ctx, 1);
    }

    /* [ format output ... ] */
    int arg_index = 2;

    for (;;) {
        char ch = *format++;
        if (ch != '%') {
out_char:
            if (output_len == buflen) {
                buflen *= 2;
                output = (char *)duk_resize_buffer(ctx, 1, buflen);
            }
            output[output_len++] = ch;
            if (ch == '\0') {
                break;
            }
            continue;
        }
        const char *chunk = format - 1;
        int width = 0;
        int prec = -1;
        int flags = 0;  // 1:conv(#),2:zero(0),4:left(-),8:space( ),16:sign(+)
        ch = *format++;
        // Flags
        for (;; ch = *format++) {
            if (ch == '#') {
                flags |= 1;
            } else if (ch == '0') {
                flags |= 2;
            } else if (ch == '-') {
                flags |= 4;
            } else if (ch == ' ') {
                flags |= 8;
            } else if (ch == '+') {
                flags |= 16;
            } else {
                break;
            }
        }
        // Width
        for (;; ch = *format++) {
            if (('0' <= ch) && (ch <= '9')) {
                width = width * 10 + (ch - '0');
            } else {
                break;
            }
        }
        if (ch == '*') {
            width = duk_require_int(ctx, arg_index++);
            ch = *format++;
        }
        // Precision
        if (ch == '.') {
            ch = *format++;
            prec = 0;
            for (;; ch = *format++) {
                if (('0' <= ch) && (ch <= '9')) {
                    prec = prec * 10 + (ch - '0');
                } else {
                    break;
                }
            }
        }
        // Length modifier => Not used
        // Conversion specifier
        int type = 0;
        switch (ch) {
        case '%':
            goto out_char;
        case 'b':
            // Binary number
            {
                char *buf = temp + sizeof(temp);
                *--buf = '\0';
                duk_uint_t value = duk_require_uint(ctx, arg_index++);
                do {
                    *--buf = '0' + (value & 1);
                    value >>= 1;
                } while (value != 0);
                chunk = buf;
            }
            break;
        case 'c':
        case 'd':
        case 'i':
            // int
            type = 1;
            break;
        case 'o':
        case 'O':
        case 'u':
        case 'x':
        case 'X':
            // uint
            type = 2;
            break;
        case 'e':
        case 'f':
        case 'g':
            // float
            type = 3;
            break;
        case 's':
            // string
            chunk = duk_safe_to_string(ctx, arg_index++);
            flags &= ~2;    // Disable zero padding
            break;
        case 't':
            // boolean
            if (duk_require_boolean(ctx, arg_index++)) {
                chunk = "true";
            } else {
                chunk = "false";
            }
            flags &= ~2;    // Disable zero padding
            break;
        case 'T':
            // type (123 => "number", null => "null", {} => "object", [] => "array")
            switch (duk_get_type(ctx, arg_index++)) {
            case DUK_TYPE_NONE:
                return DUK_RET_RANGE_ERROR;
            case DUK_TYPE_UNDEFINED:
                chunk = "undefined";
                break;
            case DUK_TYPE_NULL:
                chunk = "null";
                break;
            case DUK_TYPE_BOOLEAN:
                chunk = "boolean";
                break;
            case DUK_TYPE_NUMBER:
                chunk = "number";
                break;
            case DUK_TYPE_STRING:
                chunk = "string";
                break;
            case DUK_TYPE_OBJECT:
                if (duk_is_array(ctx, arg_index - 1)) {
                    chunk = "array";
                } else if (duk_is_function(ctx, arg_index - 1)) {
                    chunk = "function";
                } else {
                    chunk = "object";
                }
                break;
            case DUK_TYPE_BUFFER:
                chunk = "buffer";
                break;
            case DUK_TYPE_POINTER:
                chunk = "pointer";
                break;
            case DUK_TYPE_LIGHTFUNC:
                chunk = "function";
                break;
            }
            flags &= ~2;    // Disable zero padding
            break;
        case 'v':
            // primitive value
            duk_to_primitive(ctx, arg_index, DUK_HINT_STRING);
            chunk = duk_safe_to_string(ctx, arg_index++);
            flags &= ~2;    // Disable zero padding
            break;
        case 'j':
            // JSON
            chunk = duk_json_encode(ctx, arg_index++);
            flags &= ~2;    // Disable zero padding
            break;
        default:
            // Syntax error
            return DUK_RET_SYNTAX_ERROR;
        }

        if (type > 0) {
            char cformat[20];
            char *fmt = cformat;
            *fmt++ = '%';
            if (flags & 1) {
                *fmt++ = '#';
            }
            if (flags & 2) {
                *fmt++ = '0';
            }
            if (flags & 4) {
                *fmt++ = '-';
            }
            if (flags & 8) {
                *fmt++ = ' ';
            }
            if (flags & 16) {
                *fmt++ = '+';
            }
            sprintf(fmt, "%d%c", width, ch);
            chunk = temp;

            switch (type) {
            case 1:
                // int
                {
                    duk_int_t value = duk_require_int(ctx, arg_index++);
                    snprintf(temp, sizeof(temp), cformat, value);
                }
                break;
            case 2:
                // uint
                {
                    duk_uint_t value = duk_require_uint(ctx, arg_index++);
                    snprintf(temp, sizeof(temp), cformat, value);
                }
                break;
            case 3:
                // float
                {
                    duk_float_t value = (duk_float_t)duk_require_number(ctx, arg_index++);
                    snprintf(temp, sizeof(temp), cformat, value);
                }
                break;
            }
        }
        int chunk_len = strlen(chunk);
        int total_len;
        if (chunk_len < width) {
            total_len = width;
        } else {
            total_len = chunk_len;
        }
        int resize = 0;
        while ((output_len + total_len) > buflen) {
            buflen *= 2;
            resize = 1;
        }
        if (resize) {
            output = (char *)duk_resize_buffer(ctx, 1, buflen);
        }
        if (!(flags & 4)) {
            // right
            int pad_len = total_len - chunk_len;
            if (pad_len > 0) {
                memset(output + output_len, (flags & 2) ? '0' : ' ', pad_len);
                output_len += pad_len;
            }
        }
        memcpy(output + output_len, chunk, chunk_len);
        output_len += chunk_len;
        if (flags & 4) {
            // left
            int pad_len = total_len - chunk_len;
            if (pad_len > 0) {
                memset(output + output_len, ' ', pad_len);
                output_len += pad_len;
            }
        }
    }
    duk_push_string(ctx, output);
    return 1;
}

DUK_LOCAL duk_ret_t package_sprintf_entry(duk_context *ctx)
{
    fprintf(stderr, "CHECKPOINT1!\n");
    /* [ require module exports ] */
    duk_push_c_function(ctx, sprintf_body, DUK_VARARGS);
    duk_set_magic(ctx, -1, 0);
    duk_put_prop_string(ctx, 2, "sprintf");
    /* [ require module exports ] */
    duk_push_c_function(ctx, sprintf_body, 2);
    duk_set_magic(ctx, -1, 1);
    duk_put_prop_string(ctx, 2, "vsprintf");
    /* [ require module exports ] */
    return DUK_ERR_NONE;
}

DUK_LOCAL duk_ret_t package_sprintfjs_entry(duk_context *ctx)
{
    fprintf(stderr, "CHECKPOINT2!\n");
    /* [ require module exports ] */
    duk_dup(ctx, 0);
    /* [ require module exports require ] */
    duk_push_string(ctx, "sprintf");
    duk_call(ctx, 1);
    /* [ require module exports sprintfjs ] */
    duk_get_prop_string(ctx, 3, "sprintf");
    duk_put_prop_string(ctx, 2, "sprintf");
    duk_get_prop_string(ctx, 3, "vsprintf");
    duk_put_prop_string(ctx, 2, "vsprintf");
    duk_pop(ctx);
    /* [ require module exports ] */
    return DUK_ERR_NONE;
}

/*
 * Initialize sprintf/sprintf-js package
 */
DUK_INTERNAL duk_errcode_t dux_package_sprintf(duk_context *ctx)
{
    duk_errcode_t result;
    result = dux_modules_register(ctx, "sprintf", package_sprintf_entry);
    if (result == DUK_ERR_NONE) {
        result = dux_modules_register(ctx, "sprintf-js", package_sprintfjs_entry);
    }
    return result;
}

#endif  /* DUX_ENABLE_PACKAGE_SPRINTF */
