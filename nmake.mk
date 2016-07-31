all: lazycplex.lib

lazycplex.obj: lazycplex.c cplexx.h cpxconst.h
	cl /c lazycplex.c /MD /Fo:$@ /Ox /I. /W2

lazycplex.lib: lazycplex.obj
	lib /out:$@ lazycplex.obj

clean:
	rm -f lazycplex.obj lazycplex.lib 
