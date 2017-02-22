#include <stdio.h>
#include <ilcplex/cplexx.h>

int main(int argc, char* argv[])
{
    CPXENVptr env = NULL;
    int err = 0;
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

