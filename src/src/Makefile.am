INCLUDES=@GIMMIX_CFLAGS@ \
	-DPREFIX=\"$(prefix)\" \
	-fPIC

bin_PROGRAMS=gimmix

## Gimmix
gimmix_SOURCES=	wejpconfig.c \
		eggtrayicon.c \
		gimmix-config.c \
		gimmix-core.c \
		gimmix-playlist.c \
		gimmix-systray.c \
		gimmix-prefs.c \
		gimmix-interface.c \
		gimmix-firstrun.c \
		gimmix-tagedit.c \
		gimmix.c


gimmix_LDADD= @GIMMIX_LIBS@ -ltag_c

