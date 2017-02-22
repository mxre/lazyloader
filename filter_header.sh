#!/bin/sh
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

header=$1

gatename=_$(echo $(basename $header)|sed 's/\./_/g'|sed 's/\(.*\)/\U\1/g')

tmp=$(mktemp)
trap 'rm "$tmp"' EXIT

echo "#ifndef $gatename" >> $tmp
echo "#define $gatename" >> $tmp
echo "
/*
 * This header was partly generated from a solver header of the same name.
 * It contains only constants, structures, and macros generated from the
 * original header, and thus, contains no copyrightable information.
 */
 " >> $tmp
gcc -w -fpreprocessed -dD -E $1 |\
    sed '/^$/d;/CPXDEPRECATEDAPI *([0-9]\+)/d' |\
    grep -v '^#\s\+[0-9]\+\s\+".*"' |\
    head -n-1 |\
    tail -n+3 >> $tmp

echo "" >> $tmp
echo "#endif /* $gatename */" >> $tmp

mv $tmp "$2"

