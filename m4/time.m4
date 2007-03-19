dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl Check for time functions since they are
dnl different between systems
AC_DEFUN([EDE_CHECK_TIME_FUNCS], [
	dnl glibc extension, not present on BSD's
	AC_CHECK_HEADER(time.h, [
		AC_CHECK_FUNCS(stime, AC_DEFINE(HAVE_STIME, 1, [Define to 1 if you have stime() in time.h]))
	])

	dnl rest should have this
	AC_CHECK_HEADER(sys/time.h, [
		AC_CHECK_FUNCS(gettimeofday, AC_DEFINE(HAVE_GETTIMEOFDAY, 1, [Define to 1 if you have gettimeofday() in sys/time.h]))
		AC_CHECK_FUNCS(settimeofday, AC_DEFINE(HAVE_SETTIMEOFDAY, 1, [Define to 1 if you have settimeofday() in sys/time.h]))
	])
])
