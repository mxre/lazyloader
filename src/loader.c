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
#include <glob.h>
#endif
#include "loader.h"
#include "lazyloader.h"

#define DEFAULT_INSTALL_PATH_UNIX "/opt/ibm/ILOG/CPLEX_Studio{???,????}/cplex/bin/*/libcplex{???,????}.so"

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

    PRINT_DEBUG("Trying to load %s", library);
    handle = OPEN(library);
    if (handle == NULL) {
        //PRINT_DEBUG("Failed.\n");
        return 1;
    } else {
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

int try_detect_library(int min_version)
{
#ifdef _WIN32
    char* prgm = getenv("PROGRAMFILES");
    if (NULL == prgm) {
        PRINT_ERR("Could not retrieve %%PROGRAMFILES%% from environment.\n");
        return 1;
    }

    char path[MAX_PATH];
    strcpy(path, prgm);
    strcat(path, "\\IBM\\ILOG\\CPLEX_Studio*");
    WIN32_FIND_DATA ffd;
    HANDLE find = INVALID_HANDLE_VALUE;

     find = FindFirstFileEx(path, FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, 0);
    if (INVALID_HANDLE_VALUE == find) {
        return 1;
    }

    WIN32_FIND_DATA ffd2;
    HANDLE find2 = INVALID_HANDLE_VALUE;
    do {
        sprintf(path, "%s\\IBM\\ILOG\\%s\\cplex\\bin\\x64_win64\\cplex????.dll", prgm, ffd.cFileName);
        find2 = FindFirstFile(path, &ffd2);
        if (INVALID_HANDLE_VALUE == find2) {
            continue;
        }
        do {
            if (13 == strlen(ffd2.cFileName)) {
                sprintf(path, "%s\\IBM\\ILOG\\%s\\cplex\\bin\\x64_win64\\%s", prgm, ffd.cFileName, ffd2.cFileName);
                if (try_lazy_load(path, min_version) == 0) {
                    FindClose(find2);
                    FindClose(find2);
                    return 0;
                }
            }
        } while (FindNextFile (find2, &ffd2) != 0);
        FindClose(find2);
    } while (FindNextFile (find, &ffd) != 0);
    FindClose(find2);
    return 1;
#else
    glob_t globs;
	memset(&globs, 0, sizeof globs);
	glob(DEFAULT_INSTALL_PATH_UNIX, GLOB_BRACE, NULL, &globs);

	for (int i = globs.gl_pathc - 1; i >= 0;  i--) {
		if (try_lazy_load(globs.gl_pathv[i], min_version) == 0) {
            globfree(&globs);
            return 0;
        }
	}

	globfree(&globs);
    return 1;
#endif
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

#ifdef AUTOMATIC_LOAD
#if defined(_MSC_VER)
    #pragma section(".CRT$XCU",read)
    #define INITIALIZER2_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)
    #ifdef _WIN64
        #define INITIALIZER(f) INITIALIZER2_(f,"")
    #else
        #define INITIALIZER(f) INITIALIZER2_(f,"_")
    #endif
#elif defined(__GNUC__)
	#define INITIALIZER(f) \
	__attribute__((constructor)) static void f(void)
#else
    #error "Compiler not supported"
#endif

INITIALIZER(initialize)
{
    PRINT_DEBUG("Init lazyloader\n");
    initialize_lazy_loader(12060000);
}
#endif
