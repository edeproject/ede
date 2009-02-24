#!/bin/sh

# to save me from typing :P
if [ "$1" = "--compile" ]; then
	compile=1
fi

if aclocal -I m4 && autoheader && autoconf; then
	echo ""
	echo "Now run ./configure [OPTIONS]"
	echo "or './configure --help' to see them"

	if [ $compile ]; then
		./configure --enable-debug --prefix=/opt/ede && jam
	fi
else
	echo ""
	echo "We failed :(. There should be some output, right?"
	exit 1
fi
