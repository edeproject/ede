#include <FL/x.H>
#include <FL/Fl.H>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define CMDS_NUM 11

const char* cmds[] = {
	"fluid",
	"/this/command/does/not/exists",
	"ls",
	"ls -la",
	"/home/sanel/abc",
	"/home/sanel/readme.txt",
	"mozilla",
	"firefox",
	"xedit",
	"/usr/bin/xedit",
	"gvim"
};

void run_cmd(const char* cmd, Atom _XA_EDE_EVOKE_SPAWN) {
	// max size
	unsigned char txt_send[8192];
	int i;
	int len = strlen(cmd);

	for(i = 0; i < 8192-2 && i < len; i++)
		txt_send[i] = cmd[i];

	txt_send[i] = '\0';

	// send text
	if(XChangeProperty(fl_display, RootWindow(fl_display, fl_screen),
			_XA_EDE_EVOKE_SPAWN, XA_STRING, 8, PropModeReplace,
			txt_send, i + 1) == Success) { 
		puts("Success");
	} else {
		puts("Fail");
	}
}

int main(int argc, char **argv) {
	fl_open_display();
	srand(time(0));

	Atom _XA_EDE_EVOKE_SPAWN = XInternAtom(fl_display, "_EDE_EVOKE_SPAWN", False);

	for(int i = 0, j = 0; i < 5; i++) {
		j = rand() % CMDS_NUM;

		printf("%i (%s)\n", i, cmds[j]);
		sleep(1);
		run_cmd(cmds[j], _XA_EDE_EVOKE_SPAWN);

		XFlush(fl_display);
	}

	return 0;
}
