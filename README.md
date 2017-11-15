# Extension modules for Duktape

[![Build Status](https://travis-ci.org/kimushu/duktape-extension.svg?branch=master)](https://travis-ci.org/kimushu/duktape-extension)

This extension adds Node.js-like environment and embedded hardware support to [Duktape](https://github.com/svaarala/duktape) (>= 2.x)

* ES6 features
  * Promise (*Not* fully compatible to ECMA262 6th edition)
* Node.js-like environment
  * Console
  * Modules
  * Path
  * Process
  * Timers
  * Utilities
* Embedded hardware support for Rubic-compatible firmware (for example: [olive](https://github.com/kimushu/olive-piccolo))
  * Hardware
  * Peridot

# How to install

1. Download distribution package (duktape-extension-x.x.x.tar.gz) from [releases](https://github.com/kimushu/duktape-extension/releases)
1. Extract `dist` directory from downloaded archive
1. Copy extracted files to your C/C++ project
1. Edit `dux_config.h` to customize duktape-extension

# How to use

This extension has only several public functions:
* `dux_initialize()` : Initialize duktape-extension for specified Duktape context (`duk_context`)
* `dux_tick()` : Process tick routines for event loop
* `dux_[p]eval_module_file[_noresult]()` : Load file as a module and evalulate it
* `dux_[p]eval_module_[l]string[_noresult]()` : Load string (JavaScript) as a module and evaluate it

### Example
```c
#include <duktape.h>
#include <dukext.h>

int main()
{
    duk_context *ctx;

    // Create Duktape heap
    ctx = duk_create_heap_default();

    // Initialize duktape-extension without file accessor
    dux_initialize(ctx, NULL);

    // Load main source
    duk_eval_string_noresult(ctx, "setTimeout(function(){ console.log('hello world') }, 1000)");

    // Start event loop
    while (dux_tick(ctx)) {
        // dux_tick returns true if there are any incomplete jobs
        // You may insert some delays here to avoid busy loop
    }

    // Event loop finished
    duk_destroy_heap(ctx);

    // You don't need call any terminate function for duktape-extension.
    // All resources of duktape-extension will be destroyed in duk_destroy_heap()
    return 0;
}
```

# Author
kimu_shu ([GitHub](https://github.com/kimushu) | [Twitter](https://twitter.com/kimu_shu))

# License
The MIT license
