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

dnl Some distributions split packages between STL runtime
dnl and development versions; this will ensure we have development packages
dnl Code is based on AC_CXX_HAVE_STL from Todd Veldhuizen and Luc Maisonobe
AC_DEFUN([EDE_CHECK_STL], [
	AC_MSG_CHECKING(for reasonable STL support)
	AC_LANG_CPLUSPLUS
	AC_TRY_COMPILE([
		#include <list>
		#include <deque>
		using namespace std;
	],[
		list<int> x; x.push_back(5);
		list<int>::iterator iter = x.begin(); if (iter != x.end()) ++iter; return 0;
	], ac_cv_cxx_have_stl=yes, ac_cv_cxx_have_stl=no)

	if eval "test $ac_cv_cxx_have_stl = no"; then
		AC_MSG_ERROR(You don't have STL (C++ standard library) packages installed, or your version of compiler is too old. Please fix this first)
	else
		AC_MSG_RESULT(yes)
	fi
])
