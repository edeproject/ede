dnl
dnl $Id$
dnl
dnl Part of edelib.
dnl Copyright (c) 2009 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

AC_DEFUN([EDE_CHECK_STAT64], [
	AC_MSG_CHECKING([for stat64])

	AC_LANG_SAVE
	AC_LANG_C
	AC_TRY_COMPILE([
		/* C++ already defines this, but C does not and 'struct stat64' will not be seen */
		#define _LARGEFILE64_SOURCE 1
		#include <sys/stat.h>
	],[

		struct stat64 s;
		stat64("/", &s);
	],[have_stat64=yes],[have_stat64=no])
	AC_LANG_RESTORE

	if eval "test $have_stat64 = yes"; then
		AC_DEFINE(HAVE_STAT64, 1, [Define to 1 if you have stat64])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
])
