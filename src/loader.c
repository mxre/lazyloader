/**
 * This file is part of lazycplex
 *
 * Copyright 2016 Max Resch
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal  in the
 * Software without restriction, including without
 * limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice
 * shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * see http://www.opensource.org/licenses/MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "loader.h"
#include "lazyloader.h"

static int test_library(int min_version);

static void exit_lazy_loader(void);

static void default_failure_callback(const char* symbol, void* cb_data);

static void (*failure_callback)(const char* symbol, void* cb_data) = default_failure_callback;

static void* callback_data = NULL;

// debugging macros
#ifdef _MSC_VER
#define PRINT_DEBUG(fmt, ...) \
    if (debug_enabled) {\
        fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );\
    }
#define PRINT_ERR(fmt, ...) \
    fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );
#else // For GCC compatible compilers
#define PRINT_DEBUG(fmt, args ...) \
    if (debug_enabled) {\
        fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, ## args );\
    }
#define PRINT_ERR(fmt, args ...) \
    fprintf(stderr, "\n(%s): " fmt, __FUNCTION__, ## args );
#endif

// platfrom macros
#ifdef _WIN32
#define SYMBOL(name) GetProcAddress(handle, name)
#else
#define SYMBOL(name) dlsym(handle, name)
#endif
#ifdef _WIN32
#define FREE FreeLibrary(handle)
#else
#define FREE dlclose(handle)
#endif
#ifdef _WIN32
#define OPEN(library) LoadLibrary(library)
#else
#define OPEN(library) dlopen(library, RTLD_LAZY)
#endif

// handle of the loaded library
static void* handle = NULL;

static bool debug_enabled = false;


int try_lazy_load(const char* library, int min_version)
{
    const char *dbg = getenv(DEBUG_ENVIRONMENT_VARIABLE);
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    PRINT_DEBUG("Trying to load %s\n", library);
    handle = OPEN(library);
    if (handle == NULL)
        return 1;
    else {
        int ret = test_library(min_version);
        if (ret == 0) {
            PRINT_DEBUG("Success!\n");
            atexit(exit_lazy_loader);
        }
        return ret;
    }
}

int test_library(int min_version)
{
    if (handle == NULL) {
        return 1;
    }
    void* symbol = NULL;
    symbol = SYMBOL("CPXopenCPLEX");
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXopenCPLEX\n");
        FREE;
        handle = NULL;
        return 2;
    }
    int status = 0;
    void* env = ((void* (*) (int*)) (symbol)) (&status);
    if (env == NULL || status != 0) {
        PRINT_DEBUG("Could not open a CPLEX environment\nCPLEX error: %d\n", status);
        FREE;
        handle = NULL;
        return 3;
    }
    symbol = SYMBOL("CPXversionnumber");
    int ret = 0;
    int version = 0;
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXversionnumber, fallback\n");
        symbol = SYMBOL("CPXversion");
        if (symbol == NULL) {
            PRINT_DEBUG("Library is missing CPXversion\n");
            FREE;
            handle = NULL;
            return 2;
        }
        const char* verstring =
        ((const char* (*) (void*)) (symbol)) (env);
        if (verstring == NULL) {
            PRINT_DEBUG("Could not retrieve a version from the library\n");
            ret = 3;
        }
        int major, minor, micro, patch;
        if (sscanf(verstring, "%d.%d.%d.%d", &major, &minor, &micro, &patch) != 4) {
            PRINT_DEBUG("Could not retrieve a version from the library\nCPLEX error: %d\n", status);
            ret = 3;
        } else {
            version = major * 1000000 + minor * 10000 + micro * 100 + patch;
        }
    } else {
        status = ((int (*) (void*, int*)) (symbol)) (env, &version);
        if (status != 0) {
            PRINT_DEBUG("Could not retrieve a version from the library\nCPLEX error: %d\n", status);
            ret = 3;
        }
    }
    if (ret == 0) {
        PRINT_DEBUG("Loaded CPLEX library version: %d.%d.%d.%d\n",
                    (version / 1000000)      ,
                    (version / 10000  ) % 100,
                    (version / 100    ) % 100,
                    (version          ) % 100);
        if (version < min_version) {
            PRINT_DEBUG("Library version marked as too old\n");
            ret = 4;
        }
    }
    symbol = SYMBOL("CPXcloseCPLEX");
    if (symbol == NULL) {
        PRINT_DEBUG("Library is missing CPXcloseCPLEX\n");
        FREE;
        handle = NULL;
        return 2;
    }
    ((int (*) (void**)) (symbol)) (&env);
    if (ret != 0) {
        FREE;
        handle = NULL;
    }
    return ret;
}

// load a symbol with the given name
void* load_symbol(const char *name)
{
    void* symbol = NULL;
    if (!handle || !(symbol = SYMBOL(name))) {
        failure_callback(name, callback_data);
    } else {
        PRINT_DEBUG("successfully imported the symbol %s\n", name);
    }
    return symbol;
}

void* get_lazy_handle()
{
	return handle;
}

void set_lazy_handle(void* handle)
{
	handle = handle;
}

void exit_lazy_loader(void)
{
    if (handle != NULL)
        FREE;
        handle = NULL;
}

void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data) {
    failure_callback = f;
    callback_data = cb_data;
}

// Prints the failing symbol and aborts
void default_failure_callback(const char* symbol, void* cb_data)
{
    PRINT_ERR("the symbol %s could not be found!\n", symbol);
    abort();
}

