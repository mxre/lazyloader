
INCLUDES = -I/opt/ibm/ILOG/CPLEX_Studio127/cplex/include -I../include
LIBS = -L../lib -ldl -lcplex ../lib/cplex_auto.o

.PHONY: all clean

all: example

example.o: example.c
	$(CC) $(INCLUDES) -c $< -o $@

example: example.o
	$(CC) -fuse-ld=gold $< $(LIBS) -o $@

clean:
	rm -f example example.o

