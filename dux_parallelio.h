#ifndef DUX_PARALLELIO_H_
#define DUX_PARALLELIO_H_

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dux_parallelio_t
{
	duk_int_t width;
	duk_int_t offset;
	duk_uint_t mask;
	duk_uint_t *val_ptr;
	duk_uint_t *dir_ptr;
	duk_uint_t dir_val;
	duk_uint_t dir_pol;
	duk_uint_t pol_val;
}
dux_parallelio_t;

extern void dux_parallelio_init(duk_context *ctx);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* DUX_PARALLELIO_H_ */
