# lazycplex

The aim of this project is to provide an independent runtime for programs
using CPLEX. A program linked against this library, can load an appropriate
CPLEX library during runtime, as opposed during linktime (or program load,
when using the dynamic linker). Thus eliminating the need to link a static
version of CPLEX into the program, and therefore avoiding licensing issues.

## Example
See example/example.c

## Notes
This project is somewhat similar to
https://github.com/chmduquesne/lazylpsolverlibs
but, we use a faster generator (written in Python) and support for the
modern CPLEX API (as defined in cplexx.h).
