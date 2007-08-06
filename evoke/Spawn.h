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

#ifndef __SPAWN_H__
#define __SPAWN_H__

#include <unistd.h> // pid_t

/*
 * This is little bit modified code from edelib run_program()
 * so evoke specific stuff can be added. Also, possible option
 * is that edelib run_program(), at some time, consult evoke
 * for running programs.
 */
#define SPAWN_NOT_FOUND       65535  // executable not found
#define SPAWN_EMPTY           65534  // given parameter is NULL
#define SPAWN_NOT_EXEC        65533  // given parameter is not executable on system 
#define SPAWN_FORK_FAILED	  65532  // internal fork failed
#define SPAWN_WAITPID_FAILED  65531  // internal waitpid failed
#define SPAWN_EXECVE_FAILED   65530  // internal execve failed
#define SPAWN_PTY_FAILED      65529  // TODO
#define SPAWN_USER_CANCELED   65528  // TODO
#define SPAWN_CRASHED         65527  // executable crashed
#define SPAWN_KILLED          65526  // executable crashed
#define SPAWN_NOEXITED        65525  

typedef void (SignalWatch)(int pid, int status);

int spawn_program(const char* cmd, SignalWatch* wf = 0, pid_t* child_pid_ret = 0);
int spawn_program_with_core(const char* cmd, SignalWatch* wf = 0, pid_t* child_pid_ret = 0);
int spawn_backtrace(const char* program, const char* core_path, const char* output, const char* script);

#endif
