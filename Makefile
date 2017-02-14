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

generated/cplexl.h: $(CPLEX_PATH)/include/ilcplex/cplexl.h filter_header.sh
	@test -d generated/ || mkdir generated
	./filter_header.sh $< > $@

generated/cplexs.h: $(CPLEX_PATH)/include/ilcplex/cplexs.h filter_header.sh
	@test -d generated/ || mkdir generated
	./filter_header.sh $< > $@

generated/cpxconst.h: $(CPLEX_PATH)/include/ilcplex/cpxconst.h filter_header.sh
	@test -d generated/ || mkdir generated
	./filter_header.sh $< > $@

generated/cplex.h: $(CPLEX_PATH)/include/ilcplex/cplex.h filter_header.sh
	@test -d generated/ || mkdir generated
	./filter_header.sh $< > $@

generated/cplexx.h: generated/cplex.h generated/cplexl.h generated/cplexs.h
	cat $^ > $@

generated/loader.c: include/lazyloader.h stublib.py
	@test -d generated/ || mkdir generated
	python3 stublib.py -p -o $@ \
		-l $(LIBRARY_NAMES) \
		-e LOAD_CPLEX_DLL

generated/cplexl.c: generated/cplexx.h generated/cpxconst.h stublib.py
	python3 stublib.py -i $< -o $@

generated/cplexs.c: generated/cplexx.h generated/cpxconst.h stublib.py
	python3 stublib.py -i $< -o $@

generated/cplex.c: generated/cplex.h generated/cpxconst.h stublib.py
	python3 stublib.py -i $< -o $@

objects/cplexs.o: generated/cplexs.c
	@test -d objects/ || mkdir objects
	gcc -Iinclude/ -I. -c $< -o $@ -O3 -fPIC

objects/cplexl.o: generated/cplexl.c
	@test -d objects/ || mkdir objects
	gcc -Iinclude/ -I. -c $< -o $@ -O3 -fPIC

objects/cplex.o: generated/cplex.c
	@test -d objects/ || mkdir objects
	gcc -Iinclude/ -I. -c $< -o $@ -O3 -fPIC

objects/loader.o: generated/loader.c
	@test -d objects/ || mkdir objects
	gcc -Iinclude/ -I. -c $< -o $@ -O3 -fPIC

lib/libcplex.a: objects/loader.o objects/cplex.o objects/cplexl.o objects/cplexs.o
	@test -d lib/ || mkdir lib
	ar rcs $@ $^

clean:
	rm -fr lib/ objects/ generated/
