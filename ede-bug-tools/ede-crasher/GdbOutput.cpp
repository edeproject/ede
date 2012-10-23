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

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <edelib/Debug.h>
#include <edelib/File.h>
#include <edelib/FileTest.h>
#include <edelib/TempFile.h>

#include "GdbOutput.h"

/* assume core is placed in current directory */
#define CORE_FILE "core"

EDELIB_NS_USING(String)
EDELIB_NS_USING(TempFile)
EDELIB_NS_USING(file_path)
EDELIB_NS_USING(file_test)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(FILE_TEST_IS_REGULAR)

static int write_str(int fd, const char *str) {
	int len = strlen(str);
	return ::write(fd, str, len);
}

bool gdb_output_generate(const char *path, TempFile &t, int pid) {
	E_RETURN_VAL_IF_FAIL(path != NULL, false);

	int      tfd = -1;
	TempFile scr;

	if(!scr.create("/tmp/.ecrash-script")) {
		E_WARNING(E_STRLOC ": Unable to create temporary file for debugger script: (%i) %s",
				scr.status(), strerror(scr.status()));
		return false;
	}

	if(!t.create("/tmp/.ecrash-output")) {
		E_WARNING(E_STRLOC ": Unable to create temporary file for debugger output: (%i) %s",
				t.status(), strerror(t.status()));
		return false;
	}

	tfd = t.handle();

	/* write script */
	::write(scr.handle(), "bt\nquit\n", 8);
	scr.set_auto_delete(true);
	scr.close();

	String gdb_path = file_path("gdb");
	if(gdb_path.empty()) {
		/* write straight to the file, so dialog could show it */
		write_str(tfd, "Unable to find gdb. Please install it first");

		/* see it as valid, so dialog could be shown */
		return true;
	}

	/*
	 * to find core file, we will try these strategies: first try to open 'core.PID' if
	 * we got PID (default on linux); if does not exists, try to open 'core'; everything is
	 * assumed current folder, whatever it was set
	 */
	bool   core_found = false;
	String core_path;
	if(pid > -1) {
		core_path.printf("%s.%i", CORE_FILE, pid);
		if(file_test(core_path.c_str(), FILE_TEST_IS_REGULAR))
			core_found = true;
		else
			E_WARNING(E_STRLOC ": Unable to find %s. Trying 'core'\n", core_path.c_str());
	}

	if(!core_found) {
		core_path = CORE_FILE;
		if(file_test(core_path.c_str(), FILE_TEST_IS_REGULAR))
			core_found = true;
		else
			E_WARNING(E_STRLOC ": Unable to find core. Balling out\n");
	}

	if(!core_found) {
		write_str(tfd, "Unable to find core file. Backtrace will not be done.");
		/* see it as valid, so dialog could be shown */
		return true;
	}

	pid_t gdb_pid = fork();

    if(gdb_pid == -1) {
		E_WARNING(E_STRLOC ": Unable to fork the process\n");
        return false;
    } else if(gdb_pid == 0) {
		/* child; redirect to the file */
        dup2(tfd, 1);
		t.close();

		::write(1, " ", 1);

        char* argv[8];
        argv[0] = (char*)gdb_path.c_str();
        argv[1] = (char*)"--quiet";
        argv[2] = (char*)"--batch";
        argv[3] = (char*)"-x";
        argv[4] = (char*)scr.name();
        argv[5] = (char*)path;
        argv[6] = (char*)core_path.c_str();
        argv[7] = 0;

        execvp(argv[0], argv);
        return false;
    } else {
        int status;

        if(waitpid(gdb_pid, &status, 0) != gdb_pid) {
			E_WARNING(E_STRLOC ": Failed to execute waitpid() properly\n");
            return false;
		}
    }

	file_remove(core_path.c_str());
	return true;
}
