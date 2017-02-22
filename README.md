# lazycplex

The aim of this project is to provide an independent runtime for programs
using CPLEX. A program linked against this library, can load an appropriate
CPLEX library during runtime, as opposed during linktime (or program load,
when using the dynamic linker). Thus eliminating the need to link a static
version of CPLEX into the program, and therefore avoiding licensing issues.

## Example
See example/example.c

## Notes
* The example uses automatic loading and detection.
  This means, the source code does not have any reference
  to the loader.
* The loader tries to find the library in the following paths:
  * A library provided in the environement variable `$CPLEX_DLL`,
    this has to be a absolute path to library.
  * Default library names, i.e. libcplex1270.so or cplex1270.dll
    in the system's default library search path.
  * A library in a default install path:
    - `%PROGRAMFILES%\IBM\ILOG\CPLEX_Studio*\cplex\bin\*\cplex????.dll` for Windows or
    - `/opt/ibm/ILOG/CPLEX_Studio*/cplex/bin/*/libcplex????.so` for Linux
* If the environment variable `$LAZYLOAD_DEBUG` is set, the library will print debug
  output to the console.
* The header file `lazyloader.h` contains functions, that can be used to programmatically
  set the loader library. Note that if automatic loading is enabled, in the library,
  another library can only be loaded, if the automatic loading failed.

## Acknowledgements
This project is somewhat similar to
https://github.com/chmduquesne/lazylpsolverlibs
but, we use a faster generator (written in Python) and support for the
modern CPLEX API (as defined in cplexx.h).
