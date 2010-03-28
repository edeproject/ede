#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <edelib/Ede.h>

#include "Panel.h"
#include "AppletManager.h"

static bool running;

static void exit_signal(int signum) {
	running = false;
}

int main(int argc, char **argv) {
	EDE_APPLICATION("ede-panel");

	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT,  exit_signal);
	
	Panel *panel = new Panel();
	panel->load_applets();
	panel->show();
	running = true;

	while(running)
		Fl::wait();

	/* so Panel::hide() can be called */
	panel->hide();
	return 0;
}
