#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <edelib/Debug.h>
#include "PagerButton.h"

PagerButton::~PagerButton() {
	if(ttip)
		free(ttip);
}

void PagerButton::set_workspace_label(int l) {
	wlabel = l;

	char buf[6];
	snprintf(buf, sizeof(buf), "%i", l);
	copy_label(buf);
}

void PagerButton::copy_tooltip(const char *t) {
	E_RETURN_IF_FAIL(t != NULL);

	if(ttip) 
		free(ttip);

	ttip = strdup(t);
	tooltip(ttip);
}

void PagerButton::select_it(int i) {
	if(i) {
		color((Fl_Color)44);
		labelcolor(FL_BLACK);
	} else {
		color((Fl_Color)33);
		labelcolor(FL_GRAY);
	}

	redraw();
}
