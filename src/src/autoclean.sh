#!/bin/sh -xu

[ -f Makefile ] && make distclean
rm -rf autom4te.cache
rm -rf Makefile
rm -rf Makefile.in
rm -rf configure
rm -rf config.*
rm -rf stamp*
rm -rf depcomp
rm -rf install-sh
rm -rf missing
rm -rf src/Makefile
rm -rf src/Makefile.in
rm -rf aclocal.m4
rm -rf ltmain.sh
rm -rf compile
rm -rf libtool
rm -rf mkinstalldirs
rm -rf config.rpath
