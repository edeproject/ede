#include "Applet.h"
#include <FL/Fl_Button.H>
#include <FL/x.H>
#include <FL/Fl.H>
#include <stdio.h>

class MyButton : public Fl_Button {
public:
	MyButton() : Fl_Button(0, 0, 90, 25, "xxx") { 
		color(FL_RED);
		puts("MyButton::MyButton");
	}

	~MyButton() {
		puts("MyButton::~MyButton");
	}
};

EDE_PANEL_APPLET_EXPORT (
 MyButton, 
 0,
 "Panel button",
 "0.1",
 "empty",
 "Sanel Zukan"
)
