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
#include <stdio.h>

#include <string.h>

#include <sys/time.h>       // getrlimit, setrlimit
#include <sys/resource.h>   //

extern char** environ;
// FIXME: is this safe ??? (or to use somehow sig_atomic_t)
SignalWatch* global_watch = 0;

void sigchld_handler(int sig) {
	int pid, status;
	do {
		errno = 0;
		pid = waitpid(WAIT_ANY, &status, WNOHANG);

		if(global_watch != 0) {
			if(WIFEXITED(status))
				status = WEXITSTATUS(status);
			else if(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV)
				status = SPAWN_CHILD_CRASHED;
			else
				status = SPAWN_CHILD_KILLED;

			global_watch(pid, status);
		}

	} while(pid <= 0 && errno == EINTR);
}

int spawn_program(const char* cmd, SignalWatch wf, pid_t* child_pid_ret, const char* ofile) {
	if(!cmd)
		return SPAWN_EMPTY;

	int nulldev = -1;
	int status_ret = 0;

	if(wf) {
		struct sigaction sa;
		sa.sa_handler = sigchld_handler;
		sa.sa_flags = SA_NOCLDSTOP;
		//sa.sa_flags = SA_RESTART;
		sigemptyset(&sa.sa_mask);
		sigaction(SIGCHLD, &sa, (struct sigaction*)0);

		global_watch = wf;
	}

	pid_t pid = fork();

	if(pid == -1)
		return SPAWN_FORK_FAILED;

	if(pid == 0) {
		// this is child
		char* argv[4];
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = (char*)cmd;
		argv[3] = NULL;

		/*
		 * The following is to avoid X locking when executing 
		 * terminal based application that requires user input
		 */
		if(ofile)
			nulldev = open(ofile, O_WRONLY | O_TRUNC | O_CREAT, 0770);
		else
			nulldev = open("/dev/null", O_RDWR);

		if(nulldev == -1)
			return SPAWN_OPEN_FAILED;

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

	if(nulldev != -1)
		close(nulldev);
	/*
	 * Record child pid; it is returned by fork(), but since this
	 * function does not wait until child quits, it will return
	 * immediately, filling (if) requested child pid
	 */
	if(child_pid_ret)
		*child_pid_ret = pid;

	return status_ret;
}

int spawn_program_with_core(const char* cmd, SignalWatch* wf, pid_t* child_pid_ret) {
	struct rlimit r;
	if(getrlimit(RLIMIT_CORE, &r) == -1)
		return -1;

	rlim_t old = r.rlim_cur;
	r.rlim_cur = RLIM_INFINITY;

	// FIXME: add own core limit ?
	if(setrlimit(RLIMIT_CORE, &r) == -1)
		return -1;

	int ret = spawn_program(cmd, wf, child_pid_ret);

	r.rlim_cur = old;
	if(setrlimit(RLIMIT_CORE, &r) == -1)
		return -1;

	return ret;
}

int spawn_backtrace(const char* program, const char* core, const char* output, const char* script) {
	const char* gdb_script = "bt\nquit\n";
	const int gdb_script_len = 8;

	// file with gdb commands
	int sfd = open(script, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(sfd == -1)
		return -1;
	write(sfd, gdb_script, gdb_script_len);
	close(sfd);

	// output file with gdb backtrace
	int ofd = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(ofd == -1)
		return -1;

	pid_t pid = fork();

	if(pid == -1) {
		close(ofd);
		return -1;
	} else if(pid == 0) {
		dup2(ofd, 1);
		close(ofd);

		char* argv[8];
		argv[0] = "gdb";
		argv[1] = "--quiet";
		argv[2] = "--batch";
		argv[3] = "-x";
		argv[4] = (char*)script;
		argv[5] = (char*)program;
		argv[6] = (char*)core;
		argv[7] = 0;

		execvp(argv[0], argv);
		return -1;
	} else {
		int status;
		if(waitpid(pid, &status, 0) != pid)
			return -1;
	}

	return 0;
}

#if 0
int spawn_backtrace(int crash_pid, const char* output, const char* script) {
	const char* gdb_script = "info threads\nthread apply all bt full\nquit\n";
	const int gdb_script_len = 43;
	//const char* gdb_script = "bt\nquit\n";
	//const int gdb_script_len = 8;

	pid_t parent_pid, child_pid;

	//kill(crash_pid, SIGCONT);

	char parent_pid_str[64];
	parent_pid = crash_pid;
	kill(parent_pid, SIGCONT);

	//parent_pid = getpid();
	sprintf(parent_pid_str, "%ld", (long)parent_pid);

	// file with gdb commands
	int sfd = open(script, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(sfd == -1)
		return -1;
	write(sfd, gdb_script, gdb_script_len);
	close(sfd);

	// output file with gdb backtrace
	int ofd = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(ofd == -1)
		return -1;

	child_pid = fork();

	if(child_pid == -1)
		return -1;
	else if(child_pid == 0) {
		//dup2(ofd, 1);
		close(ofd);

		char* argv[8];
		argv[0] = "gdb";
		argv[1] = "--quiet";
		argv[2] = "--batch";
		argv[3] = "-x";
		argv[4] = (char*)script;
		argv[5] = "--pid";
		argv[6] = parent_pid_str;
		argv[7] = 0;

		//argv[5] = "--pid";
		//argv[6] = parent_pid_str;
		//argv[7] = 0;

		printf("%s %s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);

		execvp(argv[0], argv);
		return -1;
	} else {
		/*
		int status;
		waitpid(child_pid, &status, 0);
		*/
	}

	//unlink(script);
	return 0;
}

#if 0
int spawn_backtrace(const char* output, const char* script){
	pid_t pid, parent_pid;
	int ifd[2], ofd[2], efd[2];

	const char* gdb_script = "info threads\nthread apply all by\nquit\n";
	const int gdb_script_len = 38;

	puts("xxxx");

	// write script for gdb
	int fds = open(script, O_WRONLY | O_CREAT | O_TRUNC, 0770);
	if(fds == -1)
		return -1;
	write(fds, gdb_script, gdb_script_len);
	close(fds);

	puts("yyyyy");

	errno = 0;
	// open output for gdb trace
	int fdo = open(output, O_WRONLY, O_CREAT | O_TRUNC, 0770);
	if(fdo == -1) {
		printf("can't open fdo(%s): %s\n", output, strerror(errno));
		return -1;
	}

	parent_pid = getppid();
	char parent_pid_str[64];
	sprintf(parent_pid_str, "%d", parent_pid);

	char* argv[8];
	argv[0] = "gdb";
	argv[1] = "--quiet";
	argv[2] = "--batch";
	argv[3] = "-x";
	argv[4] = (char*)script;
	argv[5] = "--pid";
	argv[6] = parent_pid_str;
	argv[7] = 0;
	
	puts("ayyyy");

	if(pipe(ifd) == -1) {
		printf("cant open ifd pipe: %s\n", strerror(errno));
	}

	if(pipe(ofd) == -1) {
		printf("cant open ofd pipe: %s\n", strerror(errno));
	}
		
	if(pipe(efd) == -1) {
		printf("cant open efd pipe: %s\n", strerror(errno));
	}

	pid = fork();

	if(pid == -1) {
		close(ifd[0]); close(ifd[1]);
		close(ofd[0]); close(ofd[1]);
		close(efd[0]); close(efd[1]);
		return -1;
	} else if(pid == 0) {
		// child
		printf("gdb pid is %i\n", getpid());
		printf("%s %s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);

		close(ifd[1]);
		fclose(stdin);
		dup(ifd[0]);

		close(ofd[0]);
		fclose(stdout);
		dup(ofd[1]);

		close(efd[0]);
		fclose(stderr);
		dup(efd[1]);

		execvp(argv[0], (char**)argv);
		return -1;
	} else {
		// parent
		fclose(stdin);

		close(ifd[0]);
		close(ofd[1]);
		close(efd[1]);

		write(fdo, "The trace:\n", 11);

		close(ifd[1]);
		close(ofd[0]);
		close(efd[0]);

		close(fdo);

		kill(pid, SIGKILL);

		waitpid(pid, NULL, 0);
		//_exit(0);
	}

	return 0;
}
#endif

#endif
