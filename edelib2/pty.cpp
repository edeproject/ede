/*
 * $Id$
 *
 * edelib::PTY - A class for handling pseudoterminals (PTYs)
 * Adapted from KDE (kdelibs/kdesu/kdesu_pty.cpp) - original copyright message below
 *
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * This file contains code from TEShell.C of the KDE konsole.
 * Copyright (c) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>
 *
 * This is free software; you can use this library under the GNU Library
 * General Public License, version 2. See the file "COPYING.LIB" for the
 * exact licensing terms.
 *
 * pty.cpp: Access to PTY's on different systems a la UNIX98.
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* Needed for getpt, ptsname in glibc 2.1.x systems */
#endif

//#include <config.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#if defined(__osf__) || defined(__CYGWIN__)
#include <pty.h>
#endif

//#include <qglobal.h>

//#include <kdebug.h>
//#include <kstandarddirs.h>
#include "pty.h"

// FIXME: Blah defines
#define HAVE_GETPT
#define HAVE_PTSNAME
#define HAVE_GRANTPT
#define HAVE_UNLOCKPT

// stdlib.h is meant to declare the prototypes but doesn't :(
#ifndef __THROW
#define __THROW
#endif

#ifdef HAVE_GRANTPT
extern "C" int grantpt(int fd) __THROW;
#endif

#ifdef HAVE_PTSNAME
extern "C" char * ptsname(int fd) __THROW;
#endif

#ifdef HAVE_UNLOCKPT
extern "C" int unlockpt(int fd) __THROW;
#endif

#ifdef HAVE__GETPTY
extern "C" char *_getpty(int *, int, mode_t, int);
#endif

#ifdef HAVE__PTY_H
	#include <pty.h>
#endif

#include <termios.h>

#ifdef HAVE_LIBUTIL_H
	#include <libutil.h>
#elif defined(HAVE_UTIL_H)
	#include <util.h>
#endif


using namespace edelib;


void *qalloc(void *ptr, size_t size)
{
	if (ptr)
		return realloc (ptr, size);
	else
		return malloc (size);
}


PTY::PTY()
{
    ptyfd = -1;
    ptyname = ttyname = 0;
}

PTY::~PTY()
{
    if (ptyfd >= 0)
	close(ptyfd);
    if (ptyname) free (ptyname);
    if (ttyname) free (ttyname);
}


// Opens a pty master and returns its filedescriptor.

int PTY::getpt()
{

#if defined(HAVE_GETPT) && defined(HAVE_PTSNAME)

    // 1: UNIX98: preferred way
    ptyfd = ::getpt();
    ttyname = strdup(::ptsname(ptyfd));
    return ptyfd;

#elif defined(HAVE_OPENPTY)

#define MAXNAME 30
    
    // 2: BSD interface
    // More preferred than the linux hacks
    char name[MAXNAME];
    int master_fd, slave_fd;
    if (openpty(&master_fd, &slave_fd, name, 0L, 0L) != -1)  {
	ttyname = (char*)qalloc(ttyname,MAXNAME);
	strncpy(ttyname, name, MAXNAME);
	name[5]='p';
	ptyname = (char*)qalloc(ptyname,MAXNAME);
	strncpy(ptyname, name, MAXNAME);
        close(slave_fd); // We don't need this yet // Yes, we do.
	ptyfd = master_fd;
	return ptyfd;
    }
    ptyfd = -1;
//    kdDebug(900) << k_lineinfo << "Opening pty failed.\n";
    return -1;

#elif defined(HAVE__GETPTY)
    
    // 3: Irix interface
    int master_fd;
    char *tmp = _getpty(&master_fd,O_RDWR,0600,0);
    if (tmp) {
	ttyname = strdup(tmp);
	ptyfd = master_fd;
    } else {
	ptyfd = -1;
//	kdDebug(900) << k_lineinfo << "Opening pty failed.error" << errno << '\n';
    }
    return ptyfd;

#else

    // 4: Open terminal device directly
    // 4.1: Try /dev/ptmx first. (Linux w/ Unix98 PTYs, Solaris)

    ptyfd = open("/dev/ptmx", O_RDWR);
    if (ptyfd >= 0) {
	ptyname = strdup("/dev/ptmx");
#ifdef HAVE_PTSNAME
	ttyname = strdup(::ptsname(ptyfd));
	return ptyfd;
#elif defined (TIOCGPTN)
	int ptyno;
	if (ioctl(ptyfd, TIOCGPTN, &ptyno) == 0) {
	    ttyname = (char*)qalloc(ttyname,MAXNAME);
	    snprintf(ttyname,MAXNAME-1,"/dev/pts/%d", ptyno);
	    return ptyfd;
	}
#endif
	close(ptyfd);
    }

    // 4.2: Try /dev/pty[p-e][0-f] (Linux w/o UNIX98 PTY's)

    for (const char *c1 = "pqrstuvwxyzabcde"; *c1 != '\0'; c1++)
    {
	for (const char *c2 = "0123456789abcdef"; *c2 != '\0'; c2++)
	{
	    ptyname = (char*)qalloc(ptyname,strlen("/dev/pty12"));
	    ttyname = (char*)qalloc(ttyname,strlen("/dev/tty12"));
	    sprintf(ptyname, "/dev/pty%c%c", *c1, *c2);
	    sprintf(ttyname, "/dev/tty%c%c", *c1, *c2);
	    if (access(ptyname, F_OK) < 0)
		goto linux_out;
	    ptyfd = open(ptyname, O_RDWR);
	    if (ptyfd >= 0)
		return ptyfd;
	}
    }
linux_out:

    // 4.3: Try /dev/pty%d (SCO, Unixware)

    for (int i=0; i<256; i++)
    {
	ptyname = (char*)qalloc(ptyname,MAXNAME);
	ttyname = (char*)qalloc(ttyname,MAXNAME);
	snprintf(ptyname, MAXNAME-1, "/dev/ptyp%d", i);
	snprintf(ttyname, MAXNAME-1, "/dev/ttyp%d", i);
	if (access(ptyname, F_OK) < 0)
	    break;
	ptyfd = open(ptyname, O_RDWR);
	if (ptyfd >= 0)
	    return ptyfd;
    }


    // Other systems ??
    ptyfd = -1;
//    kdDebug(900) << k_lineinfo << "Unknown system or all methods failed.\n";
    return -1;

#endif // HAVE_GETPT && HAVE_PTSNAME

}


int PTY::grantpt()
{
    if (ptyfd < 0)
	return -1;

#ifdef HAVE_GRANTPT

    return ::grantpt(ptyfd);

#elif defined(HAVE_OPENPTY)

    // the BSD openpty() interface chowns the devices properly for us,
    // no need to do this at all
    return 0;

#else

    // konsole_grantpty only does /dev/pty??
    if (strncmp(ptyname, "/dev/pty", 8) != 0)
	return 0;

    fprintf (stderr, "Edelib: Pty: Your system doesn't have capabilities for PTYs and we don't implement them.");
    return -1;

    // Use konsole_grantpty:
//    if (KStandardDirs::findExe("konsole_grantpty").isEmpty())
//    {
//	kdError(900) << k_lineinfo << "konsole_grantpty not found.\n";
//	return -1;
//    }

    // As defined in konsole_grantpty.c
    const int pty_fileno = 3;

    pid_t pid;
    if ((pid = fork()) == -1)
    {
//	kdError(900) << k_lineinfo << "fork(): " << perror << "\n";
	return -1;
    }

    if (pid)
    {
	// Parent: wait for child
	int ret;
	waitpid(pid, &ret, 0);
	if (WIFEXITED(ret) && !WEXITSTATUS(ret))
	    return 0;
//	kdError(900) << k_lineinfo << "konsole_grantpty returned with error: "
//		     << WEXITSTATUS(ret) << "\n";
	return -1;
    } else
    {
	// Child: exec konsole_grantpty
	if (ptyfd != pty_fileno && dup2(ptyfd, pty_fileno) < 0)
	    _exit(1);
	execlp("konsole_grantpty", "konsole_grantpty", "--grant", (void *)0);
//	kdError(900) << k_lineinfo << "exec(): " << perror << "\n";
	_exit(1);
    }

    // shut up, gcc
    return 0;

#endif // HAVE_GRANTPT
}


/**
 * Unlock the pty. This allows connections on the slave side.
 */

int PTY::unlockpt()
{
    if (ptyfd < 0)
	return -1;

#ifdef HAVE_UNLOCKPT

    // (Linux w/ glibc 2.1, Solaris, ...)

    return ::unlockpt(ptyfd);

#elif defined(TIOCSPTLCK)

    // Unlock pty (Linux w/ UNIX98 PTY's & glibc 2.0)
    int flag = 0;
    return ioctl(ptyfd, TIOCSPTLCK, &flag);

#else

    // Other systems (Linux w/o UNIX98 PTY's, ...)
    return 0;

#endif

}


/**
 * Return the slave side name.
 */

const char *PTY::ptsname()
{
    if (ptyfd < 0)
	return 0;

    return ttyname;
}

