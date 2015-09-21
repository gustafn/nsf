#!/bin/sh

for pdir in `find . -name configure.ac`
do
  (cd `dirname $pdir`; autoconf)
done
