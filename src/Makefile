# Makefile for gimmix
# gimmix-0.2.1-RC1

# Installation directory
PREFIX=$(DESTDIR)/usr

ALLFILES=*.c

GLADE_CFLAGS=$(shell pkg-config libglade-2.0 --cflags)
GLADE_LIB=$(shell pkg-config libglade-2.0 --libs)
MPD_CFLAGS=$(shell pkg-config libmpd --cflags)
MPD_LIB=$(shell pkg-config libmpd --libs)
TAG_CFLAGS=$(shell pkg-config taglib --cflags)
#TAG_LIB=$(shell pkg-config taglib --libs)

CFLAGS += -fPIC -DPREFIX=\"$(PREFIX)\" $(GLADE_CFLAGS) $(MPD_CFLAGS) $(TAG_CFLAGS) -I.
LIBS= $(GLADE_LIB) $(MPD_LIB) -ltag_c

OBJS = wejpconfig.o gimmix-config.o gimmix-core.o gimmix-playlist.o gimmix-systray.o gimmix-prefs.o gimmix-interface.o gimmix-firstrun.o gimmix-tagedit.o gimmix.o

CC=gcc

projname := gimmix

all: gimmix

gimmix: $(OBJS)
	$(CC) $(OBJS) -o gimmix $(LIBS)

clean:
	rm -f *.o core *.so* *.bak *~ src/*~ gimmix

wejpconfig.o:			src/wejpconfig.c
	$(CC) $(CFLAGS) -c src/wejpconfig.c

gimmix-config.o:		src/gimmix-config.c
	$(CC) $(CFLAGS) -c src/gimmix-config.c

gimmix-core.o:			src/gimmix-core.c
	$(CC) $(CFLAGS) -c src/gimmix-core.c

gimmix-firstrun.o:		src/gimmix-firstrun.c
	$(CC) $(CFLAGS) -c src/gimmix-firstrun.c

gimmix-playlist.o:		src/gimmix-playlist.c
	$(CC) $(CFLAGS) -c src/gimmix-playlist.c

gimmix-systray.o:		src/gimmix-systray.c
	$(CC) $(CFLAGS) -c src/gimmix-systray.c
	
gimmix-prefs.o:			src/gimmix-prefs.c
	$(CC) $(CFLAGS) -c src/gimmix-prefs.c

gimmix-interface.o:		src/gimmix-interface.c
	$(CC) $(CFLAGS) -c src/gimmix-interface.c

gimmix-tagedit.o:		src/gimmix-tagedit.c
	$(CC) $(CFLAGS) -c src/gimmix-tagedit.c

gimmix.o:				src/gimmix.c
	$(CC) $(CFLAGS) -c src/gimmix.c


install: all
	install -d $(PREFIX)/share/gimmix
	install -D gimmix $(PREFIX)/bin/gimmix
	install -D -m644 data/gimmix.png $(PREFIX)/share/pixmaps/gimmix.png
	install -D -m644 data/gimmix_logo_small.png $(PREFIX)/share/pixmaps/gimmix_logo_small.png
	install -D -m644 data/gimmix_firstrun.png $(PREFIX)/share/gimmix/gimmix_firstrun.png
	install -D -m644 data/gimmix.desktop $(PREFIX)/share/applications/gimmix.desktop
	install -D -m644 glade/gimmix.glade $(PREFIX)/share/gimmix/gimmix.glade

