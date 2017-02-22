
INCLUDES = -I"$(CPLEX_STUDIO_DIR1263)/cplex/include" -I../include
LIBS = -LIBPATH:..\lib cplex.lib

CFLAGS = -MD -Ox -D_WIN32_WINNT=0x0601 -DBUILD_CPXSTATIC=1
LDFLAGS = -subsystem:console -nodefaultlib:libucrt.lib ucrt.lib

.PHONY: all

all: example.exe

example.obj: example.c
	$(CC) $(INCLUDES) $(CFLAGS) -c example.c

example.exe: example.obj
	link example.obj $(LIBS) -out:$@

clean:
	del /F example.exe example.obj

