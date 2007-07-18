#!/bin/sh
#
# $Id$
#
#= 
#= Usage: testpositions.sh [OPTIONS]
#=
#= Test position where will be placed newly created windows.
#= 
#= Options:
#=     -p [program] run program
#=     -t [number]  run [program] [number] times
#=     -h           show this help
#=
#= Example: testpositions.sh -e gvim -t 1000, which
#= will run 1000 instances of gvim.
#= 
#= NOTE: if you try this with some heavier programs
#= (mozilla, ooffice, etc.) swapping and possible X crashes
#= are not due window manager. Just warned you !
#= 

PROGRAM="xterm"
TIMES="10"

help()
{
	sed -ne "/^#= /{ s/^#= //p }" $0
	exit 0
}

main()
{
	if [ $# -eq 0 ]; then
		help
	fi

	argv=$@
	for argv do
		case $argv in
			-h)
			help
			continue;;
			-p)
			unset PROGRAM
			continue;;
			-t)
			unset TIMES
			continue;;
		esac

		if [ "$PROGRAM" = "" ];then
			PROGRAM=$argv
			continue
		elif [ "$TIMES" = "" ];then
			TIMES=$argv
		else
			echo "Bad parameter '$argv'."
			echo "Run $0 -h for options."
			exit 0
		fi
	done

	for((i = 1; i <= $TIMES; i++))
	do
		`$PROGRAM`&
	done
}

main $@
