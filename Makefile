all: liblazycplex.a

CPLEX_PATH=/opt/ibm/ILOG/CPLEX_Studio1263/cplex

cplexl.h: ${CPLEX_PATH}/include/ilcplex/cplexl.h filter_header.sh
	./filter_header.sh $< > $@

cpxconst.h: ${CPLEX_PATH}/include/ilcplex/cpxconst.h filter_header.sh
	./filter_header.sh $< > $@
	
cplex.h: ${CPLEX_PATH}/include/ilcplex/cplex.h filter_header.sh
	./filter_header.sh $< > $@
	
cplexx.h: cplex.h cplexl.h
	cat $^ > $@

lazycplex.c: cplexx.h
	python3 stublib.py -i cplexx.h -o $@ \
		-l cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123 \
		-e LAZYLOAD_CPLEX_DLL

lazycplex.o: lazycplex.c cplexx.h cpxconst.h
	gcc -c $< -o $@ -O3 -fPIC -I. -Wall

liblazycplex.a: lazycplex.o
	ar rcs $@ $^

clean:
	rm -f lazycplex.c liblazycplex.a test
