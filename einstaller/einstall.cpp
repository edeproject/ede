/*
 * $Id$
 *
 * Package manager for Equinox Desktop Environment
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "einstall.h"
#include "einstaller.h"

#include <fltk/run.h>
#include <fltk/filename.h> // for PATH_MAX
#include "../edelib2/process.h"
#include "../edelib2/Run.h"

using namespace fltk;
using namespace edelib;



// TODO: Check for memleaks
// TODO: use generated temp file names



// This is a simple Pty helper function to reduce typing
// Mostly copied from edelib2/Run.cpp but some important changes
// TODO: Consider merging with Run.cpp

int ptyrun(const char *command)
{
	fprintf (stderr, "command: %s\n",command);
	extern char **environ;
	
	// Initialize PTY
	PtyProcess *child = new PtyProcess();
	child->setEnvironment((const char**)environ); // environ is C constant
	
	const char *cmdparts[4];
	cmdparts[0] = "/bin/sh";
	cmdparts[1] = "-c";
	cmdparts[2] = command;
	cmdparts[3] = NULL;
	
	if (child->exec(cmdparts[0], cmdparts) < 0) {
		fprintf (stderr, "Failed to start pty\n");
		return -1;
	}

	// Wait for process to actually start. Shouldn't last long
	while (1) {
		int p = child->pid();
		if (p != 0 && child->checkPid(p))
			break;
		int exit = child->checkPidExited(p);
		if (exit != -2) {
			// Process is DOA
			fprintf (stderr, "Process has died unexpectedly! Exit status: %d\n",exit);
			delete child;
		}
		fprintf (stderr, "Not started yet...\n");
	}
	
	int status = child->checkPidExited(child->pid());
	while (status == PtyProcess::NotExited) {
		char *buffer = child->readLine();
		if (buffer != 0) {
			result_output->insert(buffer);
			result_output->relayout();
			result_output->scroll(result_output->buffer()->length(),0);
			fltk::flush();
		}
		status = child->checkPidExited(child->pid());
	}
	fprintf (stderr, "status: %d\n",status);
	delete child;
	return status;
}


// Install program from source code using standard procedure (configure, make, make install)

void sourcecode(const char* directory, const char* logname)
{
	char workpath[PATH_MAX], workfile[PATH_MAX], command[PATH_MAX];

	// Buffer for stat()
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));

	// Sometimes archives contain everything in one directory
	DIR *my_dir; 
	int count; 
	struct dirent *my_dirent;
	char my_char[PATH_MAX];
	strcpy (workpath, directory);

recurse:
	my_dir = opendir(workpath);
	count=0;
	while ((my_dirent = (struct dirent64*) readdir(my_dir)) != NULL) {
		strncpy(my_char, my_dirent->d_name, PATH_MAX);
		count++;
	}
	closedir(my_dir);
	if (count < 4) { // suspicious
		char tmp[PATH_MAX*2+1];
		strcpy (tmp, workpath);
		strcat (tmp, "/");
		strcat (tmp, my_char);
		stat(tmp, buf);
		if (strcmp(my_char,".")!=0 && strcmp(my_char,"..")!=0 && S_ISDIR(buf->st_mode)) {
			strncpy (workpath, tmp, PATH_MAX);
			goto recurse;
		}
	}

	// Main program loop
	install_progress->position(0);
	while (1) {
		snprintf(workfile, sizeof(workfile)-1, "%s/Makefile", workpath);
		if (stat (workfile, buf) == 0) {
			install_progress->position(50);
			snprintf(command, sizeof(command)-1, "cd %s; make", workpath);
			ptyrun(command);
			install_progress->position(75);
			snprintf(command, sizeof(command)-1, "cd %s; make install >> %s", workpath, logname);
			run_program(command,true,true,false);
			install_progress->position(100);
			result_output->insert(_("=== Program installed! ===\n"));
			break;
		}

		snprintf(workfile, sizeof(workfile)-1, "%s/configure", workpath);
		if (stat (workfile, buf) == 0) {
			install_progress->position(25);
			snprintf(command, sizeof(command)-1, "cd %s; ./configure", workpath);
			ptyrun(command);
			
			// Test to see if configure succeeded
			snprintf(workfile, sizeof(workfile)-1, "%s/Makefile", workpath);
			if (stat (workfile, buf) == 0) {
				install_progress->position(50);
				continue; // go back to start
			} else {
				result_output->insert(_("There was an error running configure. See below for details.\n\n"));
				break;
			}
		}

		snprintf(workfile, sizeof(workfile)-1, "%s/configure.in", workpath);
		if (stat (workfile, buf) == 0) {
			snprintf(command, sizeof(command)-1, "cd %s; autoconf", workpath);
			ptyrun(command);
			
			// Test to see if autoconf succeeded
			snprintf(workfile, sizeof(workfile)-1, "%s/configure", workpath);
			if (stat (workfile, buf) == 0) {
				install_progress->position(25);
				continue; // go back to start
			}
		}

		// Nothing found...
		result_output->insert(_("This archive is not recognized as source code. Try looking inside with archiver.\n"));
		break;
	}

	// Clean up
	snprintf(command, sizeof(command)-1, "rm -fr %s", directory);
	run_program(command);
	free(buf);
}


void install_package(const char *package, bool nodeps)
{
	char tempname[PATH_MAX], logname[PATH_MAX], tempdir[PATH_MAX];
	
	const char *e = filename_ext(package); 
	strncpy(tempname, "/tmp/einstXXXXXX", PATH_MAX); // Use better temp file name
	close(mkstemp(tempname)); 
	remove(tempname);
	strncpy(logname, tempname, PATH_MAX);

	if (strlen(e)<1) {
		result_output->insert(_("Package type is not recognized. Einstaller presently supports rpm, deb, tgz and source code packages.\n"));
		result_output->relayout();
		return; 
	}

	if (strcmp(e, ".rpm")==0) {
		char command[PATH_MAX];

		if (nodeps) 
			snprintf(command, PATH_MAX, "rpm -i --nodeps %s >& %s", package, logname);
		else 
			snprintf(command, PATH_MAX, "rpm -i %s >& %s", package, logname);
		run_program(command,true,true,false);
		install_progress->position(100);
	}
	else if (strcmp(e, ".tgz")==0) {
		char command[PATH_MAX];
		snprintf(command, PATH_MAX, "installpkg %s >& %s", package, logname);
		run_program(command,true,true,false);
		install_progress->position(100);
	}
	else if (strcmp(e, ".deb")==0) {
		char command[PATH_MAX];
		snprintf(command, PATH_MAX, "dpkg -i %s >& %s", package, logname);
		run_program(command,true,true,false);
		install_progress->position(100);
	}
	else if (strcmp(e, ".gz")==0) {
		char command[PATH_MAX];
		
		// Create temp directory
		strncpy(tempdir, "/tmp/einstdXXXXXX", PATH_MAX);
		mkdtemp(tempdir);
		
		snprintf(command, PATH_MAX, "tar xzvC %s -f %s", tempdir, package);
		ptyrun(command);
		sourcecode(tempdir, logname);
	}
	else if (strcmp(e, ".bz2")==0) {
		char command[PATH_MAX];
		
		// Create temp directory
		strncpy(tempdir, "/tmp/einstdXXXXXX", PATH_MAX);
		mkdtemp(tempdir);
		
		snprintf(command, PATH_MAX, "tar xjvC %s -f %s", tempdir, package);
		ptyrun(command);
		sourcecode(tempdir, logname);
	}
	else if (strcmp(e, ".tar")==0) {
		char command[PATH_MAX];
		
		// Create temp directory
		strncpy(tempdir, "/tmp/einstdXXXXXX", PATH_MAX);
		mkdtemp(tempdir);
		
		snprintf(command, PATH_MAX, "tar xvC %s -f %s", tempdir, package);
		ptyrun(command);
		sourcecode(tempdir, logname);
	}
	else {   
		result_output->insert(_("Package type is not recognized. Einstaller presently supports rpm, deb, tgz and source code packages.\n"));
		result_output->relayout();
		return;
	}

	char line[1024];
	FILE* log = fopen(logname, "r");

	if (log != NULL) {
		while(fgets(line, sizeof(line), log))
			result_output->insert(line);
		result_output->relayout();
		result_output->scroll(result_output->buffer()->length(),0);
		fclose(log);
	}
	unlink(logname);
}
