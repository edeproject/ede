dnl
dnl $Id$
dnl
dnl Part of Equinox Desktop Environment (EDE).
dnl Copyright (c) 2000-2007 EDE Authors.
dnl 
dnl This program is licenced under terms of the 
dnl GNU General Public Licence version 2 or newer.
dnl See COPYING for details.

dnl Enable sound param and check for facility
dnl For now only .ogg via libao
AC_DEFUN([EDE_SOUND], [

	AC_ARG_ENABLE(sounds, [  --enable-sounds         enable sounds (default=yes)],, enable_sounds=yes)

	if eval "test $enable_sounds = yes"; then
	   AC_CHECK_HEADER(ao/ao.h, [have_ao_h=yes], [have_ao_h=no])
	   AC_CHECK_LIB(ao, ao_is_big_endian, [have_ao_lib=yes], [have_ao_lib=no])
   
	   AC_CHECK_HEADER(vorbis/codec.h, [have_codec_h=yes], [have_codec_h=no])
	   AC_CHECK_LIB(vorbis, vorbis_info_init, [have_vorbis_lib=yes], [have_vorbis_lib=no])

	   AC_CHECK_HEADER(vorbis/vorbisfile.h, [have_vorbisfile_h=yes], [have_vorbisfile_h=no])
	   AC_CHECK_LIB(vorbisfile, ov_clear, [have_vorbisfile_lib=yes], [have_vorbisfile_lib=no])

	   AC_MSG_CHECKING(sound support)
	   if eval "test $have_ao_h = yes" && \
		  eval "test $have_codec_h = yes" && \
		  eval "test $have_vorbisfile_h = yes"; then
		  	AC_MSG_RESULT(ok)
	   	 	SOUNDFLAGS="-DSOUND"
	   	 	SOUNDLIBS="-lao -lvorbis -lvorbisfile"
		else
			AC_MSG_RESULT(disabled)
		fi
   	fi
])

dnl Check if we have alsa installed since evolume rely on it
dnl Here are two parameters that are set:
dnl  - HAVE_ALSA used in edeconf.h
dnl  - MAKE_EVOLUME used by jam
AC_DEFUN([EDE_CHECK_ALSA], [
	AC_CHECK_HEADER(linux/soundcard.h, AC_DEFINE(HAVE_ALSA, 1, [Define to 1 if you have alsa libraries]))

	if eval "test $ac_cv_header_linux_soundcard_h = yes"; then
		MAKE_EVOLUME="1"
	else
		echo
		echo "***************************************"
		echo "*         ALSA WAS NOT FOUND          *"
		echo "*                                     *"
		echo "* Sadly, evolume is ALSA-only at this *"
		echo "* moment. It will be disabled.        *"
		echo "***************************************"
		MAKE_EVOLUME="0"
	fi
])
