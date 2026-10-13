bin_PROGRAMS = gimmix

gimmix_SOURCES = wejpconfig.c wejpconfig.h		\
		 gimmix-config.c gimmix-config.h	\
		 gimmix-core.c gimmix-core.h		\
		 gimmix-playlist.c gimmix-playlist.h	\
		 gimmix-tooltip.c gimmix-tooltip.h	\
		 gimmix-systray.c gimmix-systray.h	\
		 gimmix-prefs.c gimmix-prefs.h		\
		 gimmix-metadata.c gimmix-metadata.h	\
		 gimmix-interface.c gimmix-interface.h	\
		 gimmix-firstrun.c gimmix-firstrun.h	\
		 gimmix-tagedit.c gimmix-tagedit.h	\
		 gimmix-http.c gimmix-http.h		\
		 gimmix-lyrics.c gimmix-lyrics.h	\
		 gimmix-covers.c gimmix-covers.h	\
		 gimmix.c gimmix.h

gimmix_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"	\
		  -DPIXMAPDIR=\"$(datadir)/pixmaps\"	\
		  -DLOCALEDIR=\"$(localedir)\"		\
		  $(FEATURE_CPPFLAGS)			\
		  $(GIMMIX_CFLAGS)			\
		  $(TAGLIB_CFLAGS)			\
		  $(PLUGINS_CFLAGS)

gimmix_LDADD = $(GIMMIX_LIBS) $(TAGLIB_LIBS) $(PLUGINS_LIBS) $(LIBINTL)
