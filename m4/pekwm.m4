dnl
dnl $Id: xlib.m4 1856 2007-03-17 11:28:25Z karijes $
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) pekwm authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl all dependencies needed for pekwm
AC_DEFUN([EDE_CHECK_PEKWM_DEPENDENCIES], [
AC_LANG_SAVE
AC_LANG_CPLUSPLUS

dnl Stupid way to find iconv; on linux, iconv is part of glibc, for others
dnl try to search binary, then adjust paths
if test "x`uname`" = "xLinux"; then
 	LIBICONV=""
 	INCICONV=""
else
	iconv_path="`which iconv 2>/dev/null | sed 's/\/bin\/.*//g'`"
	if test "x$iconv_path" != "x"; then
		LIBICONV="-L$iconv_path/lib -liconv"
		INCICONV="-I$iconv_path/include"
		dnl dummy
		AC_DEFINE(ICONV_CONST, [1], [Define if iconv() have second parameter const])
	fi
fi
 
PEKWM_LIBS="$PEKWM_LIBS $LIBICONV"
PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $INCICONV"

dnl Check for iconvctl
AC_CHECK_FUNC(iconvctl, [AC_DEFINE(HAVE_ICONVCTL, [1], [Define to 1 if you the iconvctl call])], )

dnl Assume we have X libs
PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $X_CFLAGS"
PEKWM_LIBS="$PEKWM_LIBS -lX11 -lXext $X_LIBS $X_EXTRA_LIBS"

dnl Check for Xinerama support
AC_MSG_CHECKING([whether to build support for the Xinerama extension])
AC_CHECK_LIB(Xinerama, XineramaQueryScreens,
			AC_DEFINE(HAVE_XINERAMA, [1], [Define to 1 if you want Xinerama support to be])
			PEKWM_LIBS="$PEKWM_LIBS -lXinerama"
			PEKWM_FEATURES="$PEKWM_FEATURES Xinerama")

dnl Check for Xft support
AC_MSG_CHECKING([whether to support Xft fonts])
PKG_CHECK_MODULES([xft], [xft >= 2.0.0], HAVE_XFT=yes, HAVE_XFT=no)
if test "x$HAVE_XFT" = "xyes"; then
	AC_DEFINE(HAVE_XFT, [1], [Define to 1 if you want Xft2 font support])
	PEKWM_LIBS="$PEKWM_LIBS $xft_LIBS"
	PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $xft_CFLAGS"
	PEKWM_FEATURES="$PEKWM_FEATURES Xft";
else
	AC_MSG_WARN([Couldn't find Xft >= 2.0.0])
fi

if test "x$HAVE_LIBPNG" = "xyes"; then                
        THEME="default"
else
        THEME="default-plain"
fi
AC_SUBST([THEME])

dnl Check for XRANDR support
AC_MSG_CHECKING([wheter to build support for the XRANDR extension])
PKG_CHECK_MODULES([xrandr], [xrandr >= 1.2.0], HAVE_XRANDR=yes, HAVE_XRANDR=no)
if test "x$HAVE_XRANDR" = "xyes"; then
	AC_DEFINE(HAVE_XRANDR, [1], [Define to 1 if you have an XRANDR capable server])
	PEKWM_LIBS="$PEKWM_LIBS $xrandr_LIBS"
	PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $xrandr_CFLAGS"
	PEKWM_FEATURES="$PEKWM_FEATURES Xrandr"
else
	AC_MSG_WARN([Couldn't find Xrandr >= 1.2.0])
fi

dnl Check for header files
AC_STDC_HEADERS
AC_CHECK_HEADERS([limits slist ext/slist])

dnl Check that at least one of slist or ext/slist exists
if test "x$av_cv_header_slist" = "xno" \
   && test "x$av_cv_header_ext_slist" = "xno"; then
  AC_MSG_ERROR([Could not find slist or ext/slist include.])
fi

dnl Detect namespace slist is available in
AC_TRY_COMPILE([
#ifdef HAVE_SLIST
#include <slist>
#else // HAVE_EXT_SLIST
#include <ext/slist>
#endif // HAVE_SLIST
               ],[std::slist<int> test;],
               AC_DEFINE(SLIST_NAMESPACE, [std], [Name of namespace slist is in])
               slist_std_namespace=yes, [])

 if test "x$slist_std_namespace" != "xyes"; then
  AC_TRY_COMPILE([
#ifdef HAVE_SLIST
#include <slist>
#else // HAVE_EXT_SLIST
#include <ext/slist>
#endif // HAVE_SLIST
                 ],[__gnu_cxx::slist<int> test;],
                 AC_DEFINE(SLIST_NAMESPACE, [__gnu_cxx], [Name of namespace slist is in]), [])
fi

AC_CHECK_FUNC(setenv, [AC_DEFINE(HAVE_SETENV, [1], [Define to 1 if you the setenv systam call])], )
AC_CHECK_FUNC(unsetenv, [AC_DEFINE(HAVE_UNSETENV, [1], [Define to 1 if you the unsetenv systam call])], )
AC_CHECK_FUNC(swprintf, [AC_DEFINE(HAVE_SWPRINTF, [1], [Define to 1 if you have swprintf])], )

dnl Check whether time.h has timersub
AC_MSG_CHECKING(for timersub in time.h)
AC_TRY_LINK([#include <sys/time.h>],
            [struct timeval *a; timersub(a, a, a);],
            [have_timersub=yes],[])

if test "x$have_timersub" = "xyes"; then
	AC_MSG_RESULT(yes)
	AC_DEFINE([HAVE_TIMERSUB], 1, [Define to 1 if your system defines timersub.])
else
   AC_MSG_RESULT(no)
fi

dnl Check for XPM support
AC_MSG_CHECKING([wheter to build support XPM images])
AC_CHECK_LIB(Xpm, XpmReadFileToPixmap,
  AC_MSG_CHECKING([for X11/xpm.h])
    AC_TRY_COMPILE(
	#include <X11/xpm.h>
	, int foo = XpmSuccess,
	AC_MSG_RESULT([yes])
	AC_DEFINE(HAVE_IMAGE_XPM, [1], [Define to 1 if you libXpm])
	PEKWM_LIBS="$PEKWM_LIBS -lXpm"
	PEKWM_FEATURES="$PEKWM_FEATURES image-xpm",
AC_MSG_RESULT([no])))

dnl Check for JPEG support
AC_CHECK_LIB(jpeg, jpeg_read_header,
	AC_MSG_CHECKING([for jpeglib.h])
	AC_TRY_CPP([#include <jpeglib.h>], jpeg_ok=yes, jpeg_ok=no)
	AC_MSG_RESULT($jpeg_ok)
	if test "$jpeg_ok" = yes; then
		AC_DEFINE(HAVE_IMAGE_JPEG, [1], [Define to 1 if you have jpeg6b])
		PEKWM_LIBS="$PEKWM_LIBS -ljpeg"
		PEKWM_FEATURES="$PEKWM_FEATURES image-jpeg"
	fi,
AC_MSG_RESULT([no]))

dnl Check for PNG support
PKG_CHECK_MODULES([libpng12], [libpng12 >= 1.2.0], HAVE_LIBPNG=yes, HAVE_LIBPNG=no)
if test "x$HAVE_LIBPNG" = "xyes"; then
	AC_DEFINE(HAVE_IMAGE_PNG, [1], [Define to 1 if you have libpng12])
	PEKWM_LIBS="$PEKWM_LIBS $libpng12_LIBS"
	PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $libpng12_CFLAGS"
	PEKWM_FEATURES="$PEKWM_FEATURES image-png"
else
	PKG_CHECK_MODULES([libpng], [libpng >= 1.0.0], HAVE_LIBPNG=yes, HAVE_LIBPNG=no)
	if test "x$HAVE_LIBPNG" = "xyes"; then                
		AC_DEFINE(HAVE_IMAGE_PNG, [1], [Define to 1 if you have libpng12])
		PEKWM_LIBS="$PEKWM_LIBS $libpng_LIBS"
		PEKWM_CXXFLAGS="$PEKWM_CXXFLAGS $libpng_CFLAGS"
		PEKWM_FEATURES="$PEKWM_FEATURES image-png"
	else
		AC_MSG_RESULT([no])
	fi
fi

AC_DEFINE(OPACITY, [1], [Define to 1 to compile in support for opacity hinting])

EVO=`date`
AC_DEFINE_UNQUOTED(FEATURES, "$PEKWM_FEATURES", [Build info for pekwm, do not touch])
AC_DEFINE_UNQUOTED(EXTRA_VERSION_INFO, " Built on $EVO", [Build info for pekwm, do not touch])
AC_LANG_RESTORE
])
