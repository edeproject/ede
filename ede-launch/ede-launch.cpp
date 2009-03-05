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

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pixmap.H>
#include <edelib/Run.h>
#include <edelib/Window.h>
#include <edelib/Nls.h>
#include <edelib/Debug.h>
#include <edelib/MessageBox.h>
#include "icons/run.xpm"

/* 
 * Window from X11 is alread included with Fl.H so we can't use 
 * EDELIB_NS_USING(Window) here. Stupid C++ namespaces
 */
#define LaunchWindow edelib::Window

EDELIB_NS_USING(run_sync)
EDELIB_NS_USING(alert)

static Fl_Pixmap        image_run(run_xpm);
static Fl_Input*        dialog_input;
static Fl_Check_Button* in_term;

void help(void) {
	puts("Usage: ede-launch [OPTIONS] program");
	puts("EDE program launcher");
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

static void start_crasher(const char* cmd, int sig) {
	/* 
	 * call edelib implementation instead start_child_process()
	 * to prevents loops if 'ede-crasher' crashes
	 */
	run_sync("ede-crasher --apppath %s --signal %i", cmd, sig);
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
			/* perent */
			close(in[0]);
			close(out[1]);
			close(err[1]);
			break;
	}

	int status, ret;
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
	int ret = start_child_process_with_core(cmd);

	if(ret == 199) {
		alert(_("Program '%s' not found"), cmd);
		return false;
	}

	if(ret == EACCES) {
		alert(_("You do not have enough permissions to execute '%s'"), cmd);
		return false;
	}

	return true;
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
	if(in_term->value()) {
		char buff[128];
		char* term = getenv("TERM");
		if(!term)
			term = "xterm";

		snprintf(buff, sizeof(buff), "%s -e %s", term, cmd);
		started = start_child(buff);
	} else {
		started = start_child(cmd);
	}

	if(!started) {
		/* show dialog again */
		win->show();
	}
}

static void start_dialog(int argc, char** argv) {
	LaunchWindow* win = new LaunchWindow(370, 195, _("Run Command"));
	win->begin();
		Fl_Box* icon = new Fl_Box(10, 10, 55, 55);
		icon->image(image_run);

		Fl_Box* txt = new Fl_Box(70, 10, 290, 69, _("Enter the name of the application you would like to run or the URL you would like to view"));
		txt->align(132|FL_ALIGN_INSIDE);

		dialog_input = new Fl_Input(70, 90, 290, 25, _("Open:"));

		in_term = new Fl_Check_Button(70, 125, 290, 25, _("Run in terminal"));
		in_term->down_box(FL_DOWN_BOX);

		Fl_Button* ok = new Fl_Button(175, 160, 90, 25, _("&OK"));
		ok->callback(ok_cb, win);
		Fl_Button* cancel = new Fl_Button(270, 160, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb, win);
	win->end();
	win->show(argc, argv);

	while(win->shown())
		Fl::wait();
}

int main(int argc, char** argv) {
	if(argc == 1)
		start_dialog(argc, argv);
	else if(argc != 2) {
		help();
		return 0;
	} else {
		start_child(argv[1]);
	}

	return 0;
}
