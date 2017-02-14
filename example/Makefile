
INCLUDES = -I/opt/ibm/ILOG/CPLEX_Studio127/cplex/include -I../include
LIBS = -L../lib -ldl -lcplex

.PHONY: all clean

all: example

example.o: example.c
	$(CC) $(INCLUDES) -c $< -o $@

example: example.o
	$(CC) $< $(LIBS) -o $@

clean:
	rm -f example
