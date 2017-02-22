#include <stdio.h>
#include "lazyloader.h"
#include <ilcplex/cplexx.h>

int main(int argc, char* argv[])
{
    int err = 0;
#ifdef _WIN32
    err = try_lazy_load("C:\\Program Files\\IBM\\ILOG\\CPLEX_Studio1263\\cplex\\bin\\x64_win64\\cplex1263.dll", 12000000);
#else
    err = try_lazy_load("/opt/ibm/ILOG/CPLEX_Studio126/cplex/bin/x86-64_linux/libcplex1260.so", 12000000);
#endif
	if (err != 0) {
		fprintf(stderr, "Could not load library\n");
		return 0;
	}
    CPXENVptr env = NULL;
    env = CPXXopenCPLEX(&err);
    if (env == NULL) {
        return 0;
    }

    int version = 0;
    CPXXversionnumber(env, &version);
    printf("%d.%d.%d.%d\n",
                    (version / 1000000),
                    (version / 10000  ) % 100,
                    (version / 100    ) % 100,
                    (version          ) % 100);
    CPXXcloseCPLEX(&env);
}

