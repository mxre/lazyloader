# lazycplex

The aim of this project is to provide an independent runtime for programs
using CPLEX. A program linked against this library, can load an appropriate
CPLEX library during runtime, as opposed during linktime (or program load,
when using the dynamic linker). Thus eliminating the need to link a static
version of CPLEX into the program, and therefore avoiding licensing issues.

This static library supports the standard C CPLEX API, (functions beginning with `CPX`)
and the more modern `CPXX` functions defined in `cplexx.h`, which are actually macros
directing to either `CPXS` or `CPXL` functions.

Unsupported are all functions marked with ``CPXDEPRECATEDAPI` in the headers.

## Example
See example/example.c

## Notes
* The example uses automatic loading and detection.
  This means, the source code does not have any reference
  to the loader.
* To use the automatic CPLEX loader the additional object
  `cplex_auto.obj` or `cplex_auto.o` has to be linked to the program.
* The loader tries to find the library in the following paths:
  * A library provided in the environement variable `$CPLEX_DLL`,
    this has to be a absolute path to library.
  * Default library names, i.e. libcplex1270.so or cplex1270.dll
    in the system's default library search path.
  * A library in a default install path:
    - `%PROGRAMFILES%\IBM\ILOG\CPLEX_Studio*\cplex\bin\*\cplex????.dll` for Windows or
    - `/opt/ibm/ILOG/CPLEX_Studio*/cplex/bin/*/libcplex????.so` for Linux
  * If there multiple versions of CPLEX are installed on the system, the library will choose
    the first one it finds this may not be the most recent version of CPLEX.
* If the environment variable `$LAZYLOAD_DEBUG` is set, the library will print debug
  output to the console.
* The header file `lazyloader.h` contains functions, that can be used to programmatically
  set the loader library. Note that if automatic loading is used,
  another library will only be loaded, if the automatic loading failed.
* The stub generator generates a source file for each funtion, this minimizses the size
  of an executable that is linked with this library since only required object files need to
  be included.

## Requirements
* For Linux nothing special is required, besides a CPLEX installation (we need the header files)
  to generate the source stubs and a recent C compiler (like GCC or clang) and Python 3
* For Windows we use Microsoft's Visual C Compiler 19 (Visual Studio 14),
  and Windows SDK 10.
  Futhermore Python3 is required to generate the source stubs from the CPLEX header files.

Generated headers and source stubs are identical for Windows and Linux, and can be shared.

## Acknowledgements
This project is somewhat similar to
https://github.com/chmduquesne/lazylpsolverlibs
but, we use a faster generator (written in Python) and support for the
modern CPLEX API (as defined in cplexx.h).
