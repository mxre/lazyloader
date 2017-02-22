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

all: lib/libcplex.a

# Source of the CPLEX header definitions 
#CPLEX_PATH=/opt/ibm/ILOG/CPLEX_Studio127/cplex
CPLEX_PATH=${CPLEX_DIR}/cplex

# Possible Library names, to search for at runtime
LIBRARY_NAMES=cplex1270,cplex1263,cplex1262,cplex1261,cplex1260,cplex125,cplex124,cplex123

generated/%.h: $(CPLEX_PATH)/include/ilcplex/%.h filter_header.sh
	@test -d generated/ || mkdir generated
	./filter_header.sh $< > $@

generated/cplexx.h: generated/cplex.h generated/cplexl.h generated/cplexs.h
	cat $^ > $@

generated/loader_g.c: include/lazyloader.h stublib.py
	@test -d generated/ || mkdir generated
	python3 stublib.py -p -o $@ \
		-l $(LIBRARY_NAMES) \
		-e CPLEX_DLL

generated/%.c: generated/%.h generated/cpxconst.h stublib.py
	python3 stublib.py -i $< -o $(@D)
	@touch $@

generated/Makefile: generated.mk
	cp generated.mk generated/Makefile

generate: generated/loader_g.c generated/cplex.c generated/cplexs.c generated/cplexl.c src/loader.h generated/Makefile
	make -j20 -C generated all

lib/libcplex.a: generate
	make -C generated libcplex.a
	@test -d lib/ || mkdir lib
	mv generated/libcplex.a $@

clean:
	rm -fr lib/ generated/
