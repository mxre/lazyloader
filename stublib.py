#!/usr/bin/env python3

#
# This file is part of lazycplex.
#
# Copyright 2016 Max Resch
#
# Permission is hereby granted, free of charge, to any
# person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal  in the
# Software without restriction, including without
# limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice
# shall be included in all copies or substantial portions
# of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# see http://www.opensource.org/licenses/MIT
#

import re, sys, getopt, os

def write_loader(out, libnames, env):
    out.write('#define DEBUG_ENVIRONMENT_VARIABLE "LAZYLOAD_DEBUG"\n')
    out.write('#ifdef _WIN32\n')
    out.write('#define LOADER_ENVIRONMENT_VARIABLE L"{}"\n'.format(env))
    out.write('#else\n')
    out.write('#define LOADER_ENVIRONMENT_VARIABLE "{}"\n'.format(env))
    out.write('#endif\n')
    out.write('#include "src/loader.c"\n')
    out.write(r'''
int initialize_lazy_loader(int min_version) {
    const char *dbg = getenv(DEBUG_ENVIRONMENT_VARIABLE);
    debug_enabled = (dbg != NULL && strlen(dbg) > 0);

    if (handle != NULL) {
        PRINT_DEBUG("The library is already initialized.\n");
        return 0;
    }

    int i;
#ifdef _WIN32
    wchar_t* s = _wgetenv(LOADER_ENVIRONMENT_VARIABLE);
    const wchar_t* libnames[] = {
        L""
        L"",
''')
    for name in libnames:
        out.write('        L"{}.dll",\n'.format(name))
    out.write('''#else
    char* s = getenv(LOADER_ENVIRONMENT_VARIABLE);
    const char* libnames[] = {
        ""
        "",
''')
    for name in libnames:
        out.write('        "lib{}.so",\n'.format(name))
    out.write(r'''
#endif
        NULL };
    if (s != NULL)
        libnames[0] = s;

    PRINT_DEBUG("Looking for a suitable library.\n");
    for (i = 0; libnames[i] != NULL; i++) {
        if (libnames[i][0] == 0) {
            continue;
        }
        if (try_lazy_load(libnames[i], min_version) == 0) {
            break;
        }
    }

    if (handle == NULL) {
        if (try_detect_library(min_version) == 0) {
            return 0;
        }
    }

    if (handle == NULL) {
        PRINT_DEBUG("Library lookup failed! Suggest setting a library path manually.\n")
        return 1;
    } else {
        return 0;
    }
}
''')

def write_loader_prelude(out, include):
    out.write("// This file was generated by stublib.py from {}\n\n".format(include))
    out.write('''
// for windows create static symbols
#define BUILD_CPXSTATIC 1
// for GCC
#ifdef _WIN64
#define _LP64 1
#endif
''')
    out.write('#include "src/loader.h"\n')
    out.write('#include "{}"\n'.format(include))
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
    if match is None:
        return None
    name = match.group(1)
    name = re.sub(re_fnpointer, fun_pointer_replace, name)
    match = re.findall(re_argsnames, name)
    return ", ".join(k for k in match)

def symbol_declaration(line, name, out):
    if name != None:
        symbol = "native_" + name
        out.write(line.replace(name, "(*{})".format(symbol)).replace(";", ""))
        out.write(" = NULL;\n".format(symbol))

def function_from_line(line, name, out):
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
    print("stublib.py [-i header | -p -l libnames -e envar] -o output")
    print("")
    print("-i header to process, this header will also be included in the C file")

    print("-p don't parse include, write loader")
    print("-l names of the libraries to try (without lib prefix and suffix)")
    print("-e name of an environment variable that will be considered when loading libraries")
    print("-t generate a text file containing all generated objects and c files (for windows)")

if __name__ == "__main__":
    include = None
    env = None
    libnames = None
    outfile = None
    textfile = None
    makefile = None

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:l:e:o:pt")
    except getopt.GetoptError:
        usage()
        sys.exit(1)
    loader = False
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
        elif o == "-p":
            loader = True
        elif o == "-t":
            textfile = True
        else:
            assert False, "unhandled option: {}".format(o)

    if (include is None and not loader) or ((env is None or libnames is None) and loader):
        usage()
        sys.exit(1)

    if loader:
        with open(outfile, "w", encoding='utf_8', newline='\n') as w:
            write_loader(w, libnames, env)
    else:
        if makefile:
            filename = os.path.splitext(os.path.basename(include))[0]
        with open(include) as header:
            f = header.read()
            f = re.sub(r"CPXDEPRECATEDAPI\([0-9]{8}\)", "", f)
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
                name = function_name(line)
                if name == None:
                    # print(line)
                    continue
                of = "{}/{}.c".format(outfile, name)
                with open(of, "w", encoding='utf_8', newline='\n') as w:
                    write_loader_prelude(w, include)
                    symbol_declaration(line, name, w)
                    function_from_line(line, name, w)
                    w.write("\n")
            if textfile:
                basename = os.path.splitext(os.path.basename(include))[0]
                of = "{}/{}.objects".format(outfile, basename)
                with open(of, "w", encoding='utf_8', newline='\n') as w:
                    for line in f.splitlines():
                        name = function_name(line)
                        w.write("{}/{}.obj\n".format(outfile, name))
                of = "{}/{}.sources".format(outfile, basename)
                with open(of, "w", encoding='utf_8', newline='\n') as w:
                    for line in f.splitlines():
                        name = function_name(line)
                        w.write("{}/{}.c\n".format(outfile, name))
