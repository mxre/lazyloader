all: liblazycplex.a

CPLEX_PATH=/opt/ibm/ILOG/CPLEX_Studio126/cplex

"cplexl.h": "${CPLEX_PATH}/include/ilcplex/cplexl.h"
	@cp ${CPLEX_PATH}/include/ilcplex/cpxconst.h include/
	./filter_header.sh $< > $@

lazycplex.c: cplexl.h stublib.sh
	./stublib.sh -i cplexl.h -l cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123 -e LAZYLOAD_CPLEX -r > $@

lazycplex.o: lazycplex.c
	gcc -c $< -o $@ -O3 -fPIC -I. -Wall

liblazycplex.a: lazycplex.o
	ar rcs $@ $<

clean:
	rm -f lazycplex.c liblazycplex.a test
