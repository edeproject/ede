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

#include "Log.h"
#include <edelib/Debug.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

RealLog::RealLog() : f(NULL), buff(NULL), tbuff(NULL), bufflen(256), tbufflen(20), 
	to_stdout(false), to_stderr(false) { 
}

RealLog::~RealLog() {
	if(f) {
		puts("RealLog::~RealLog()\n");
		fclose(f);
	}

	if(buff)
		delete [] buff;
	if(tbuff)
		delete [] tbuff;
}

bool RealLog::open(const char* file) {
	EASSERT(file != NULL);

	if(strcmp(file, "stdout") == 0)
		to_stdout = true;
	else if(strcmp(file, "stderr") == 0)
		to_stderr = true;
	else {
		f = fopen(file, "a");
		if(!f)
			return false;
	}

	buff = new char[bufflen];
	tbuff = new char[tbufflen];
	return true;
}

void RealLog::printf(const char* fmt, ...) {
	EASSERT(buff != NULL);
	EASSERT(tbuff != NULL);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buff, bufflen, fmt, ap);
	va_end(ap);

	time_t t = time(NULL);
	strftime(tbuff, tbufflen, "%F %T", localtime(&t));

	if(to_stdout)
		fprintf(stdout, "[%s] %s", tbuff, buff);
	else if(to_stderr)
		fprintf(stderr, "[%s] %s", tbuff, buff);
	else {
		fprintf(f, "[%s] %s", tbuff, buff);
		fflush(f);
	}
}
