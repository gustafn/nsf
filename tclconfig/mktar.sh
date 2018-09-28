#!/bin/sh

pwd=`pwd`
name=`basename $(pwd)`
echo "name=$name"
OPTIONS=--disable-copyfile
#OPTIONS=
make distclean
cd ..
tar zcvf ./$name.tar.gz $OPTIONS \
    `find ./$name -type f -o -type l| fgrep -v .git| fgrep -v .dSYM | fgrep -v .DS_Store |\
        fgrep -v "~"| fgrep -v "#" | fgrep -v -- -orig | fgrep -v .junk |\
        fgrep -v .gcov |fgrep -v .gcda|fgrep -v .gcno|fgrep -v lcov-result|fgrep -v lcov.info |\
        egrep -v "doc/example.*[.]txt"| fgrep -v '.c-' | fgrep -v '.tcl-' |\
        fgrep -v .dylib | fgrep -v ._ | egrep -v ".o$" |\
        fgrep -v "autom4te"| fgrep -v config. | fgrep -v callgrind.out. | fgrep -v .plist |\
        fgrep -v .db | fgrep -v .gdb | fgrep -v .graffle`


