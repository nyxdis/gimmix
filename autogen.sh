#!/bin/sh -e

autopoint --force
autoreconf -fiv

if [ -z "${NOCONFIGURE}" ]; then
	./configure "$@"
fi
