/*
 * $Id$
 *
 * edelib::Run - Library for executing external programs
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#define PREFIX "/usr"

#include "Run.h"

#include "Config.h"
#include <fltk/ask.h>
#include "NLS.h"
#include "process.h"

using namespace fltk;
using namespace edelib;


// GLOBAL NOTE: asprintf() is a GNU extension which is also available under *BSD
// If this is considered a problem, use our tasprintf() instead (in Util.h)


// --------------------------------------------
// Start a process using exec(3)
// This means that there is no handling or control over process,
// it's just forked into background. If you need to chat with
// program or use its output, see edelib::PtyProcess
// --------------------------------------------

int run_fork(const char *cmd, bool wait)
{
	int pid, status;
	int nulldev;
	extern char **environ;
	
	status=0;
	if (cmd == NULL)
		return (EDERUN_EMPTY);
	
	pid = fork ();
	if (pid == -1)
		return (EDERUN_FORK_FAILED);
	if (pid == 0)
	{
		char *argv[4];
		// child
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = (char*)cmd;
		argv[3] = NULL;
		
		// The following is to avoid X locking when executing
		//  terminal based application that requires user input
		if ((nulldev = open ("/dev/null", O_RDWR)))
		{
			close (0); dup (nulldev);
			close (1); dup (nulldev);
			close (2); dup (nulldev);
		}
		
		if (execve ("/bin/sh", argv, environ) == -1)
			perror ("/bin/sh");
		_exit (EDERUN_EXECVE_FAILED);
	}
	do
	{
		if ((wait) && (waitpid (pid, &status, 0) == -1))
		{
			if (errno != EINTR)
				return (EDERUN_WAITPID_FAILED);
		}
		else {
			if (status==127) status=EDERUN_NOT_FOUND;
			if (status==126) status=EDERUN_NOT_EXEC;
			return status;
		}
	}
	while (1);
    
    return 0;
}


// --------------------------------------------
// Start a process as root user
// We use edelib::PtyProcess to chat with su/sudo. Afterwards
// the program continues undisturbed
// --------------------------------------------

// this is our internal message:
#define CONTMSG "elauncher_ok_to_continue"
// these are part of sudo/su chat:
#define PWDQ "Password:"
#define BADPWD "/bin/su: incorrect password"
#define SUDOBADPWD "Sorry, try again."

int run_as_root(const char *cmd, bool wait) 
{
	// -- we could check for availibility of sudo, but there's no point
	bool use_sudo = false;
	Config pGlobalConfig(find_config_file("ede.conf", 0));
	pGlobalConfig.set_section("System");
	pGlobalConfig.read("UseSudo", use_sudo, false);
	
	// Prepare array as needed by exec()
	char *parts[4];
	if (use_sudo) {
		parts[0] = "/bin/sudo";
		parts[1] = "";
		// This "continue message" prevents accidentally exposing password
		asprintf(&parts[2], "echo %s; %s", CONTMSG, cmd);
		parts[3] = NULL;
	} else {
		parts[0] = "/bin/su";
		parts[1] = "-c";
		// This "continue message" prevents accidentally exposing password
		asprintf(&parts[2], "echo %s; %s", CONTMSG, cmd);
		parts[3] = NULL;
	}
	// the actual command is this:
//	cmd_ = strtok(cmd," ");
	
tryagain:
	PtyProcess *child = new PtyProcess();
	child->setEnvironment((const char**)environ);
	if (child->exec(parts[0], (const char**)parts) < 0) {
		return EDERUN_PTY_FAILED;
	}
	
	// Wait for process to actually start. Shouldn't last long
	while (1) {
		int p = child->pid();
		if (p != 0 && child->checkPid(p))
			break;
		int exit = child->checkPidExited(p);
		if (exit != -2) {
			// Process is DOA
			fprintf (stderr, "Edelib: Process has died unexpectedly! Exit status: %d\n",exit);
			delete child;
			goto tryagain;
		}
		fprintf (stderr, "Edelib: Process not started yet...\n");
	}

	// Run program as root using su or sudo
	char *line;

	// TODO: fix password dialog so that Cancel can be detected
	// At the moment it's impossible to tell if the password is blank
	const char *pwd = password(_("This program requires administrator privileges.\nPlease enter your password below:"));
	
	// Chat routine
	while (1) {
		line = child->readLine();
		
		// This covers other cases of failed process startup
		// Our su command should at least produce CONTMSG
		if (line == 0 && child->checkPidExited(child->pid()) != PtyProcess::NotExited) {
			// TODO: capture stdout? as in sudo error?
			fprintf (stderr, "Edelib: su process has died unexpectedly in chat stage!\n");
			delete child;
			
			if (choice_alert (_("Failed to start authentication. Try again"), 0, _("Yes"), _("No")) == 2) return 0;
			goto tryagain;
		}
		
		if (strncasecmp(line,PWDQ,strlen(PWDQ))== 0)
			child->writeLine(pwd,true);

		if (strncasecmp(line,CONTMSG,strlen(CONTMSG)) == 0)
			break; // program starts...

		if ((strncasecmp(line,BADPWD,strlen(BADPWD)) == 0) || (strncasecmp(line,SUDOBADPWD,strlen(SUDOBADPWD)) == 0)) {
				// end process
			child->waitForChild();
			delete child;
			
			if (choice_alert (_("The password is wrong. Try again?"), 0, _("Yes"), _("No")) == 2) return 0;

			goto tryagain;
		}
	}
	
	// Wait for program to end, discarding output
	int child_val = child->waitForChild();
	if (child_val==127) child_val=EDERUN_NOT_FOUND;
	if (child_val==126) child_val=EDERUN_NOT_EXEC;
	
	// deallocate one string we allocated 
	free(parts[2]);
	delete child;
	
	return child_val;
}


static bool done_checks=false;
static bool elauncher_found=false;

// Check availability of various necessary components
// For the moment just elauncher :)
void do_checks()
{
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	if (stat (PREFIX"/bin/elauncher", buf) == 0)
		elauncher_found = true;
	else
		elauncher_found = false;
}


int edelib::run_program(const char *path, bool wait, bool root, bool use_elauncher)
{
	char *execstr;
	if (!done_checks) do_checks();
	if (use_elauncher && elauncher_found) {
		if (root)
			asprintf(&execstr,"elauncher --root \"%s\"", path);
		else
			asprintf(&execstr,"elauncher \"%s\"", path);
		run_fork (execstr, false); // elauncher can't wait
	} else {
		if (root)
			return run_as_root(path, wait);
		else
			return run_fork(path, wait);
	}
	return 0; // shutup compiler!
}
