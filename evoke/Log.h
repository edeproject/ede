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

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

class Log {
	public:
		Log() { }
		virtual ~Log() { }
		virtual bool open(const char* file) = 0;
		virtual void printf(const char* fmt, ...) = 0;
};

class DummyLog : public Log {
	public:
		DummyLog() { }
		virtual ~DummyLog() { }
		virtual bool open(const char* file) { return true; }
		virtual void printf(const char* fmt, ...) { }
};

class RealLog : public Log {
	private:
		FILE* f;
		char* buff;
		char* tbuff;
		int bufflen;
		int tbufflen;
		bool to_stdout;
		bool to_stderr;

	public:
		RealLog();
		virtual ~RealLog();
		virtual bool open(const char* file);
		virtual void printf(const char* fmt, ...);
};

#endif
