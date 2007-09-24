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

#define SPAWN_OK            0
#define SPAWN_FORK_FAILED   1
#define SPAWN_EMPTY         2
#define SPAWN_EXECVE_FAILED 3
#define SPAWN_OPEN_FAILED   4
#define SPAWN_PIPE_FAILED   5

#define SPAWN_CHILD_CRASHED -2
#define SPAWN_CHILD_KILLED  -3

typedef void (SignalWatch)(int pid, int status);

int spawn_program(const char* cmd, SignalWatch* wf = 0, pid_t* child_pid_ret = 0, const char* ofile = 0);
int spawn_program_with_core(const char* cmd, SignalWatch* wf = 0, pid_t* child_pid_ret = 0);
int spawn_backtrace(const char* gdb_path, const char* program, const char* core, const char* output, const char* script);

#endif
