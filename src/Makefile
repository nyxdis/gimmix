# Makefile for gimmix
# gimmix-0.1beta2

# Installation directory
PREFIX=$(DESTDIR)/usr

ALLFILES=*.c

CONFUSE_LIB=$(shell pkg-config libconfuse --libs)
NOTIFY_CFLAGS=$(shell pkg-config libnotify --cflags)
NOTIFY_LIB=$(shell pkg-config libnotify --libs)
GLADE_CFLAGS=$(shell pkg-config libglade-2.0 --cflags)
GLADE_LIB=$(shell pkg-config libglade-2.0 --libs)
MPD_CFLAGS=$(shell pkg-config libmpd --cflags)
MPD_LIB=$(shell pkg-config libmpd --libs)

CFLAGS += -fPIC -DPREFIX=\"$(PREFIX)\" $(NOTIFY_CFLAGS) $(GLADE_CFLAGS) $(MPD_CFLAGS) -I.
LIBS=$(CONFUSE_LIB) $(NOTIFY_LIB) $(GLADE_LIB) $(MPD_LIB)

OBJS = config.o gimmix-core.o playlist.o interface.o gimmix.o

CC=gcc

projname := gimmix

all: gimmix

gimmix: $(OBJS)
	$(CC) $(OBJS) -o gimmix $(LIBS)

clean:
	rm -f *.o core *.so* *.bak *~ src/*~ gimmix

config.o:		src/config.c
	$(CC) $(CFLAGS) -c src/config.c
gimmix-core.o:		src/gimmix-core.c
	$(CC) $(CFLAGS) -c src/gimmix-core.c
playlist.o:		src/playlist.c
	$(CC) $(CFLAGS) -c src/playlist.c
interface.o:		src/interface.c
	$(CC) $(CFLAGS) -c src/interface.c
gimmix.o:		src/gimmix.c
	$(CC) $(CFLAGS) -c src/gimmix.c
	
install:
	install -d $(PREFIX)/share/gimmix
	install -D gimmix $(PREFIX)/bin/gimmix
	install -D -m644 pixmaps/gimmix.png $(PREFIX)/share/pixmaps/gimmix.png
	install -D -m644 glade/gimmix.glade $(PREFIX)/share/gimmix/gimmix.glade
