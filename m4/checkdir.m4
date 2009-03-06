dnl
dnl $Id$
dnl
dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl EDE_CHECK_DIR(VARIABLE, DIRECTORY_LIST)
AC_DEFUN(EDE_CHECK_DIR,
[AC_MSG_CHECKING(which directory to use for $1)
	AC_CACHE_VAL(ac_cv_dir_$1,
	[for ac_var in $2; do
		if test -d $ac_var; then
			ac_cv_dir_$1="$ac_var"
			break
		fi
	done
	])

	$1="$ac_cv_dir_$1"

	AC_SUBST($1)
	AC_MSG_RESULT($ac_cv_dir_$1)
])

