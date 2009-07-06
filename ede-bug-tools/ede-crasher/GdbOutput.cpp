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

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <edelib/Missing.h>
#include <edelib/File.h>
#include <edelib/Debug.h>
#include <edelib/File.h>
#include <edelib/FileTest.h>

#include "GdbOutput.h"

/* assume core is placed in current directory */
#define CORE_FILE "core"

EDELIB_NS_USING(String)
EDELIB_NS_USING(file_path)
EDELIB_NS_USING(file_test)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(FILE_TEST_IS_REGULAR)

static void close_and_invalidate(int &fd) {
	if(fd != -1) {
		::close(fd);
		fd = -1;
	}
}

static int write_str(int fd, const char *str) {
	int len = strlen(str);
	return ::write(fd, str, len);
}

GdbOutput::~GdbOutput() {
	close_and_invalidate(sfd);
	close_and_invalidate(ofd);

	if(!gdb_script_path.empty())
		file_remove(gdb_script_path.c_str());
	if(!gdb_output_path.empty())
		file_remove(gdb_output_path.c_str());

	file_remove(CORE_FILE);
}

bool GdbOutput::fds_open(void) {
	errno = 0;

	char sp[] = "/tmp/.ecrash-script.XXXXXX";
	char op[] = "/tmp/.ecrash-output.XXXXXX";

	sfd = mkstemp(sp);
	if(sfd == -1) {
		E_WARNING(E_STRLOC ": Unable to open script file: (%i), %s\n", errno, strerror(errno));
		return false;
	}

	/* copy it as fast as possible */
	gdb_script_path = sp;

	ofd = mkstemp(op);
	if(ofd == -1) {
		E_WARNING(E_STRLOC ": Unable to open output file: (%i), %s\n", errno, strerror(errno));
		return false;
	}

	/* copy it as fast as possible */
	gdb_output_path = op;
	return true;
}

bool GdbOutput::run(void) {
	E_RETURN_VAL_IF_FAIL(fds_opened(), false);

	/* write script */
	::write(sfd, "bt\nquit\n", 8);
	close_and_invalidate(sfd);

	String gdb_path = file_path("gdb");
	if(gdb_path.empty()) {
		/* write straight to the file, so dialog could show it */
		write_str(ofd, "Unable to find gdb. Please install it first");

		/* see it as valid, so dialog could be shown */
		return true;
	}

	if(!file_test(CORE_FILE, FILE_TEST_IS_REGULAR)) {
		write_str(ofd, "Unable to find '"CORE_FILE"'. Backtrace will not be done.");
		/* see it as valid, so dialog could be shown */
		return true;
	}

	pid_t pid = fork();

    if(pid == -1) {
        close_and_invalidate(ofd);
		E_WARNING(E_STRLOC ": Unable to fork the process\n");
        return false;
    } else if(pid == 0) {
        dup2(ofd, 1);
        close_and_invalidate(ofd);

        char* argv[8];
        argv[0] = (char*)gdb_path.c_str();
        argv[1] = "--quiet";
        argv[2] = "--batch";
        argv[3] = "-x";
        argv[4] = (char*)gdb_script_path.c_str();
        argv[5] = (char*)program_path;
        argv[6] = (char*)CORE_FILE;
        argv[7] = 0;

        execvp(argv[0], argv);
        return false;
    } else {
        int status;

        if(waitpid(pid, &status, 0) != pid) {
			E_WARNING(E_STRLOC ": Failed to execute waitpid() properly\n");
            return false;
		}
    }

	return true;
}
