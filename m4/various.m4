dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl --enable-debug and --enable-profile options
AC_DEFUN([EDE_DEVELOPMENT], [
	dnl clear all optimization flags
	OPTIMFLAGS=""

	AC_ARG_ENABLE(debug, [  --enable-debug          enable debug (default=no)],,enable_debug=no)
	if eval "test $enable_debug = yes"; then
		DEBUGFLAGS="$DEBUGFLAGS -g3"
	fi

	AC_ARG_ENABLE(profile, [  --enable-profile        enable profile (default=no)],,enable_profile=no)
	if eval "test $enable_profile = yes"; then
		DEBUGFLAGS="$DEBUGFLAGS -pg"
	fi

	AC_ARG_ENABLE(pedantic, [  --enable-pedantic       enable pedantic (default=no)],,enable_pedantic=no)
	if eval "test $enable_pedantic = yes"; then
		DEBUGFLAGS="$DEBUGFLAGS -pedantic"
	fi
])
