dnl
dnl $Id$
dnl
dnl Part of edelib.
dnl Copyright (c) 2009 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

AC_DEFUN([EDE_CHECK_STATFS], [
	AC_MSG_CHECKING([for statfs])

	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE([
		#include <sys/vfs.h>
	],[
		struct statfs s;
		statfs("/", &s);
	],[have_statfs=yes],[have_statfs=no])
	AC_LANG_RESTORE

	if eval "test $have_statfs = yes"; then
		AC_DEFINE(HAVE_STATFS, 1, [Define to 1 if you have statfs])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
])
