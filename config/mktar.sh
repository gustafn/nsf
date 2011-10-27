#!/bin/sh

pwd=`pwd`
name=`basename $(pwd)`
echo "name=$name"

make distclean
cd ..
tar zcvf ./$name.tar.gz \
    `find ./$name -type f -o -type l| fgrep -v .git| fgrep -v .dSYM | fgrep -v .DS_Store| \
    	fgrep -v "~"| fgrep -v "#" | fgrep -v -- -orig | fgrep -v .junk | \
    	egrep -v "doc/example.*[.]txt"| \
    	fgrep -v ".dylib"| \
	fgrep -v "autom4te"| fgrep -v config. | fgrep -v callgrind.out. | fgrep -v .plist |\
        fgrep -v .db | fgrep -v .gdb | fgrep -v .graffle`

