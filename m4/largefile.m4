dnl
dnl $Id$
dnl
dnl Copyright (c) 2009 edelib authors
dnl Copyright 1998-2007 by Bill Spitzak and others.
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this library. If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([EDE_LARGEFILE], [
	AC_ARG_ENABLE(largefile, [  --enable-largefile      enable large file support],,[enable_largefile=no])
	LARGEFILE=""

	if test "$enable_largefile" = yes; then
		LARGEFILE="-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE"
		if test "x$ac_cv_sys_large_files" = "x1"; then
			LARGEFILE="$LARGEFILE -D_LARGE_FILES"
		fi

		if test "x$ac_cv_sys_file_offset_bits" = "x64"; then
			LARGEFILE="$LARGEFILE -D_FILE_OFFSET_BITS=64"
		fi
	fi
])
