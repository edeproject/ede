/*
 * $Id$
 *
 * Copyright (C) 2012-2013 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pixmap.H>

#include <edelib/Run.h>
#include <edelib/Resource.h>
#include <edelib/Window.h>
#include <edelib/Debug.h>
#include <edelib/Missing.h>
#include <edelib/MessageBox.h>
#include <edelib/String.h>
#include <edelib/File.h>
#include <edelib/WindowUtils.h>
#include <edelib/DesktopFile.h>
#include <edelib/StrUtil.h>
#include <edelib/Debug.h>
#include <edelib/Directory.h>
#include <edelib/Regex.h>
#include <edelib/Util.h>
#include <edelib/Ede.h>
#include "StartupNotify.h"

#include "icons/run.xpm"

/* so value could be directed returned to shell */
#define RETURN_FROM_BOOL(v) (v != false)

/* config name where all things are stored and read from */
#define EDE_LAUNCH_CONFIG "ede-launch"

/* default 'Preferred' key in config file */
#define KEY_PREFERRED "Preferred"

/* patterns for guessing user input */
#define REGEX_PATTERN_MAIL "\\b[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}\\b"
#define REGEX_PATTERN_URL  "((http|https|ftp|gopher|!file):\\/\\/|www)[a-zA-Z0-9\\-\\._]+\\/?[a-zA-Z0-9_\\.\\-\\?\\+\\/~=&#;,]*[a-zA-Z0-9\\/]{1}"

/* list of known terminals */
static const char *known_terminals[] = {
	"xterm",
	"rxvt",
	"urxvt",
	"mrxvt",
	"st",
	"Terminal",
	"gnome-terminal",
	"konsole",
	0
};

EDELIB_NS_USING_AS(Window, LaunchWindow)
EDELIB_NS_USING_LIST(14, (Resource,
						  Regex,
						  String,
						  list,
						  DesktopFile,
					  	  RES_USER_ONLY,
						  DESK_FILE_TYPE_APPLICATION,
						  run_sync, run_async, alert, file_path, window_center_on_screen, str_ends, system_data_dirs))

static Fl_Pixmap        image_run((const char**)run_xpm);
static Fl_Input*        dialog_input;
static Fl_Check_Button *in_term;
static Resource         launcher_resource; /* lazy loaded */

static void help(void) {
	puts("Usage: ede-launch [OPTIONS] [PROGRAM]");
	puts("EDE program launcher");
	puts("Options:");
	puts("   -h, --help                            show this help");
	puts("   -l, --launch [TYPE] [PARAMETERS]      launch preferred application of TYPE with");
	puts("                                         given PARAMETERS; see Types below");
	puts("   -w, --working-dir [DIR]               run programs with DIR as working directory\n");
	puts("Types:");
	puts("   browser                               preferred web browser");
	puts("   mail                                  preferred mail reader"); 
	puts("   terminal                              preferred terminal");
	puts("   file_manager                          preferred file manager\n");
	puts("Example:");
	puts("   ede-launch --launch browser http://www.foo.com");
	puts("   ede-launch --launch mail mailto:foo@foo.com");
	puts("   ede-launch foo@foo.com");
	puts("   ede-launch gvim");
	puts("   ede-launch ~/Desktop/foo.desktop");
}

static int re_match(const char *p, const char *str) {
	static Regex re;

	E_RETURN_VAL_IF_FAIL(re.compile(p) == true, -1);
	return re.match(str);
}

static const char *resource_get(const char *g, const char *k) {
	if(!launcher_resource)
		launcher_resource.load(EDE_LAUNCH_CONFIG);

	E_RETURN_VAL_IF_FAIL(launcher_resource != false, NULL);

	static char buf[64];
	return launcher_resource.get(g, k, buf, sizeof(buf)) ? (const char*)buf : NULL;
}

static char *get_basename(const char* path) {
	char *p = (char*)strrchr(path, E_DIR_SEPARATOR);
	return p ? p + 1 : p;
}

static char **cmd_split(const char* cmd) {
	int i = 0, sz = 10;
	char *c = strdup(cmd);

	char **arr = (char**)malloc(sizeof(char*) * sz);

	for(char *p = strtok(c, " "); p; p = strtok(NULL, " ")) {
		if(i >= sz) {
			sz *= 2;
			arr = (char**)realloc(arr, sizeof(char*) * sz);
		}
		arr[i++] = strdup(p);
	}

	arr[i] = NULL;
	free(c);

	return arr;
}

static bool allowed_launch_type(const char *t) {
	E_RETURN_VAL_IF_FAIL(t != 0, false);

	/* should be synced with keys from ede-preferred-applications */
	static const char *launch_types[] = {
		"browser",
		"mail",
		"terminal",
		"file_manager",
		0
	};

	for(int i = 0; launch_types[i]; i++) {
		/* we do not allow >= 64 chars from user input */
		if(strncmp(t, launch_types[i], 64) == 0)
			return true;
	}

	return false;
}

#define RETURN_IF_VALID_TERM(t, r) \
do { \
	if(t && ((strcmp(t, "linux") != 0) || (strcmp(t, "dumb") != 0))) { \
		r = file_path(t, false); \
		if(E_UNLIKELY(r.empty())) return true; \
	} \
} while(0)

static bool find_terminal(String &ret) {
	const char *term = resource_get(KEY_PREFERRED, "terminal");
	if(term) {
		ret = term;
		return true;
	}

	term = getenv("EDE_LAUNCH_TERMINAL");
	RETURN_IF_VALID_TERM(term, ret);

	for(int i = 0; known_terminals[i]; i++) {
		term = known_terminals[i];
		RETURN_IF_VALID_TERM(term, ret);
	}

	return false;
}

static void start_crasher(const char* cmd, int sig, int pid) {
	const char *ede_app_flag = "", *base = get_basename(cmd);

	/* this means the app was called without full path */
	if(!base) base = cmd;

	/* 
	 * determine is our app by checking the prefix; we don't want user to send bug reports about crashes 
	 * of foreign applications
	 */
	if((strncmp(base, "ede-", 4) == 0) || (strncmp(base, "edelib-", 7) == 0))
		ede_app_flag = "--edeapp";

	/* call edelib implementation instead start_child_process() to prevents loops if 'ede-crasher' crashes */
	run_sync(PREFIX "/bin/ede-crasher %s --appname %s --apppath %s --signal %i --pid %i", ede_app_flag, base, cmd, sig, pid);
}

static int start_child_process(const char* cmd) {
	int pid, in[2], out[2], err[2];
	char** params = cmd_split(cmd);

	pipe(in);
	pipe(out);
	pipe(err);

	signal(SIGCHLD, SIG_DFL);

	pid = fork();
	switch(pid) {
		case 0:
			/* child process */
			close(0);
			dup(in[0]);
			close(in[0]);
			close(in[1]);

			close(1);
			dup(out[1]);
			close(out[0]);
			close(out[1]);

			close(2);
			dup(err[1]);
			close(err[0]);
			close(err[1]);

			errno = 0;

			/* start it */
			execvp(params[0], params);

			/* some programs use value 2 (tar) */
			if(errno == 2)
				_exit(199);
			else
				_exit(errno);
			break;
		case -1:
			E_WARNING(E_STRLOC ": fork() failed\n");
			/* close the pipes */
			close(in[0]);
			close(in[1]);

			close(out[0]);
			close(out[1]);

			close(err[0]);
			close(err[1]);
			break;
		default:
			/* parent */
			close(in[0]);
			close(out[1]);
			close(err[1]);
			break;
	}

	/* cleanup when returns from the child */
	for(int i = 0; params[i]; i++)
		free(params[i]);
	free(params);

	int status = 0, ret = 1;
	errno = 0;
	if(waitpid(pid, &status, 0) < 0) {
		E_WARNING(E_STRLOC ": waitpid() failed with '%s'\n", strerror(errno));
		return 1;
	}

	if(WIFEXITED(status)) {
		ret = WEXITSTATUS(status);
	} else if(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
		start_crasher(cmd, SIGSEGV, pid);
	} else {
		E_WARNING(E_STRLOC ": child '%s' killed\n", cmd);
	}

	return ret;
}

static int start_child_process_with_core(const char* cmd) {
#ifndef __minix
	/* 
	 * Minix does not have setrlimit() so we disable everything as by default it
	 * will dump core.
	 */
	struct rlimit r;
	errno = 0;

	if(getrlimit(RLIMIT_CORE, &r) == -1) {
		E_WARNING(E_STRLOC ": gerlimit() failed with '%s'\n", strerror(errno));
		return -1;
	}

	rlim_t old = r.rlim_cur;
	r.rlim_cur = RLIM_INFINITY;

	if(setrlimit(RLIMIT_CORE, &r) == -1) {
		E_WARNING(E_STRLOC ": setrlimit() failed with '%s'\n", strerror(errno));
		return -1;
	}
#endif
	int ret = start_child_process(cmd);

#ifndef __minix
	r.rlim_cur = old;
	if(setrlimit(RLIMIT_CORE, &r) == -1) {
		E_WARNING(E_STRLOC ": setrlimit() failed with '%s'\n", strerror(errno));
		return -1;
	}
#endif

	return ret;
}

static bool start_child(const char *cmd, bool notify = false) {
	E_DEBUG(E_STRLOC ": Starting '%s'\n", cmd);
	StartupNotify *n;

	if(notify) n = startup_notify_start(cmd, "applications-order");

	int ret = start_child_process_with_core(cmd);

	if(notify) startup_notify_end(n);

	if(ret == 199) {
		alert(_("Program '%s' not found.\n\nPlease check if given path to the "
				"executable was correct or adjust $PATH environment variable to "
				"point to the directory where target executable exists"), cmd);

		return false;
	}

	if(ret == EACCES) {
		/* now check the if file is executable since EACCES is common error if not so */
		if(access(cmd, X_OK) != 0)
			alert(_("You are trying to execute '%s', but it is not executable file"), cmd);
		else
			alert(_("You do not have enough permissions to execute '%s'"), cmd);

		return false;
	}

	return true;
}

static bool start_child_in_term(const char *cmd, bool notify = false) {
	String term;

	if(!find_terminal(term)) {
		E_WARNING(E_STRLOC ": unable to find any suitable terminal\n");
		return false;
	}

	/* construct TERM -e cmd */
	term += " -e ";
	term += cmd;
	return start_child(term.c_str(), notify);
}

static bool start_desktop_file(const char *cmd) {
	DesktopFile d;

	if(!d.load(cmd)) {
		alert(_("Unable to load .desktop file '%s'. Got: %s"), cmd, d.strerror());
		return false;
	}

	if(d.type() != DESK_FILE_TYPE_APPLICATION) {
		alert(_("Starting other types of .desktop files except 'Application' is not supported now"));
		return false;
	}

	char buf[PATH_MAX];
	if(!d.exec(buf, PATH_MAX)) {
		alert(_("Unable to run '%s'.\nProbably this file is malformed or 'Exec' key has non-installed program"), cmd);
		return false;
	}

	bool notify = d.startup_notify();
	if(d.terminal())
		return start_child_in_term(buf, notify);

	return start_child(buf, notify);
}

/* concat all arguments preparing it for start_child() */
static void join_args(int start, int argc, char **argv, const char *program, String &ret, bool is_mailto = false) {
	String       args;
	unsigned int alen;
	char         sep = ' ';

	/* append program if given */
	if(program) {
	   	args = program;
		args += ' ';
	}

	/* 'mailto' is handled special */
	if(is_mailto) args += "mailto:";

	for(int i = start; i < argc; i++) {
		args += argv[i];
		args += sep;
	}

	alen = args.length();

	/* remove start/ending quotes and spaces */
	if((args[0] == '"') || isspace(args[0]) || (args[alen - 1] == '"') || isspace(args[alen - 1])) {
		int i;
		char *copy = strdup(args.c_str());
		char *ptr = copy;

		/* remove ending first */
		for(i = (int)alen - 1; i > 0 && (ptr[i] == '"' || isspace(ptr[i])); i--)
			;

		ptr[i + 1] = 0;

		/* remove then starting */
		for(; *ptr && (*ptr == '"' || isspace(*ptr)); ptr++)
			;

		ret = copy;
		free(copy);
	} else {
		ret = args;
	}
}

static void cancel_cb(Fl_Widget*, void* w) {
	LaunchWindow* win = (LaunchWindow*)w;
	win->hide();
}

static void ok_cb(Fl_Widget*, void* w) {
	LaunchWindow* win = (LaunchWindow*)w;
	const char* cmd = dialog_input->value();
	bool started = false;

	win->hide();

	/* do not block dialog when program is starting */
	Fl::check();

	/* TODO: is 'cmd' safe after hide? */
	started = (in_term->value()) ? start_child_in_term(cmd) : start_child(cmd);

	if(!started) {
		/* show dialog again */
		win->show();

		if(cmd) 
			dialog_input->position(0, dialog_input->size());
	} else {
		Resource rc;

		/* so could load previous content */
		rc.load(EDE_LAUNCH_CONFIG);
		rc.set("History", "open", cmd);
		rc.save(EDE_LAUNCH_CONFIG);
	}
}

static int start_dialog(int argc, char** argv) {
	LaunchWindow* win = new LaunchWindow(370, 195, _("Run Command"));
	win->begin();
		Fl_Box* icon = new Fl_Box(10, 10, 55, 55);
		icon->image(image_run);

		Fl_Box* txt = new Fl_Box(70, 10, 290, 69, _("Enter the name of the application you would like to run"));
		txt->align(132|FL_ALIGN_INSIDE);

		dialog_input = new Fl_Input(70, 90, 290, 25, _("Open:"));

		const char *cmd = resource_get("History", "open");
		if(cmd) {
			dialog_input->value(cmd);
			/* make text appear selected */
			dialog_input->position(0, dialog_input->size());
		}

		in_term = new Fl_Check_Button(70, 125, 290, 25, _("Run in terminal"));
		in_term->down_box(FL_DOWN_BOX);

		Fl_Button* ok = new Fl_Button(175, 160, 90, 25, _("&OK"));
		ok->callback(ok_cb, win);
		Fl_Button* cancel = new Fl_Button(270, 160, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb, win);
	win->end();
	win->window_icon(run_xpm);

	window_center_on_screen(win);
	win->show(argc, argv);

	return Fl::run();
}

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

static const char* next_param(int curr, char **argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

int main(int argc, char** argv) {
	EDE_APPLICATION("ede-launch");

	int        ca = 1; /* current argument index */
	const char *cwd, *launch_type;
	cwd = launch_type = 0;

	/* in case if ede-launch launches itself; just skip ourself and use the rest of arguments */
	while(argv[ca] && strstr(argv[ca], "ede-launch") != NULL)
		ca++;

	/* start dialog if we have nothing */
	if(!argv[ca])
		return start_dialog(argc, argv);

	/* parse args and stop as soon as detected first non-parameter value (not counting parameter values) */
	for(; ca < argc; ca++) {
		if(argv[ca][0] != '-') break;

		if(CHECK_ARGV(argv[ca], "-h", "--help")) {
			help();
			return 0;
		}
		
		if(CHECK_ARGV(argv[ca], "-l", "--launch")) {
			launch_type = next_param(ca, argv, argc);
			if(!launch_type) {
				puts("Missing lauch type. Run program with '-h' to see options");
				return 1;
			}

			if(!allowed_launch_type(launch_type)) {
				puts("This is not allowed launch type. Run program with '-h' to see options");
				return 1;
			}

			ca++;
			continue;
		} 

		if(CHECK_ARGV(argv[ca], "-w", "--working-dir")) {
			cwd = next_param(ca, argv, argc);
			if(!cwd) {
				puts("Missing working directory parameter. Run program with '-h' to see options");
				return 1;
			}

			ca++;
			continue;
		}

		printf("Bad parameter '%s'. Run program with '-h' to see options\n", argv[ca]);
		return 1;
	}

	/* make sure we have something to run but only if --launch isn't used */
	if(!argv[ca] && !launch_type) {
		puts("Missing execution parameter(s). Run program with '-h' to see options");
		return 1;
	}

	/* setup working dir */
	if(cwd) {
		errno = 0;
		if(chdir(cwd) != 0) {
			alert(_("Unable to change directory to '%s'. Got '%s' (%i)"), cwd, strerror(errno), errno);
			return 1;
		}
	}
	/* check if we have .desktop file */
	if(argv[ca] && str_ends(argv[ca], ".desktop"))
		return RETURN_FROM_BOOL(start_desktop_file(argv[ca]));

	/* make arguments and exec program */
	String     args;
	const char *prog = NULL;

	if(launch_type) {
		/* explicitly launch what user requested */
		prog = resource_get(KEY_PREFERRED, launch_type);

		if(!prog) {
			E_WARNING(E_STRLOC ": Unable to find out launch type\n");
			return 1;
		}

		join_args(ca, argc, argv, prog, args);
	} else {
		bool mailto = false;

		/* 
		 * guess what arg we got; if got something like web url or mail address, launch preferred apps with it
		 * Note however how this matching works on only one argument; other would be ignored
		 */
		if(re_match(REGEX_PATTERN_MAIL, argv[ca]) > 0) {
			prog = resource_get(KEY_PREFERRED, "mail");
			if(!prog) {
				E_WARNING(E_STRLOC ": Unable to find mail agent\n");
				return 1;
			}

			mailto = true;

			/* use only one argumet */
			argc = ca + 1;
		} else if(re_match(REGEX_PATTERN_URL, argv[ca]) > 0) {
			prog = resource_get(KEY_PREFERRED, "browser");
			if(!prog) {
				E_WARNING(E_STRLOC ": Unable to find browser\n");
				return 1;
			}

			/* use only one argumet */
			argc = ca + 1;
		}

		/* if 'prog' was left to be NULL, argc[ca] will be seen as program to be run */
		join_args(ca, argc, argv, prog, args, mailto);
	}

	return RETURN_FROM_BOOL(start_child(args.c_str()));
}
