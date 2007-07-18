dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl Check do we have jam installed and try to determine version
dnl where is minimal supported 2.3
AC_DEFUN([EDE_PROG_JAM], [
	AC_PATH_PROG(JAM, jam)

	if test -n "$JAM"; then
		AC_MSG_CHECKING([for jam version])

		echo "Echo \$(JAMVERSION) ;" > conftest.jam
		jam_version_orig=`$JAM -f conftest.jam | head -1`
		jam_version=`echo $jam_version_orig | sed -e 's/\.//g'`
		rm -f conftest.jam
		if test "$jam_version" -ge 23; then
			msg="$jam_version_orig (ok)"
			AC_MSG_RESULT($msg)
		else
			msg="jam version $jam_version_orig is too old. Download a newer version from our repository"
			AC_MSG_ERROR($msg)
		fi
	else
		AC_MSG_ERROR(Jam is missing! You can download it from our repository)
	fi
])
