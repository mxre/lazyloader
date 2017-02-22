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

## Todo
* Windows nmake needs a better detection for which object files need to be built.
  Currently this list is statically writen into the Makefile, and probably only valid
  for CPLEX 12.6.3

## Requirements
* For Linux nothing special is required, besides a CPLEX installation (we need the header files)
  to generate the source stubs and a recent C compiler (like GCC or clang) and Python 3
* For Windows we used Microsoft's Visual C Compiler 19 (Visual Studio 14),
  and Windows SDK 10. As a runtime library we default to using the Universal C Runtime,
  and a statically linked Visual Studio Core Runtime. (thus avoiding the need to provide a
  Visual Studio Redistributable CRT on a deployment system).
  Futhermore bash (i.e. from Windows Subsystem for Linux) with gcc and Python3 are required
  to generate the header stubs from a CPLEX installation.

Generated headers and source stubs are identical for Windows and Linux, and can be shared.
This is especially interresting, as genrerating them under Windows can be somewhat harder than
under Linux.

## Acknowledgements
This project is somewhat similar to
https://github.com/chmduquesne/lazylpsolverlibs
but, we use a faster generator (written in Python) and support for the
modern CPLEX API (as defined in cplexx.h).
