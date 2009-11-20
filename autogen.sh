#!/bin/sh

# create aclocal.m4 file with content of given directory without 'aclocal' tool
aclocal_emulate() {
	dir="$1"
	filename="aclocal.m4"

	rm -f $filename

	for i in `ls $dir/*`; do
		echo "m4_include([$i])" >> $filename
	done

	echo " " >> $filename
}

if [ "$1" = "--compile" ]; then
	compile=1
fi

if aclocal_emulate m4 && autoheader && autoconf; then
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
