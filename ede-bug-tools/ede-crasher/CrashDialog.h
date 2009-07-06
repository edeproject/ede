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

#ifndef __CRASHDIALOG_H__
#define __CRASHDIALOG_H__

struct ProgramDetails {
	bool        ede_app;
	const char *name;
	const char *path;
	const char *pid;
	const char *sig;
	const char *bugaddress;
};

int crash_dialog_show(const ProgramDetails& p);

#endif
