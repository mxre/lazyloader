all: lazycplex.lib

lazycplex.c: cplexx.h
	python3 stublib.py -i cplexx.h -o $@ -e LOAD_CPLEX_DLL -l \
		cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123

lazycplex.obj: lazycplex.c cplexx.h cpxconst.h
	cl /c lazycplex.c /MD /Fo:$@ /Ox /I. /W2

lazycplex.lib: lazycplex.obj
	lib /out:$@ lazycplex.obj

clean:
	-del lazycplex.obj lazycplex.lib
