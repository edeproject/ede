#!/bin/sh
# Create ~/Desktop/ directory if not exists and installs target file. If directory
# exists but is empty, files will be also installed. If directory is not empty, it will do nothing.

desktop_dir="$HOME/Desktop"
targets="ede-xterm.desktop"

[ -d $desktop_dir ] || mkdir -p $desktop_dir

curr_dir=`dirname $0`
content=`ls $desktop_dir/*.desktop 2>>/dev/null`

# check if directory is empty and install if it does
if test "x$content" = "x"; then
	echo "Preparing $desktop_dir for the first time..."

	for file in $targets; do
		cp $curr_dir/$file $desktop_dir
	done
fi
