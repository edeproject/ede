dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl Check for presence of libmagic, since is used for mime types
dnl Note that libmagic requires libz to be linked against in this test
AC_DEFUN([EDE_CHECK_LIBMAGIC], [
	AC_CHECK_HEADER(magic.h, [
		AC_CHECK_LIB(magic, magic_open, AC_DEFINE(HAVE_LIBMAGIC, 1, [Define to 1 if you have libmagic library]),,
			-lz)
		])
])
