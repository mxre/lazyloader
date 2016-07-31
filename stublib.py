#!/usr/bin/env python3

import re, sys, getopt

def write_loader_prelude(out, include, libnames, env):
    out.write("#include <stdlib.h>\n")
    out.write("#include <stdio.h>\n")
    out.write("#include <string.h>\n")
    out.write("#include <stdbool.h>\n")
    out.write("#ifdef _WIN32\n")
    out.write("#include <windows.h>\n")
    out.write("#else\n")
    out.write("#include <dlfcn.h>\n")
    out.write("#endif\n")
    out.write('#define DEBUG_ENVIRONMENT_VARIABLE "LAZYCPLEX_DEBUG"\n')
    out.write('#define LOADER_ENVIRONMENT_VARIABLE "{}"\n'.format(env))
    out.write(r'''
static int test_library(int min_version);

static void* load_symbol(const char* name);

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
#else // For GCC comaptible compilers
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

int initialize_lazy_loader(int min_version) {
    const char *dbg = getenv(DEBUG_ENVIRONMENT_VARIABLE);
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    int i;
    char* s = getenv(LOADER_ENVIRONMENT_VARIABLE);
    const char* libnames[] = {
        ""
        "",
#ifdef _WIN32
''')
    for name in libnames:
        out.write('        "{}.dll",\n'.format(name))
    out.write("#else\n")
    for name in libnames:
        out.write('        "{}.dll",\n'.format(name))
    out.write(r'''
#endif
        NULL };
    if (s != NULL)
        libnames[0] = s;

    PRINT_DEBUG("Looking for a suitable library.\n");
    for (i = 0; libnames[i] != NULL; i++){
        if (strlen(libnames[i]) == 0) {
            continue;
        }
        if (try_lazy_load(libnames[i], min_version) == 0) {
            break;
        }
    }

    if (handle == NULL) {
        PRINT_DEBUG("Library lookup failed! Suggest setting a library path manually.\n")
        return 1;
    } else {
        return 0;
    }
}

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

static int test_library(int min_version)
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
static __inline void* load_symbol(const char *name)
{
    void* symbol = NULL;
    if (!handle || !(symbol = SYMBOL(name))) {
        failure_callback(name, callback_data);
    } else {
        PRINT_DEBUG("successfully imported the symbol %s\n", name, symbol);
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

static void exit_lazy_loader(void)
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

// for windows create static symbols
#define BUILD_CPXSTATIC 1
// for GCC
#ifdef _WIN64
#define _LP64 1
#endif
''')
    out.write('#include "{}"\n'.format(include))
    out.write('#include "lazyloader.h"\n')
    out.write('''
// remove for windows some of the macros
#undef CPXPUBLIC
#define CPXPUBLIC
#undef CPXPUBVARARGS
#define CPXPUBVARARGS

''')

re_name = re.compile(r"[^\(]* (\w+) *\(")
def function_name(name):
    name = name.replace("__attribute__((__stdcall__))", "")
    name = name.replace("__attribute__((__cdecl__))", "")
    name = name.replace("__attribute__", "")
    name = name.replace("__cdecl__", "")
    name = name.replace("__stdcall__", "")
    name = name.replace("__stdcall", "")
    name = name.replace("__cdecl", "")
    name = name.strip()
    match = re.match(re_name, name)
    if match == None:
        return None
    else:
        name = match.group(1)
        return name

def fun_pointer_replace(match):
    return match.group(1)

re_args = re.compile(r"[^(]*\((.*)\) *;")
re_fnpointer = re.compile(r"\(([^\)]*)\) *\([^\)]*\)")
re_argsnames = re.compile(r"(?:[^,]*\W(\w+)(?:,|$))")
def function_args(name):
    name = name.replace("__attribute__((__stdcall__))", "")
    name = name.replace("__attribute__((__cdecl__))", "")
    name = name.replace("__attribute__", "")
    name = name.replace("__cdecl__", "")
    name = name.replace("__stdcall__", "")
    name = name.replace("__stdcall", "")
    name = name.replace("__cdecl", "")
    name = name.strip()
    name = name.replace("...", "")
    match = re.match(re_args, name)
    if match == None:
        return None
    name = match.group(1)
    name = re.sub(re_fnpointer, fun_pointer_replace, name)
    match = re.findall(re_argsnames, name)
    return ", ".join(k for k in match)

def symbol_declaration(line, out):
    name = function_name(line)
    if name != None:
        symbol = "native_" + name
        out.write(line.replace(name, "(*{})".format(symbol)).replace(";", ""))
        out.write(" = NULL;\n".format(symbol))

def function_from_line(line, out):
    name = function_name(line)
    args = function_args(line)
    if name != None:
        symbol = "native_" + name
        out.write("// {}\n".format(name))
        out.write(line.replace(";", "\n{\n"))
        out.write("    if ({} == NULL)\n".format(symbol))
        out.write('        {} = load_symbol("{}");\n'.format(symbol, name))
        out.write("    return {}({});\n".format(symbol, args))
        out.write("}\n\n")

def usage():
    print("stublib.py -i header -l libnames -e envar -o output")
    print("")
    print("-i header to process, this header will also be included in the C file")
    print("-l names of the libraries to try (without lib prefix and suffix)")
    print("-e name of an environment variable that will be considered when loading libraries")

if __name__ == "__main__":
    include = None
    env = None
    libnames = None
    outfile = None

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:l:e:o:")
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for o, a in opts:
        if o == "h":
            usage()
            sys.exit(0)
        elif o == "-i":
            include = a
        elif o == "-l":
            libnames = a.split(",")
        elif o == "-e":
            env = a
        elif o == "-o":
            outfile = a
        else:
            assert False, "unhandled option: {}".format(o)

    if include == None or env == None or libnames == None:
        usage()
        sys.exit(1)

    w = open(outfile, "w", encoding='utf_8', newline='\n')
    write_loader_prelude(w, include, libnames, env)
    with open(include) as header:
        f = header.read()
        f = re.sub(r"\\\n", "", f)
        f = re.sub(r"#.*\n", "", f)
        f = re.sub(r"[{}]", "", f)
        f = f.replace('extern "C"', "")
        f = f.replace("\r\n", " ").replace("\n", " ").replace(";", ";\n")
        f = re.sub(r"/\*.*\*/", "", f)
        f = re.sub(r"typedef[^\n]+\n", "", f)
        f = re.sub(r"[\t ]+", " ", f)
        f = re.sub(r"^[\t ]+", "", f)
        f = re.sub(r"\n ", "\n", f)
        # a = open("w.h", "w")
        # a.write(f)
        # a.close()
        for line in f.splitlines():
            symbol_declaration(line, w)
        w.write("\n")
        for line in f.splitlines():
            function_from_line(line, w)
    w.close()