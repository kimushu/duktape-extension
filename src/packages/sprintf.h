#ifndef SPRINTF_H_INCLUDED
#define SPRINTF_H_INCLUDED

#if defined(DUX_ENABLE_PACKAGE_SPRINTF)

/*
 * Functions
 */

DUK_INTERNAL_DECL duk_errcode_t dux_package_sprintf(duk_context *ctx);
#define DUX_INIT_PACKAGE_SPRINTF    dux_package_sprintf,

#else   /* !DUX_ENABLE_PACKAGE_SPRINTF */

#define DUX_INIT_PACKAGE_SPRINTF

#endif  /* !DUX_ENABLE_PACKAGE_SPRINTF */
#endif  /* !SPRINTF_H_INCLUDED */
