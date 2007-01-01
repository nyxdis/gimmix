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
AC_PROG_LN_S
AC_PROG_INTLTOOL(, [no-xml])

if test "${GCC}" = "yes"
then
	CFLAGS="${CFLAGS} -pipe"
	CFLAGS="${CFLAGS} -Wall"
fi

ALL_LINGUAS="zh_TW"
GETTEXT_PACKAGE=gimmix
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [needed for bindtextdomain() and textdomain()])
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

AC_MSG_CHECKING(for flying pigs)
AC_MSG_RESULT([ooh, there's one!])
pkg_modules="gtk+-2.0 >= 2.6.0 \
			 libmpd >= 0.12.0 \
			 libglade-2.0 >= 2.6.0 \
			 taglib >= 1.4"
PKG_CHECK_MODULES([GIMMIX], [$pkg_modules])

CFLAGS="${CFLAGS}"

dnl ==========================================================================

AC_OUTPUT([
src/Makefile
po/Makefile.in
Makefile
])

echo "
gimmix $VERSION configuration summary:

        prefix                   : ${prefix}
        source code location     : ${srcdir}
        compiler                 : ${CC}
        compiler flags           : ${CFLAGS}
"
