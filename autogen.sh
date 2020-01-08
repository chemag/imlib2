#!/bin/sh
# Run this to generate all the initial makefiles, etc.

rm -rf autom4te.cache aclocal.m4

autoreconf -vif

if [ -z "$NOCONFIGURE"  -a "$1" != "-n" ]; then
	./configure "$@"
fi
