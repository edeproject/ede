/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Spawn.h"

#include <sys/types.h> // fork
#include <unistd.h>    // fork, open, close, dup
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

extern char** environ;

int run_fork(const char* cmd, bool wait) {
	if(!cmd)
		return SPAWN_EMPTY;

	int nulldev = -1;
	int status_ret = 0;
	
	pid_t pid = fork();

	if(pid == -1)
		return SPAWN_FORK_FAILED;

	// run the child
	if(pid == 0) {
		char* argv[4];
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = (char*)cmd;
		argv[3] = NULL;

		/*
		 * The following is to avoid X locking when executing 
		 * terminal based application that requires user input
		 */
		if((nulldev = open("/dev/null", O_RDWR)) == -1)
			return SPAWN_FORK_FAILED;

		/* TODO: redirect these to EvokeService log */
		close(0); dup(nulldev);
		close(1); dup(nulldev);
		close(2); dup(nulldev);

		if(execve(argv[0], argv, environ) == -1) {
			close(nulldev);
			// should not get here
			return SPAWN_EXECVE_FAILED;
		}
	}

	return status_ret;
}


int spawn_program(const char* cmd, bool wait) {
	return run_fork(cmd, wait);
}
