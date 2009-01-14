dnl
dnl $Id$
dnl
dnl Part of edelib.
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

AC_DEFUN([EDE_CHECK_LIBXPM], [
	AC_MSG_CHECKING([for libXpm])

	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE([
		#include <X11/xpm.h>
	],[
		Pixmap pix, mask;
		Display* display;
		Drawable d;
		XpmCreatePixmapFromData(display, d, 0, &pix, &mask, 0);
	],[have_libxpm=yes],[have_libxpm=no])
	AC_LANG_RESTORE

	if eval "test $have_libxpm = yes"; then
		AC_DEFINE(HAVE_LIBXPM, 1, [Define to 1 if you have libXpm])
		AC_MSG_RESULT(yes)
		LIBXPM_LIBS="-lXpm"
	else
		AC_MSG_RESULT(no)
	fi
])
