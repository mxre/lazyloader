#!/bin/bash
#
# This file is part of lazylpsolverlibs.
#
# You can redistribute it and/or modify it under the terms of the MIT
# license.
#
# You should have received a copy of the MIT license along
# with lazylpsolverlibs (file MIT). If not, see
# http://www.opensource.org/licenses/mit-license.php/


# extract the function name from a function declaration
func_name(){
    echo $@ | sed 's/__attribute__((__stdcall__))//g' \
            | sed 's/__attribute__((__cdecl__))//g' \
            | sed 's/__attribute__//g' \
            | sed 's/__cdecl__//g' \
            | sed 's/__stdcall__//g' \
            | sed 's/__stdcall//g' \
            | sed 's/__cdecl//g' \
            | sed 's/ *(.*) *;/();/g' \
            | sed 's/ /\n/g' \
            | grep '(' \
            | sed 's/();//g' \
            | sed 's/\*//g'
}

# extract the arguments names from a function declaration
func_args(){
    echo $@ | sed 's/__attribute__((__stdcall__))//g' \
            | sed 's/__attribute__((__cdecl__))//g' \
            | sed 's/__attribute__//g' \
            | sed 's/__cdecl__//g' \
            | sed 's/__stdcall__//g' \
            | sed 's/__stdcall//g' \
            | sed 's/__cdecl//g' \
            | sed 's/[^(]*(\(.*\)) *;/\1)/g' \
            | sed 's/(\([^)]*\)) *([^)]*)/\1/g' \
            | sed 's/, *\.\.\.//g' \
            | sed 's/ *, */, /g' \
            | sed 's/ *+ */+/g' \
            | sed 's/ *)/)/g' \
            | sed 's/ /\n/g' \
            | sed 's/\[.*\]//g' \
            | grep ',\|)' \
            | grep -v '\<void\>' \
            | tr '\n' ' ' \
            | sed 's/)//g' \
            | sed 's/\*//g' \
            | sed 's/ *$//g'
}

# create a function from a function declaration. This function actually
# imports the original symbol and passes its arguments to this symbol.
make_func(){
    name=$(func_name "$*")
    symbol="native_$name"
    args=$(func_args "$*")
    echo "$*" | sed 's/;/{/g'
    echo "    if (!$symbol)
        $symbol = load_symbol(\"$name\");
    return $symbol($args);
}"
}

# declare a symbol to import a function that matches the input function
# declaration to
make_symbol_decl(){
    name=$(func_name "$*")
    symbol="native_$name"
    echo "$*"   | sed "s/\<$name\>/(*$symbol)/g" \
                | sed 's/;/ = NULL;/g'
}

# create the loading interface
make_loading_interface(){
echo "#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// for windows create static symbols
#define BUILD_CPXSTATIC 1
// for GCC
#ifdef _WIN64 
#define _LP64 1
#endif
"
if [ x"$declared_header" != x ]; then
    echo "#include \"$declared_header\""
else
    echo "#include \"$header\""
fi
if [ x"$register_callback" != x ]; then
    echo "#include <lazyloader.h>"
fi
cat <<_EOF_
// remove for windows some of the macros
#undef CPXPUBLIC
#define CPXPUBLIC
#undef CPXPUBVARARGS
#define CPXPUBVARARGS

/* Debugging macros */
#ifdef _MSC_VER
#define PRINT_DEBUG(fmt, ...) if (debug_enabled) { \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ ); }
#define PRINT_ERR(fmt, ...) \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, __VA_ARGS__ );
#else
#define PRINT_DEBUG(fmt, args ...) if (debug_enabled) { \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args ); }
#define PRINT_ERR(fmt, args ...) \
    fprintf( stderr, "\n(%s): " fmt, __FUNCTION__, ## args );
#endif

static void default_failure_callback(const char* symbol, void* cb_data);
static void* load_symbol(const char* name);

/* Handle to the library */
static void* handle = NULL;

/* Called whenever the library is about to crash */
static void (*failure_callback)(const char* symbol, void* cb_data) = default_failure_callback;

static void* callback_data = NULL;

static bool debug_enabled = false;

/* Prints the failing symbol and aborts */
void default_failure_callback(const char* symbol, void* cb_data){
    PRINT_ERR ("the symbol %s could not be found!\n", symbol);
    abort();
}

void* get_lazy_handle() {
	return handle;
}

void set_lazy_handle(void* handle) {
	handle = handle;
}

static void exit_lazy_loader() {
    if (handle != NULL)
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        handle = NULL;
}

#ifdef _WIN32
#define SYMBOL(sym) GetProcAddress(handle, sym)
#else
#define SYMBOL(sym) dlsym(handle, sym)
#endif

#ifdef _WIN32
#define FREE FreeLibrary(handle)
#else
#define FREE dlclose(handle)
#endif

int test_library(int min_version) {
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

int try_lazy_load(const char* library, int min_version) {
    const char *dbg = getenv("LAZYCPLEX_DEBUG");
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    PRINT_DEBUG("Trying to load %s\n", library);
#ifdef _WIN32
    handle = LoadLibrary(library);
#else
    handle = dlopen(library, RTLD_LAZY);
#endif
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
_EOF_
echo "
/* Searches and loads the actual library */
int initialize_lazy_loader(int min_version) {
    const char *dbg = getenv(\"LAZYCPLEX_DEBUG\");
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG(\"The library is already initialized.\\n\");
        return 0;
    }

    int i;
    char* s = getenv(\"$environment_var\");
    const char* libnames[] = {
        \"\"
        \"\",
#ifdef _WIN32"
for name in $libnames; do
    echo "        \"$name.dll\","
done
echo "#else"
for name in $libnames; do
    echo "        \"lib$name.so\","
done
echo "#endif"
echo "        NULL };"
if [ x"$environment_var" != x ]; then
    echo "    if (s != NULL)
        libnames[0] = s;"
fi
echo "
    PRINT_DEBUG(\"Looking for a suitable library.\\n\");
    for (i = 0; libnames[i] != NULL; i++){
        if (strlen(libnames[i]) == 0) {
            continue;
        }
        if (try_lazy_load(libnames[i], min_version) == 0) {
            break;
        }
    }

    if (handle == NULL) {
        PRINT_DEBUG(\"Library lookup failed! Suggest setting a library path manually.\\n\")
        return 1;
    } else {
        return 0;
    }
}

static __inline void* load_symbol(const char *name){
    void* symbol = NULL;
#ifdef _WIN32
    if (!handle || !(symbol = GetProcAddress(handle, name))) {
#else
    if (!handle || !(symbol = dlsym(handle, name))) {
#endif
        failure_callback(name, callback_data);
    } else {
        PRINT_DEBUG(\"successfully imported the symbol %s at %p.\\n\", name, symbol);
    }
    return symbol;
}"


if [ x"$register_callback" != x ]; then
    echo "
void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data) {
    failure_callback = f;
    callback_data = cb_data;
}"
fi
}

header_to_cfile(){
    func_decl=$(mktemp)
    cat $header | grep -v "#include" \
                | $cpp \
                | grep -v '^#' \
                | tr '\n' ' ' \
                | sed 's/{[^}]*}/{}/g' \
                | sed 's/;/;\n/g' \
                | grep '(' \
                | sed 's/\s\+/ /g' \
                | sed 's/^ *//g' \
                | grep -v 'typedef' \
                > $func_decl
    make_loading_interface
    echo ""
    echo "/* imported functions */"
    echo ""
    while read line; do
        make_symbol_decl "$line"
    done < $func_decl
    echo ""
    echo "/* hijacked functions */"
    echo ""
    while read line; do
        make_func "$line"
    done < $func_decl
    rm $func_decl
}

usage(){
    echo "Usage: stublib.sh -i header -l libnames [-e environment_var] [-d declared_header] [-c c_preprocessor]"
    echo ""
    echo " - libnames has to be a coma-separated list, without the prefix lib nor the extension name .so"
    echo " - environment_var will be tried just after (interpreted as a full path as well)"
    echo " - libnames will be tried in the order specified, in every directory of LD_LIBRARY_PATH (linux) or PATH (windows)"
    echo " - declared_header will be used instead of header if specified"
    echo " - c_preprocessor will be used instead of cpp if specified"
}

cpp=cpp
while getopts "hi:l:e:d:c:r" opt; do
    case $opt in
        h)
            usage
            ;;
        i)
            header=$OPTARG
            test -f $header || (echo "$header: No such file or directory" >&2 && exit 1)
            ;;
        l)
            libnames=$(echo $OPTARG | sed "s/,/ /g")
            ;;
        e)
            environment_var=$OPTARG
            ;;
        d)
            declared_header=$OPTARG
            ;;
        c)
            cpp=$OPTARG
            ;;
        r)
            register_callback=x
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            exit 1
            ;;
    esac
done

test -z $header && echo "You must provide an argument to -i" && usage && exit
header_to_cfile
