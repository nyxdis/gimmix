ENV_CFLAGS=$CFLAGS

AC_PREREQ(2.59)
dnl Update it right before new releases
AC_INIT([Gimmix], 0.3.1, [priyankmg@gmail.com], gimmix)
AC_CONFIG_SRCDIR([src/gimmix.c])
AC_LANG(C)
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

dnl Host dependant flags
case "${host}" in
	*-*-cygwin*)
		ENV_CFLAGS="$ENV_CFLAGS -DCYGWIN"
		;;
esac

AC_PROG_CC
AC_HEADER_STDC
AC_CHECK_HEADERS([locale.h stdlib.h string.h sys/ioctl.h unistd.h])
AC_PROG_INSTALL

if test "${GCC}" = "yes"
then
	CFLAGS="${CFLAGS} -pipe"
	CFLAGS="${CFLAGS} -Wall"
fi

GTK_REQUIRED=2.6.0
GLADE_REQUIRED=2.6.0
MPD_REQUIRED=0.12.0
TAGLIB_REQUIRED=1.4

GETTEXT_PACKAGE="gimmix"
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

AC_MSG_CHECKING(for flying pigs)
AC_MSG_RESULT([ooh, there's one!])

PKG_CHECK_MODULES([GIMMIX], gtk+-2.0 >= $GTK_REQUIRED libglade-2.0 >= $GLADE_REQUIRED libmpd >= $MPD_REQUIRED taglib >= $TAGLIB_REQUIRED)

CFLAGS="${CFLAGS}"

dnl ==========================================================================

AC_OUTPUT([
src/Makefile
Makefile
])

echo "
gimmix $VERSION configuration summary:

        prefix                   : ${prefix}
        source code location     : ${srcdir}
        compiler                 : ${CC}
        compiler flags           : ${CFLAGS}
"
