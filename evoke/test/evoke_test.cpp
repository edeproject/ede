#include <FL/Fl.h>
#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Button.h>
#include <FL/x.h>

#include <string.h>

Fl_Input* inp;
Fl_Double_Window* win;

void run_cb(Fl_Widget*, void*) {
	Atom _XA_EDE_EVOKE_SPAWN = XInternAtom(fl_display, "_EDE_EVOKE_SPAWN", False);
	// max size
	unsigned char txt_send[8192];
	int i;
	const char* txt_val = inp->value() ? inp->value() : "(none)";
	int len = strlen(txt_val);

	for(i = 0; i < 8192-2 && i < len; i++)
		txt_send[i] = txt_val[i];

	txt_send[i] = '\0';

	// send text
	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen),
			_XA_EDE_EVOKE_SPAWN, XA_STRING, 8, PropModeReplace,
			txt_send, i + 1);
}

void evoke_quit_cb(Fl_Widget*, void*) {
	Atom _XA_EDE_EVOKE_QUIT = XInternAtom(fl_display, "_EDE_EVOKE_QUIT", False);

	int dummy = 1;
	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen),
            _XA_EDE_EVOKE_QUIT, XA_CARDINAL, 32, PropModeReplace,
            (unsigned char*)&dummy, sizeof(int));
}

void quit_all_cb(Fl_Widget*, void*) {
	Atom _XA_EDE_EVOKE_SHUTDOWN_ALL = XInternAtom(fl_display, "_EDE_EVOKE_SHUTDOWN_ALL", False);

	int dummy = 1;
	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen),
            _XA_EDE_EVOKE_SHUTDOWN_ALL, XA_CARDINAL, 32, PropModeReplace,
            (unsigned char*)&dummy, sizeof(int));
}

int main(int argc, char **argv) {
	win = new Fl_Double_Window(355, 94, "Evoke test");
	win->begin();
		inp = new Fl_Input(10, 25, 335, 25, "Program to run:");
		inp->align(FL_ALIGN_TOP_LEFT);
		Fl_Button* b1 = new Fl_Button(255, 60, 90, 25, "&Run");
		b1->callback(run_cb);
		Fl_Button* b2 = new Fl_Button(150, 60, 90, 25, "&Quit evoke");
		b2->callback(evoke_quit_cb);
		Fl_Button* b3 = new Fl_Button(55, 60, 90, 25, "&Quit all");
		b3->callback(quit_all_cb);
    win->end();
	win->show(argc, argv);
	return Fl::run();
}
