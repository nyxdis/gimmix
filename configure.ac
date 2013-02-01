AC_PREREQ(2.59)
AC_INIT([Gimmix], 0.6.0, [mende.christoph@gmail.com], gimmix)
AC_CONFIG_SRCDIR([src/gimmix.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-bzip2 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl i18n
IT_PROG_INTLTOOL([0.35.0])
GETTEXT_PACKAGE=gimmix
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

ALL_LINGUAS="cs de fr sv tr zh_TW"
AM_GLIB_GNU_GETTEXT()
AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)


AC_PROG_CC
AC_HEADER_STDC
AC_CHECK_HEADERS([locale.h stdlib.h string.h sys/ioctl.h unistd.h])


AC_ARG_ENABLE(	tageditor,
		AC_HELP_STRING([--disable-tageditor], [Disable id3 tag editing support]),
		[tageditor=${enableval}],
		[tageditor="yes"])
dnl Check for id3 tag editing support
AC_MSG_CHECKING(whether id3 tag editing support is enabled)
if test "$tageditor" = "yes"; then
	enable_tageditor=yes
	AC_MSG_RESULT(yes)
	pkg_modules="taglib_c >= 1.4"
	CFLAGS="${CFLAGS} -DHAVE_TAGEDITOR"
	AC_DEFINE(HAVE_TAGEDITOR, 1, [building gimmix with id3 tag editing support])
else
	pkg_modules=""
	enable_tageditor=no
	AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(HAVE_TAGEDITOR, test "x$enable_tageditor" = "xyes")

dnl Lyrics support
AC_ARG_ENABLE(	lyrics,
		AC_HELP_STRING([--disable-lyrics], [Disable lyrics plugin]),
		[lyrics=${enableval}],
		[lyrics="yes"])
dnl Check for Lyrics plugin support
AC_MSG_CHECKING(whether lyrics plugin is enabled)
if test "$lyrics" = "yes"; then
	enable_lyrics=yes
	AC_MSG_RESULT(yes)
	pkg_modules="${pkg_modules} nxml libcurl libxml-2.0"
	CFLAGS="${CFLAGS} -DHAVE_LYRICS"
	AC_DEFINE(HAVE_LYRICS, 1, [building gimmix with lyrics support])
else
	enable_lyrics=no
	AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(HAVE_LYRICS, test "x$enable_lyrics" = "xyes")

AC_ARG_ENABLE(	cover,
		AC_HELP_STRING([--disable-cover], [Disable Amazon cover fetcher plugin]),
		[cover=${enableval}],
		[cover="yes"])
dnl Check for Amazon cover fetcher plugin support
AC_MSG_CHECKING(whether amazon cover fetcher plugin is enabled)
if test "$cover" = "yes"; then
	enable_cover=yes
	AC_MSG_RESULT(yes)
	if test "x$enable_lyrics" = "xno" ; then
		pkg_modules="${pkg_modules} nxml libcurl libxml-2.0"
	fi
	CFLAGS="${CFLAGS} -DHAVE_COVER_PLUGIN"
	AC_DEFINE(HAVE_COVER_PLUGIN, 1, [building gimmix with cover plugin support])
else
	enable_cover=no
	AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(HAVE_COVER_PLUGIN, test "x$enable_cover" = "xyes")


AC_MSG_CHECKING(for flying pigs)
AC_MSG_RESULT([ooh, there is one!])

pkg_modules="${pkg_modules} \
	gtk+-2.0 >= 2.24 \
	libmpd >= 0.17.0 \
	gthread-2.0 \
	x11"

PKG_CHECK_MODULES([GIMMIX], [$pkg_modules])


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
	id3 tag editing support  : ${enable_tageditor}
"
