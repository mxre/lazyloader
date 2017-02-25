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
.SUFFIX:

all: lib/cplex.lib

CPLEX_PATH=$(CPLEX_STUDIO_DIR1263)/cplex

# Possible Library names, to search for at runtime
LIBRARY_NAMES=cplex1270,cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123

CFLAGS = -MT -Ox -D_WIN32_WINNT=0x0A00 -DAUTOMATIC_LOAD=1

generated/cplex.h: "$(CPLEX_PATH)/include/ilcplex/cplex.h" filter_header.sh
	@if not exist generated\ md generated > NUL
	@copy "$(CPLEX_PATH)\include\ilcplex\cplex.h" cplex.h > NUL
	bash ./filter_header.sh cplex.h $@
	@del cplex.h

generated/cplexs.h: "$(CPLEX_PATH)/include/ilcplex/cplexs.h" filter_header.sh
	@if not exist generated\ md generated > NUL
	@copy "$(CPLEX_PATH)\include\ilcplex\cplexs.h" cplexs.h > NUL
	bash ./filter_header.sh cplexs.h $@
	@del cplexs.h

generated/cplexl.h: "$(CPLEX_PATH)/include/ilcplex/cplexl.h" filter_header.sh
	@if not exist generated\ md generated > NUL
	@copy "$(CPLEX_PATH)\include\ilcplex\cplexl.h" cplexl.h > NUL
	bash ./filter_header.sh cplexl.h $@
	@del cplexl.h

generated/cpxconst.h: "$(CPLEX_PATH)/include/ilcplex/cpxconst.h" filter_header.sh
	@if not exist generated\ md generated > NUL
	@copy "$(CPLEX_PATH)\include\ilcplex\cpxconst.h" cpxconst.h > NUL
	bash ./filter_header.sh cpxconst.h $@
	@del cpxconst.h

generated/cplexx.h: generated/cplex.h generated/cplexs.h generated/cplexl.h
	@if not exist generated\ md generated > NUL
	type generated/cplex.h generated/cplexs.h generated/cplexl.h > $@

generated/loader_g.c: include/lazyloader.h stublib.py
	@if not exist generated\ md generated > NUL
	python stublib.py -p -o $@ \
		-l $(LIBRARY_NAMES) \
		-e CPLEX_DLL

generated/cplex.c: generated/cplex.h generated/cpxconst.h stublib.py
	python stublib.py -i generated/cplex.h -o generated -m
	@type NUL > $@

generated/cplexs.c: generated/cplexs.h generated/cpxconst.h stublib.py
	python stublib.py -i generated/cplexs.h -o generated -m
	@type NUL > $@

generated/cplexl.c: generated/cplexl.h generated/cpxconst.h stublib.py
	python stublib.py -i generated/cplexl.h -o generated -m
	@type NUL > $@

{generated}.c{generated}.obj::
	cl -Iinclude/ -I. -nologo $(CFLAGS) -Fo:generated\ -c $<

generated/loader_g.obj: generated/loader_g.c src/loader.c src/loader.h

generated/cplex.o: generated/cplex.c
	$(MAKE) /nologo /F generated/cplex.mk all
	@type NUL > $@

generated/cplexs.o: generated/cplexs.c
	$(MAKE) /nologo /F generated/cplexs.mk all
	@type NUL > $@

generated/cplexl.o: generated/cplexl.c
	$(MAKE) /nologo /F generated/cplexl.mk all
	@type NUL > $@

lib/cplex.lib: generated/cplex.o generated/cplexs.o generated/cplexl.o generated/loader_g.obj
	@if not exist lib\ md lib > NUL
	lib -nologo -out:$@ generated/*.obj

clean:
	-del /F /Q lib generated
