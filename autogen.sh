#!/bin/sh -x

glib-gettextize --copy --force || exit 1
intltoolize --automake --copy --force || exit 1
aclocal || exit 1
autoconf || exit 1
autoheader || exit 1
automake --force-missing --add-missing --copy --foreign || exit 1
