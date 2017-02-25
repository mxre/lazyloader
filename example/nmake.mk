
INCLUDES=-I"$(CPLEX_STUDIO_DIR1263)/cplex/include" -I../include
LIBS=-LIBPATH:..\lib cplex.lib

CFLAGS =-MT -Ox -D_WIN32_WINNT=0x0A00 -DBUILD_CPXSTATIC=1
LDFLAGS =-subsystem:console -nodefaultlib:libucrt.lib ucrt.lib

.PHONY: all
.SUFFIX:

all: example.exe

example.obj: example.c
	$(CC) -nologo $(INCLUDES) $(CFLAGS) -c example.c

example.exe: example.obj
	link -nologo $(LDFLAGS) example.obj $(LIBS) -out:$@

clean:
	del /F example.exe example.obj
