#!/bin/sh -x

glib-gettextize --copy --force
intltoolize --automake --copy --force
aclocal
autoconf
autoheader
automake --force-missing --add-missing --copy --foreign
