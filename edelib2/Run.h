/*
 * $Id$
 *
 * edelib::Run - Library for executing external programs
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef _edelib_Run_h_
#define _edelib_Run_h_

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace edelib {

/** \fn run_program(char *path, bool wait=true, bool root=false, bool use_elauncher=false)
	Standard EDE function for running external tasks. This function is time tested
	and provides a number of neat facilities. 
	
	Parameters:
	\a path - Full path to executable. If path is ommited, function will search PATH
	environment variable.
	\a wait - If true, parent process will be frozen until program ends. If false, 
	it will fork into background and parent has no way to know what happened with it.
	default = true
	\a root - If true, sudo will be used to run program as root, with a nice facility 
	to enter your root password. If sudo is not available, "su -c" will be tried.
	default = false
	\a use_elauncher - Program will be launched through elauncher which provides busy
	cursor, information about missing executable, standard output, backtrace in case
	of segfault etc. However, use of elauncher may cause some minimal overhead. 
	Also, since there is no way to wait with elauncher, \a wait value will be
	ignored. default = false
	
	Return value of the function is program "exit value". Usually exit value of 0 
	means successful execution, and values 1-255 have certain special meanings 
	per program documentation. Several special values above 255 are:
	EDERUN_NOT_FOUND - \a cmd doesn't exist
	EDERUN_EMPTY - \a cmd is empty
	EDERUN_NOT_EXEC - \a cmd doesn't have execute permission
	EDERUN_FORK_FAILED - fork() function returned a PID of -1 (see fork(2))
	EDERUN_WAITPID_FAILED - waitpid() function resulted with error (see waitpid(2))
	EDERUN_EXECVE_FAILED - execve() function returned -1 (see execve(2))
	EDERUN_PTY_FAILED - could not create pseudo-terminal (see getpt(3) and grantpt(3))
*/

enum{
	EDERUN_NOT_FOUND	= 65535,
	EDERUN_EMPTY		= 65534,
	EDERUN_NOT_EXEC		= 65533,
	EDERUN_FORK_FAILED	= 65532,
	EDERUN_WAITPID_FAILED	= 65531,
	EDERUN_EXECVE_FAILED	= 65530,
	EDERUN_PTY_FAILED	= 65529,
	EDERUN_USER_CANCELED	= 65528
};


int run_program(const char *path, bool wait=true, bool root=false, bool use_elauncher=false);

}

#endif
