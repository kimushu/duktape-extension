#ifndef DUX_NODE_H_INCLUDED
#define DUX_NODE_H_INCLUDED

#if !defined(DUX_OPT_NO_NODEJS_MODULES)

#include "dux_events.h"
#include "dux_console.h"
#include "dux_process.h"
#include "dux_timer.h"
#include "dux_util.h"

DUK_INTERNAL_DECL duk_errcode_t dux_node_init(duk_context *ctx);
DUK_INTERNAL_DECL duk_int_t dux_node_tick(duk_context *ctx);
#define DUX_INIT_NODE   dux_node_init,
#define DUX_TICK_NODE   dux_node_tick,

#else   /* !DUX_OPT_NO_NODEJS_MODULES */

#define DUX_INIT_NODE
#define DUX_TICK_NODE

#endif  /* DUX_OPT_NO_NODEJS_MODULES */
#endif  /* !DUX_NODE_H_INCLUDED */
