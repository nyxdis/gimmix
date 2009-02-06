INCLUDES=@GIMMIX_CFLAGS@ \
	-DPREFIX=\"$(prefix)\" \
	-fPIC

bin_PROGRAMS=gimmix

## Gimmix
gimmix_SOURCES=	wejpconfig.c \
		eggtrayicon.c \
		sexy-tooltip.c \
		gimmix-config.c \
		gimmix-core.c \
		gimmix-playlist.c \
		gimmix-tooltip.c \
		gimmix-systray.c \
		gimmix-prefs.c \
		gimmix-metadata.c \
		gimmix-interface.c \
		gimmix-firstrun.c \
		gimmix-tagedit.c \
		gimmix-lyrics.c \
		gimmix-covers.c \
		gimmix.c


gimmix_LDADD= @GIMMIX_LIBS@

