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

class edelib::TempFile;

bool gdb_output_generate(const char *path, edelib::TempFile &t);
#endif
