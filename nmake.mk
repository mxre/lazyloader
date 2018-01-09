#
# This file is part of lazycplex.
#
# Copyright 2016 Max Resch
#
# Permission is hereby granted, free of charge, to any
# person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal  in the
# Software without restriction, including without
# limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software
# is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice
# shall be included in all copies or substantial portions
# of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# see http://www.opensource.org/licenses/MIT
#

.PHONY: all

all: lib/cplex.lib lib/cplex_auto.obj

CPLEX_PATH=$(CPLEX_STUDIO_DIR1263)/cplex

# Possible Library names, to search for at runtime
LIBRARY_NAMES=cplex1270,cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123

CFLAGS=-MD -Ox -D_WIN32_WINNT=0x0601 -Iinclude/ -I. -I"$(CPLEX_PATH)/include/ilcplex"

generated\loader_g.c: include/lazyloader.h stublib.py
	python stublib.py -p -o $@ \
		-l $(LIBRARY_NAMES) \
		-e CPLEX_DLL

generated\cplex.sources: stublib.py
	python stublib.py -t \
		-i "$(CPLEX_PATH)/include/ilcplex/cplex.h" \
		-o generated
generated\cplexs.sources: stublib.py
	python stublib.py -t \
		-i "$(CPLEX_PATH)/include/ilcplex/cplexs.h" \
		-o generated
generated\cplexl.sources: stublib.py
	python stublib.py -t \
		-i "$(CPLEX_PATH)/include/ilcplex/cplexl.h" \
		-o generated

{generated}.c{generated}.obj::
	cl -nologo $(CFLAGS) -Fo:generated\ -c $<

generated\cplex.objects: generated/cplex.sources
	cl -nologo $(CFLAGS) -Fo:generated\ -c @$?
	@copy /b $@+,, $@ > NUL
generated\cplexl.objects: generated/cplexl.sources
	cl -nologo $(CFLAGS) -Fo:generated\ -c @$?
	@copy /b $@+,, $@ > NUL
generated\cplexs.objects: generated/cplexs.sources
	cl -nologo $(CFLAGS) -Fo:generated\ -c @$?
	@copy /b $@+,, $@ > NUL

generated\loader_g.obj: generated\loader_g.c src\loader.c src\loader.h include\lazyloader.h

generated\auto.c: src\auto.c
	copy $? $@

generated\auto.obj: generated\auto.c include\lazyloader.h

lib\cplex.lib: generated\cplex.objects generated\cplexs.objects generated/cplexl.objects generated\loader_g.obj
	lib -nologo -out:$@ @generated\cplex.objects @generated\cplexs.objects @generated\cplexl.objects generated\loader_g.obj

lib\cplex_auto.obj: generated\auto.obj
	copy $? $@

clean:
	-del /F /Q lib generated
