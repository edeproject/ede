dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

AC_DEFUN([EDE_CHECK_FLTK], [
	dnl AC_MSG_NOTICE(whether is fltk 2.0 present)
	AC_PATH_PROG(FLTK2_CONFIG, fltk2-config)
	if test -n "$FLTK2_CONFIG"; then
		FLTKFLAGS=`fltk2-config --cxxflags`

		dnl remove -lsupc++ so we can chose what to use
		FLTKLIBS=`fltk2-config --use-images --ldflags | sed -e 's/-lsupc++//g'`
	else
		AC_MSG_ERROR([You don't have fltk2 installed. To compile EDE, you will need it.])
	fi
])


AC_DEFUN([EDE_CHECK_EFLTK], [
	dnl AC_MSG_NOTICE(whether is efltk present)
	AC_PATH_PROG(EFLTK_CONFIG, efltk-config)
	if test -n "$EFLTK_CONFIG"; then
		EFLTKFLAGS=`efltk-config --cxxflags`
		EFLTKLIBS=`efltk-config --use-images --ldflags`
	else
		AC_MSG_ERROR([You don't have efltk installed. To compile EDE, you will need it.])
	fi

	AC_MSG_CHECKING(efltk version >= 2.0.4)
	EFLTK_VERSION="`efltk-config --version`"
	case "$EFLTK_VERSION" in ["2.0."[56789]])
		dnl Display 'yes' for efltk version check
		AC_MSG_RESULT(yes)
		;;
		*)
		AC_MSG_ERROR([It seems that you have older efltk version. Required is >= 2.0.4])
	esac
])
