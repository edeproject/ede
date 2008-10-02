#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_show_colormap.H>
#include <FL/x.H>

#include <stdio.h>
#include <string.h>

Fl_Button* color_button = 0;
Fl_Input*  txt = 0;

void color_cb(Fl_Widget*, void*) {
	Fl_Color c = fl_show_colormap(color_button->color());
	color_button->color(c);
}

void send_cb(Fl_Widget*, void*) {
	printf("send: %s color: %i\n", txt->value(), color_button->color());

	Atom _XA_EDE_DESKTOP_NOTIFY = XInternAtom(fl_display, "_EDE_DESKTOP_NOTIFY", False);
	Atom _XA_EDE_DESKTOP_NOTIFY_COLOR = XInternAtom(fl_display, "_EDE_DESKTOP_NOTIFY_COLOR", False);
	//Atom _XA_UTF8_STRING = XInternAtom(fl_display, "UTF8_STRING", false);

	// max size
	unsigned char txt_send[8192];
	int i;
	const char* txt_val = txt->value() ? txt->value() : "(none)";
	int len = strlen(txt_val);

	for(i = 0; i < 8192-2 && i < len; i++) 
		txt_send[i] = txt_val[i];

	txt_send[i] = '\0';

	// send text
	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_EDE_DESKTOP_NOTIFY, XA_STRING, 8, PropModeReplace, 
			txt_send, i + 1);

	// send color
	int col = color_button->color();
	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_EDE_DESKTOP_NOTIFY_COLOR, XA_CARDINAL, 32, PropModeReplace, 
			(unsigned char*)&col, sizeof(int));
}

int main() {
	fl_open_display();

	Fl_Double_Window* win = new Fl_Double_Window(295, 144, "Notify test");
	win->begin();
		txt = new Fl_Input(10, 15, 275, 25);
		txt->align(FL_ALIGN_TOP_LEFT);

		color_button = new Fl_Button(260, 50, 25, 25, "Color");
		color_button->align(FL_ALIGN_LEFT);
		color_button->callback(color_cb);
		Fl_Box* bx = new Fl_Box(10, 50, 164, 85, "Type some text and choose color, then press Send. "
		"Desktop should get notified about this.");
		bx->align(FL_ALIGN_WRAP);

		Fl_Button* send_button = new Fl_Button(195, 110, 90, 25, "&Send");
		send_button->callback(send_cb);
	win->end();
	win->show();
	return Fl::run();
}
