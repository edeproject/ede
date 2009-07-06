/*
 * $Id$
 *
 * ede-crasher, a crash handler tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __GDBOUTPUT_H__
#define __GDBOUTPUT_H__

#ifndef PATH_MAX
# define PATH_MAX 256
#endif

#include <edelib/String.h>

class GdbOutput {
private:
	int  sfd, ofd;
	const char *program_path;
	edelib::String gdb_output_path;
	edelib::String gdb_script_path;

public:
	GdbOutput() : sfd(-1), ofd(-1), program_path(NULL) { }
	~GdbOutput();

	void set_program_path(const char *p) { program_path = p; }

	bool fds_open(void);
	bool fds_opened(void) { return (sfd != 1 && ofd != -1); }

	bool run(void);
	const char *output_path(void) { return (gdb_output_path.empty() ? NULL : gdb_output_path.c_str()); }
};

#endif
