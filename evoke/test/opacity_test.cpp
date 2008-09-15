#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/x.H>

#include <stdio.h>

Fl_Window* win;
Fl_Value_Slider* slider;
Atom opacity_atom;

void slider_cb(Fl_Widget*, void*) {
	printf("callback %i\n", (int)slider->value());
	//int v = (int)slider->value();
	//unsigned int v = (unsigned int)0xe0000000;
	unsigned int v = (unsigned int)4;
	XChangeProperty(fl_display, fl_xid(win), opacity_atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&v, 1);
	XFlush(fl_display);
}

int main(void) {
	fl_open_display();

	opacity_atom = XInternAtom(fl_display, "_NET_WM_WINDOW_OPACITY", False);

	win = new Fl_Window(240, 175, "Opacity test");
	win->begin();
		Fl_Box* sbox = new Fl_Box(10, 9, 220, 102, "sample text");
		sbox->box(FL_ENGRAVED_BOX);
		sbox->color((Fl_Color)186);
		sbox->labelcolor((Fl_Color)23);

		slider = new Fl_Value_Slider(10, 142, 220, 18, "Opacity percent:");
		slider->type(1);
		slider->step(1);
		slider->maximum(100);
		slider->minimum(1);
		slider->value(100);
		slider->callback(slider_cb);
		slider->align(FL_ALIGN_TOP_LEFT);
	win->end();
	win->show();
	slider_cb(0,0);
	return Fl::run();
}
