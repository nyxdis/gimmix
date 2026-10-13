AC_PREREQ([2.69])
AC_INIT([gimmix], [0.6.0], [https://github.com/nyxdis/gimmix/issues], [gimmix],
	[https://github.com/nyxdis/gimmix])
AC_CONFIG_SRCDIR([src/gimmix.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2 subdir-objects -Wall])
AM_SILENT_RULES([yes])

AC_PROG_CC
PKG_PROG_PKG_CONFIG([0.24])

dnl i18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
GETTEXT_PACKAGE=gimmix
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])

dnl optional features; the defines are passed on the command line so that
dnl every header sees them without including config.h first
FEATURE_CPPFLAGS=""

AC_ARG_ENABLE([tageditor],
	[AS_HELP_STRING([--disable-tageditor], [disable tag editing support (needs taglib)])],
	[], [enable_tageditor=yes])
AS_IF([test "x$enable_tageditor" = "xyes"], [
	PKG_CHECK_MODULES([TAGLIB], [taglib_c >= 1.4])
	FEATURE_CPPFLAGS="$FEATURE_CPPFLAGS -DHAVE_TAGEDITOR"
])
AM_CONDITIONAL([HAVE_TAGEDITOR], [test "x$enable_tageditor" = "xyes"])

AC_ARG_ENABLE([lyrics],
	[AS_HELP_STRING([--disable-lyrics], [disable the lyrics plugin (LRCLIB)])],
	[], [enable_lyrics=yes])
AS_IF([test "x$enable_lyrics" = "xyes"], [
	FEATURE_CPPFLAGS="$FEATURE_CPPFLAGS -DHAVE_LYRICS"
])
AM_CONDITIONAL([HAVE_LYRICS], [test "x$enable_lyrics" = "xyes"])

AC_ARG_ENABLE([cover],
	[AS_HELP_STRING([--disable-cover], [disable the cover art plugin (Cover Art Archive)])],
	[], [enable_cover=yes])
AS_IF([test "x$enable_cover" = "xyes"], [
	FEATURE_CPPFLAGS="$FEATURE_CPPFLAGS -DHAVE_COVER_PLUGIN"
])
AM_CONDITIONAL([HAVE_COVER_PLUGIN], [test "x$enable_cover" = "xyes"])

dnl both plugins fetch over HTTP and parse JSON
AS_IF([test "x$enable_lyrics" = "xyes" -o "x$enable_cover" = "xyes"], [
	PKG_CHECK_MODULES([PLUGINS], [libcurl >= 7.19.4 json-glib-1.0 >= 1.2])
])
AC_SUBST([FEATURE_CPPFLAGS])

PKG_CHECK_MODULES([GIMMIX], [
	glib-2.0 >= 2.32
	gthread-2.0
	gtk+-3.0 >= 3.22
	libmpd >= 0.17.0
])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	po/Makefile.in
])
AC_OUTPUT

echo "
gimmix $VERSION configuration summary:

	prefix                   : ${prefix}
	compiler                 : ${CC}
	compiler flags           : ${CFLAGS}
	lyrics plugin            : ${enable_lyrics}
	cover art plugin         : ${enable_cover}
	tag editing support      : ${enable_tageditor}
"
