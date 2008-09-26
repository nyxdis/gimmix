ENV_CFLAGS=$CFLAGS

AC_PREREQ(2.59)
dnl Update it right before new releases
AC_INIT([Gimmix], 0.5.0a, [priyankmg@gmail.com], gimmix)
AC_CONFIG_SRCDIR([src/gimmix.c])
AC_LANG(C)
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_SYSTEM
AC_PROG_INTLTOOL([0.21])
AC_PROG_LIBTOOL
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

AC_ARG_ENABLE(lyrics,
    AC_HELP_STRING([  --enable-lyrics], [Enable lyrics plugin]), [lyrics="yes"])
dnl Check for Lyrics plugin support
AC_MSG_CHECKING(whether lyrics plugin is enabled)
if test "x$lyrics" = "xyes" ; then
	enable_lyrics=yes
	AC_MSG_RESULT(yes)
	pkg_modules="nxml libcurl"
	CFLAGS="${CFLAGS} -DHAVE_LYRICS"
	AC_DEFINE(HAVE_LYRICS, 1, [building gimmix with lyrics support])
else
	pkg_modules=""
	enable_lyrics=no
	AC_MSG_RESULT(no)
fi
AC_SUBST([GFPM_LYRICS])
AM_CONDITIONAL(HAVE_LYRICS, test "x$enable_lyrics" = "xyes")

AC_ARG_ENABLE(cover,
    AC_HELP_STRING([  --enable-cover], [Enable Amazon cover fetcher plugin]), [cover="yes"])
dnl Check for Amazon cover fetcher plugin support
AC_MSG_CHECKING(whether amazon cover fetcher plugin is enabled)
if test "x$cover" = "xyes" ; then
	enable_cover=yes
	AC_MSG_RESULT(yes)
	if test "x$enable_lyrics" = "xno" ; then
		pkg_modules="nxml libcurl"
	fi
	CFLAGS="${CFLAGS} -DHAVE_COVER_PLUGIN"
	AC_DEFINE(HAVE_COVER_PLUGIN, 1, [building gimmix with cover plugin support])
else
	enable_cover=no
	AC_MSG_RESULT(no)
fi
AC_SUBST([GFPM_COVER])
AM_CONDITIONAL(HAVE_COVER_PLUGIN, test "x$enable_cover" = "xyes")


if test "${GCC}" = "yes"
then
	CFLAGS="${CFLAGS} -pipe"
	CFLAGS="${CFLAGS} -Wall"
fi

ALL_LINGUAS="de tr zh_TW"
GETTEXT_PACKAGE=gimmix
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [needed for bindtextdomain() and textdomain()])
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

AC_MSG_CHECKING(for flying pigs)
AC_MSG_RESULT([ooh, there's one!])

pkg_modules="${pkg_modules} gtk+-2.0 >= 2.6.0 \
			 libmpd >= 0.12.0 \
			 libglade-2.0 >= 2.6.0 \
			 taglib >= 1.4 \
			 gthread-2.0"
			 
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
	lyrics support		 : ${enable_lyrics}
	covers support		 : ${enable_cover}
"
