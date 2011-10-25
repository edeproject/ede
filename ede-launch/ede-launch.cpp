/*
 * $Id$
 *
 * ede-launch, launch external application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
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
#include <edelib/Ede.h>
#include "StartupNotify.h"

#include "icons/run.xpm"

/* so value could be directed returned to shell */
#define RETURN_FROM_BOOL(v) (v != false)

/* config name where all things are stored and read from */
#define EDE_LAUNCH_CONFIG "ede-launch"

EDELIB_NS_USING_LIST(11, (Resource, String, DesktopFile, RES_USER_ONLY, DESK_FILE_TYPE_APPLICATION, run_sync, run_async, alert, file_path, window_center_on_screen, str_ends))
EDELIB_NS_USING_AS(Window, LaunchWindow)

static Fl_Pixmap        image_run((const char**)run_xpm);
static Fl_Input*        dialog_input;
static Fl_Check_Button* in_term;

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
	puts("   ede-launch --launch mail mailto: foo@foo.com");
	puts("   ede-launch gvim");
	puts("   ede-launch ~/Desktop/foo.desktop");
}

static char* get_basename(const char* path) {
	char *p = (char*)strrchr(path, '/');
	if(p)
		return (p + 1);

	return (char*)p;
}

static char** cmd_split(const char* cmd) {
	int   sz = 10;
	int   i = 0;
	char* c = strdup(cmd);

	char** arr = (char**)malloc(sizeof(char*) * sz);

	for(char* p = strtok(c, " "); p; p = strtok(NULL, " ")) {
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

static void start_crasher(const char* cmd, int sig) {
	const char* base = get_basename(cmd);
	const char* ede_app_flag = "";

	/* this means the app was called without full path */
	if(!base) base = cmd;

	/* 
	 * determine is our app by checking the prefix; we don't want user to send bug reports about crashes 
	 * of foreign applications
	 */
	if(strncmp(base, "ede-", 4) == 0)
		ede_app_flag = "--edeapp";

	/* call edelib implementation instead start_child_process() to prevents loops if 'ede-crasher' crashes */
	run_sync(PREFIX "/bin/ede-crasher %s --appname %s --apppath %s --signal %i", ede_app_flag, base, cmd, sig);
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
		start_crasher(cmd, SIGSEGV);
	} else {
		E_WARNING(E_STRLOC ": child '%s' killed\n", cmd);
	}

	return ret;
}

static int start_child_process_with_core(const char* cmd) {
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

	int ret = start_child_process(cmd);

	r.rlim_cur = old;
	if(setrlimit(RLIMIT_CORE, &r) == -1) {
		E_WARNING(E_STRLOC ": setrlimit() failed with '%s'\n", strerror(errno));
		return -1;
	}

	return ret;
}

static bool start_child(const char* cmd) {
	E_DEBUG(E_STRLOC ": Starting '%s'\n", cmd);

	StartupNotify *n = startup_notify_start(cmd, "applications-order");
	int ret = start_child_process_with_core(cmd);

	startup_notify_end(n);

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

static bool start_desktop_file(const char *cmd) {
	DesktopFile d;

	if(!d.load(cmd)) {
		alert(_("Unable to load .desktop file '%s'. Got: %s"), cmd, d.strerror());
		goto FAIL;
	}

	if(d.type() != DESK_FILE_TYPE_APPLICATION) {
		alert(_("Starting other types of .desktop files except 'Application' is not supported now"));
		goto FAIL;
	}

	char buf[PATH_MAX];
	if(!d.exec(buf, PATH_MAX)) {
		alert(_("Unable to run '%s'.\nProbably this file is malformed or 'Exec' key has non-installed program"), cmd);
		goto FAIL;
	}

	return start_child(buf);

FAIL:
	return false;
}

/* concat all arguments preparing it for start_child() */
static void join_args(int start, int argc, char **argv, const char *program, String &ret) {
	String       args;
	unsigned int alen;

	/* append program if given */
	if(program) {
	   	args = program;
		args += ' ';
	}

	for(int i = start; i < argc; i++) {
		args += argv[i];
		args += ' ';
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

#define RETURN_IF_VALID_TERM(t, r) \
do { \
	if(t && ((strcmp(t, "linux") != 0) || (strcmp(t, "dumb") != 0))) { \
		r = file_path(t, false); \
		if(E_UNLIKELY(r.empty())) return true; \
	} \
} while(0)

static bool find_terminal(String &ret) {
	/* before goint to list, try to read it from config file */
	Resource rc;
	if(rc.load(EDE_LAUNCH_CONFIG)) {
		char buf[64];
		if(rc.get("Preferred", "terminal", buf, sizeof(buf))) {
			ret = buf;
			return true;
		}
	}

	/* list of known terminals */
	static const char *terms[] = {
		"xterm",
		"rxvt",
		"Terminal",
		"gnome-terminal",
		"konsole",
		0
	};

	const char* term = getenv("TERM");

	RETURN_IF_VALID_TERM(term, ret);

	term = getenv("COLORTERM");
	RETURN_IF_VALID_TERM(term, ret);

	for(int i = 0; terms[i]; i++) {
		term = terms[i];
		RETURN_IF_VALID_TERM(term, ret);
	}

	return false;
}

static void ok_cb(Fl_Widget*, void* w) {
	LaunchWindow* win = (LaunchWindow*)w;
	const char* cmd = dialog_input->value();
	bool started = false;

	win->hide();

	/* do not block dialog when program is starting */
	Fl::check();

	/* TODO: is 'cmd' safe after hide? */
	if(in_term->value()) {
		char buf[128];
		String term;

		if(find_terminal(term)) {
			snprintf(buf, sizeof(buf), "%s -e %s", term.c_str(), cmd);
			started = start_child(buf);
		} else {
			E_WARNING(E_STRLOC ": unable to find any suitable terminal\n");
		}
	} else {
		started = start_child(cmd);
	}

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

		Resource rc;
		char     buf[128];

		if(rc.load(EDE_LAUNCH_CONFIG) && rc.get("History", "open", buf, sizeof(buf))) {
			dialog_input->value(buf);

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

	/* start dialog if we have nothing */
	if(argc <= 1)
		return start_dialog(argc, argv);

	int        ca = 1; /* current argument index */
	const char *cwd, *launch_type;
	cwd = launch_type = 0;

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
	String args;
	if(launch_type) {
		Resource rc;
		char     buf[64];

		if(!rc.load(EDE_LAUNCH_CONFIG) || !rc.get("Preferred", launch_type, buf, sizeof(buf))) {
			E_WARNING("Unable to find out launch type. Balling out...\n");
			return 1;
		}

		join_args(ca, argc, argv, buf, args);
	} else {
		join_args(ca, argc, argv, 0, args);
	}

	return RETURN_FROM_BOOL(start_child(args.c_str()));
}
