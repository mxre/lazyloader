#include <stdio.h>
#include "lazyloader.h"
#include <ilcplex/cplex.h>

int main(int argc, char* argv[])
{
    try_lazy_load("/opt/ibm/ILOG/CPLEX_Studio127/cplex/bin/x86-64_linux/libcplex1270.so", 1280000);
    int err = 0;
    CPXENVptr env = NULL;
    env = CPXopenCPLEX(&err);
    if (env == NULL) {
        return 0;
    }

    int version = 0;
    CPXversionnumber(env, &version);
    printf("%d.%d.%d.%d\n",
                    (version / 1000000),
                    (version / 10000  ) % 100,
                    (version / 100    ) % 100,
                    (version          ) % 100);
    CPXcloseCPLEX(&env);
}

