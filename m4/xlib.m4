dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

AC_DEFUN([EDE_CHECK_X11], [
	dnl generic X11 checkers
	AC_PATH_X
	AC_PATH_XTRA
	if eval "test $ac_x_libraries = no" || eval "test $ac_x_includes = no"; then
		AC_MSG_ERROR([X11 libraries are not found! Please install them first])
	fi
])

AC_DEFUN([EDE_X11_SHAPE], [
	AC_ARG_ENABLE(shape, [  --enable-shape          enable XShape extension (default=yes)],,enable_shape=yes)

	dnl $X_LIBS contains path to X11 libs, since are not in path by default 
	if eval "test $enable_shape = yes"; then
		AC_CHECK_HEADER(X11/extensions/shape.h, [
			AC_CHECK_LIB(Xext, XShapeInputSelected, 
				AC_DEFINE(HAVE_SHAPE, 1, [Define to 1 if you have XShape extension]),,$X_LIBS)
		])
	fi
])
