/*
 * $Id$
 *
 * edelib::PtyProcess - This class enables us to "chat" with terminal programs synchronously
 * Adapted from KDE (kdelibs/kdesu/process.cpp) - original copyright message below
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
 * Id: process.cpp 439322 2005-07-27 18:49:23Z coolo 
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
 * process.cpp: Functionality to build a front end to password asking
 *  terminal programs.
 */

//#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>

#if defined(__SVR4) && defined(sun)
#include <stropts.h>
#include <sys/stream.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>                // Needed on some systems.
#endif

//#include <qglobal.h>
//#include <qfile.h>

//#include <kdebug.h>
//#include <kstandarddirs.h>

#include "process.h"
#include "pty.h"
//#include "kcookie.h"
#include "NLS.h"

using namespace edelib;


int strpos(const char *string, char c)
{
	for (uint i=0;i<strlen(string);i++)
		if (string[i] == c) return i;
	return -1;
}

int PtyProcess::waitMS(int fd,int ms)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000*ms;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	return select(fd+1, &fds, 0L, 0L, &tv);
}

/*
** Basic check for the existence of @p pid.
** Returns true iff @p pid is an extant process.
*/
bool PtyProcess::checkPid(pid_t pid)
{
	return kill(pid,0) == 0;
}

/*
** Check process exit status for process @p pid.
** On error (no child, no exit), return Error (-1).
** If child @p pid has exited, return its exit status,
** (which may be zero).
** If child @p has not exited, return NotExited (-2).
*/

int PtyProcess::checkPidExited(pid_t pid)
{
	int state, ret;
	ret = waitpid(pid, &state, WNOHANG);

	if (ret < 0)
	{
//		kdError(900) << k_lineinfo << "waitpid(): " << perror << "\n";
		return Error;
	}
	if (ret == pid)
	{
		if (WIFEXITED(state))
			return WEXITSTATUS(state);
		if (WIFSIGNALED(state) && WTERMSIG(state)==SIGSEGV)
			return Crashed;
		return Killed;
	}

	return NotExited;
}


class PtyProcess::PtyProcessPrivate
{
public:
	char **env;
	~PtyProcessPrivate() {
		int i=0;
		if (env)
			while (env[i] != NULL)
				free(env[i++]);
	};
};


PtyProcess::PtyProcess()
{
    m_bTerminal = false;
    m_bErase = false;
    m_pPTY = 0L;
    d = new PtyProcessPrivate;
    d->env = 0;
    m_Pid = 0;
    m_Inbuf = m_TTY = m_Exit = m_Command = 0;
}


int PtyProcess::init()
{
    delete m_pPTY;
    m_pPTY = new PTY();
    m_Fd = m_pPTY->getpt();
    if (m_Fd < 0)
        return -1;
    if ((m_pPTY->grantpt() < 0) || (m_pPTY->unlockpt() < 0))
    {
        fprintf(stderr, "Edelib: PtyProcess: Master setup failed.\n");
        m_Fd = -1;
        return -1;
    }
    if (m_TTY) free(m_TTY);
    m_TTY = strdup(m_pPTY->ptsname());
//    m_Inbuf.resize(0);
    if (m_Inbuf) free(m_Inbuf);
    m_Inbuf = 0;
    return 0;
}


PtyProcess::~PtyProcess()
{
    if (m_TTY) free(m_TTY);
    if (m_Inbuf) free(m_Inbuf);
    delete m_pPTY;
    delete d;
}

/** Set additinal environment variables. */
void PtyProcess::setEnvironment( const char **env )
{
	// deallocate old environment store
	int i=0;
	if (d->env)
		while (d->env[i] != NULL)
			free(d->env[i++]);
	
	// count number of environment variables
	int n_env=0;
	while (env[n_env++] != NULL);
	d->env = (char**)malloc((n_env+2)*sizeof(char *));
	
	// copy env to d->env
	i=0;
	while (env[i] != NULL) {
		d->env[i] = strdup(env[i]); 
		i++; // gcc insists that strdup(env[i++]) above would be ambiguous...
	}
	d->env[i] = NULL;
}

char **PtyProcess::environment() const
{
    return d->env;
}

/*
 * Read one line of input. The terminal is in canonical mode, so you always
 * read a line at at time
 */

char *PtyProcess::readLine(bool block)
{
    int pos;
    char *ret = 0;

    if (m_Inbuf && strlen(m_Inbuf)>0)
    {
        pos = strpos(m_Inbuf,'\n');
        if (pos == -1)
        {
            ret = strdup(m_Inbuf);
	    free(m_Inbuf);
	    m_Inbuf = 0;
        } else
        {
	    // ret = part of m_Inbuf before \n
	    // m_Inbuf = part of m_Inbuf after \n
	    ret = strdup(m_Inbuf);
	    free(m_Inbuf);
	    m_Inbuf = strdup(ret + pos + 1);
	    ret[pos+1] = '\0';
        }
	return ret;
    }

    int flags = fcntl(m_Fd, F_GETFL);
    if (flags < 0)
    {
//        kdError(900) << k_lineinfo << "fcntl(F_GETFL): " << perror << "\n";
	fprintf (stderr, "Edelib: PtyProcess: fcntl not working - %d\n", errno);
        return ret;
    }
    int oflags = flags;
    if (block)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    if ((flags != oflags) && (fcntl(m_Fd, F_SETFL, flags) < 0))
    {
       // We get an error here when the child process has closed 
       // the file descriptor already.
       return ret;
    }

    int nbytes;
    char buf[256];
    while (1)
    {
        nbytes = read(m_Fd, buf, 255);
        if (nbytes == -1)
        {
            if (errno == EINTR)
                continue;
            else break; 
        }
        if (nbytes == 0)
            break;        // eof

        buf[nbytes] = '\000';
	if (m_Inbuf) 
		m_Inbuf = (char*)realloc(m_Inbuf, strlen(m_Inbuf)+nbytes+1);
	else {
		m_Inbuf = (char*)malloc(nbytes+1);
		m_Inbuf[0] = 0;
	}
        strcat(m_Inbuf, buf);

        ret = strdup(m_Inbuf);
	// only one line...
        pos = strpos(ret,'\n');
        if (pos != -1) { 
		free (m_Inbuf);
		m_Inbuf = strdup(ret + pos + 1);
		ret[pos+1] = '\0';
	}
        break;
    }

    return ret;
}


void PtyProcess::writeLine(const char *line, bool addnl)
{
    if (line && strlen(line)>0)
        write(m_Fd, line, strlen(line));
    if (addnl)
        write(m_Fd, "\n", 1);
}


void PtyProcess::unreadLine(const char *line, bool addnl)
{
    char *tmp = (char*) malloc(strlen(line)+1);
    strcpy(tmp,line);
    if (addnl)
        strcat(tmp, "\n");

    if (m_Inbuf) {
	char *tmp2 = (char*)malloc(strlen(m_Inbuf)+strlen(tmp)+1);
	strcpy(tmp2,tmp);
	strcat(tmp2,m_Inbuf);
	free(m_Inbuf);
	m_Inbuf=tmp2;
	free(tmp);
    } else 
	m_Inbuf = tmp;
}

/*
 * Fork and execute the command. This returns in the parent.
 */

int PtyProcess::exec(const char *command, const char **args)
{
    fprintf(stderr, "Edelib: PtyProcess: Running `%s'\n", command);
    int i;

    if (init() < 0)
        return -1;

    // Open the pty slave before forking. See SetupTTY()
    fprintf (stderr, "pty: %s\n", m_TTY);
    int slave = open(m_TTY, O_RDWR);
    if (slave < 0)
    {
        fprintf(stderr, "Edelib: PtyProcess: Could not open slave pty.\n");
        return -1;
    }

    if ((m_Pid = fork()) == -1)
    {
        fprintf(stderr, "Edelib: PtyProcess: fork(): %s\n", strerror(errno));
        return -1;
    }

    // Parent
    if (m_Pid)
    {
        close(slave);
        return 0;
    }

    // Child
    if (SetupTTY(slave) < 0) {
        _exit(1);
    }

    i=0; 
    while (d->env[i] != NULL)
        putenv(d->env[i++]);
//    unsetenv("KDE_FULL_SESSION");

    // From now on, terminal output goes through the tty.

    const char *path;
//    if (strchr(command,'/'))
        path = command;
/*	VEDRAN: This is now handled elsewhere - fully qualified path
	*must* be provided*/
//    else
//    {
//        QString file = KStandardDirs::findExe(command);
//        if (file.isEmpty())
//        {
//            kdError(900) << k_lineinfo << command << " not found\n";
//            _exit(1);
//        }
//        path = QFile::encodeName(file);
//    }

//    const char **argp = (const char **)malloc((args.count()+2)*sizeof(char *));
/*    const char **cptr = args;
    int count=0;
    while (cptr++)
        count++;
    fprintf(stderr, "G\n");
    const char **argp  = (const char **)malloc((count+2)*sizeof(char *));
    fprintf(stderr, "H\n");

    i = 0;
//    argp[i++] = strdup(path);
    cptr = args;
    int j=0;
    while (cptr[j])
        argp[i++] = strdup(cptr[j++]);
//    for (QList<QByteArray>::ConstIterator it=args.begin(); it!=args.end(); ++it)
//        argp[i++] = *it;

    argp[i + 2] = 0;*/

    execv(path, const_cast<char **>(args));
    _exit(1);
    return -1; // Shut up compiler. Never reached.
}


/*
 * Wait until the terminal is set into no echo mode. At least one su
 * (RH6 w/ Linux-PAM patches) sets noecho mode AFTER writing the Password:
 * prompt, using TCSAFLUSH. This flushes the terminal I/O queues, possibly
 * taking the password  with it. So we wait until no echo mode is set
 * before writing the password.
 * Note that this is done on the slave fd. While Linux allows tcgetattr() on
 * the master side, Solaris doesn't.
 */

int PtyProcess::WaitSlave()
{
    int slave = open(m_TTY, O_RDWR);
    if (slave < 0)
    {
//        kdError(900) << k_lineinfo << "Could not open slave tty.\n";
        return -1;
    }

//    kdDebug(900) << k_lineinfo << "Child pid " << m_Pid << endl;

    struct termios tio;
    while (1)
    {
	if (!checkPid(m_Pid))
	{
		close(slave);
		return -1;
	}
        if (tcgetattr(slave, &tio) < 0)
        {
//            kdError(900) << k_lineinfo << "tcgetattr(): " << perror << "\n";
            close(slave);
            return -1;
        }
        if (tio.c_lflag & ECHO)
        {
//            kdDebug(900) << k_lineinfo << "Echo mode still on.\n";
	    waitMS(slave,100);
            continue;
        }
        break;
    }
    close(slave);
    return 0;
}


int PtyProcess::enableLocalEcho(bool enable)
{
    int slave = open(m_TTY, O_RDWR);
    if (slave < 0)
    {
//        kdError(900) << k_lineinfo << "Could not open slave tty.\n";
        return -1;
    }
    struct termios tio;
    if (tcgetattr(slave, &tio) < 0)
    {
//        kdError(900) << k_lineinfo << "tcgetattr(): " << perror << "\n";
        close(slave); return -1;
    }
    if (enable)
        tio.c_lflag |= ECHO;
    else
        tio.c_lflag &= ~ECHO;
    if (tcsetattr(slave, TCSANOW, &tio) < 0)
    {
//        kdError(900) << k_lineinfo << "tcsetattr(): " << perror << "\n";
        close(slave); return -1;
    }
    close(slave);
    return 0;
}


// runChild() -- added by Vedran
// This routine will execute child process capturing all output
//
// Rationale:
// Even though most users today use window managers to run programs and not
// xterms, many XWindow programs will not display any kind of error dialog
// if there is some error that prevents them to run, but instead produce 
// some sort of error message on stdout or stderr and set the exit code to
// nonzero. While this makes them easier for scripting purposes, this will
// leave a user unfamiliar with UNIX a bit baffled - they will click the
// shiny icon and nothing will happen. This function should help a window 
// manager or program launcher to do something smart about it.

#define MAXBUF 10000

int PtyProcess::runChild()
{
	int ret = NotExited;
	int nbytes;
	char buf[256];
	
	const char *message = _("\n *** Further output ommitted by Edelib ***\n");
	
	while (ret == NotExited) {
		while (1) {
			nbytes = read(m_Fd, buf, 255);
			if (nbytes == -1)
			{
				if (errno == EINTR)
					continue;
				else break;
			}
			if (nbytes == 0)
				break;        // eof
		
			buf[nbytes] = '\0';

			// We don't want m_Inbuf to grow too big
			if (m_Inbuf && strlen(m_Inbuf)<=MAXBUF) {
				m_Inbuf = (char*)realloc(m_Inbuf, strlen(m_Inbuf)+nbytes+1);
				strcat(m_Inbuf, buf);

			} else if (m_Inbuf == 0)
				m_Inbuf = strdup(buf);
		}
		ret = checkPidExited(m_Pid);
	}

	if (m_Inbuf && strlen(m_Inbuf)>MAXBUF) {
		// Attach message about cutting out the rest
		m_Inbuf = (char*)realloc(m_Inbuf, strlen(m_Inbuf)+strlen(message));
		strcat(m_Inbuf, message);
	}
	return ret;
}

/*
 * Copy output to stdout until the child process exists, or a line of output
 * matches `m_Exit'.
 * We have to use waitpid() to test for exit. Merely waiting for EOF on the
 * pty does not work, because the target process may have children still
 * attached to the terminal.
 */

int PtyProcess::waitForChild()
{
    int retval = 1;

    fd_set fds;
    FD_ZERO(&fds);

    
    while (1)
    {
        FD_SET(m_Fd, &fds);
        int ret = select(m_Fd+1, &fds, 0L, 0L, 0L);
        if (ret == -1)
        {
            if (errno != EINTR)
            {
//                kdError(900) << k_lineinfo << "select(): " << perror << "\n";
                return -1;
            }
            ret = 0;
        }

        if (ret)
        {
            char *line = readLine(false);
            while (line && strlen(line)>0)
            {
                if (m_Exit && strlen(m_Exit)>0 && !strncasecmp(line, m_Exit, strlen(m_Exit)))
                    kill(m_Pid, SIGTERM);
                if (m_bTerminal)
                {
                    fputs(line, stdout);
                    fputc('\n', stdout);
                }
                line = readLine(false);
            }
        }

	ret = checkPidExited(m_Pid);
	if (ret == Error)
	{
		if (errno == ECHILD) retval = 0;
		else retval = 1;
		break;
	}
	else if (ret == Killed || ret == Crashed)
	{
		retval = 0;
		break;
	}
	else if (ret == NotExited)
	{
		// keep checking
	}
	else
	{
		retval = ret;
		break;
	}
    }
    return retval;
}

/*
 * SetupTTY: Creates a new session. The filedescriptor "fd" should be
 * connected to the tty. It is closed after the tty is reopened to make it
 * our controlling terminal. This way the tty is always opened at least once
 * so we'll never get EIO when reading from it.
 */

int PtyProcess::SetupTTY(int fd)
{
    // Reset signal handlers
    for (int sig = 1; sig < NSIG; sig++)
        signal(sig, SIG_DFL);
    signal(SIGHUP, SIG_IGN);

    // Close all file handles
    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    for (int i = 0; i < (int)rlp.rlim_cur; i++)
        if (i != fd) close(i);

    // Create a new session.
    setsid();

    // Open slave. This will make it our controlling terminal
    int slave = open(m_TTY, O_RDWR);
    if (slave < 0)
    {
        fprintf(stderr, "Edelib: PtyProcess: Could not open slave side: %s\n", strerror(errno));
        return -1;
    }
    close(fd);

#if defined(__SVR4) && defined(sun)

    // Solaris STREAMS environment.
    // Push these modules to make the stream look like a terminal.
    ioctl(slave, I_PUSH, "ptem");
    ioctl(slave, I_PUSH, "ldterm");

#endif

#ifdef TIOCSCTTY
    ioctl(slave, TIOCSCTTY, NULL);
#endif

    // Connect stdin, stdout and stderr
    dup2(slave, 0); dup2(slave, 1); dup2(slave, 2);
    if (slave > 2)
        close(slave);

    // Disable OPOST processing. Otherwise, '\n' are (on Linux at least)
    // translated to '\r\n'.
    struct termios tio;
    if (tcgetattr(0, &tio) < 0)
    {
        fprintf (stderr, "Edelib: PtyProcess: tcgetattr(): %s\n", strerror(errno));
        return -1;
    }
    tio.c_oflag &= ~OPOST;
    if (tcsetattr(0, TCSANOW, &tio) < 0)
    {
        fprintf(stderr, "Edelib: PtyProcess: tcsetattr(): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

void PtyProcess::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }
