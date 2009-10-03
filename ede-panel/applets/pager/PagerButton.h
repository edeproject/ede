#ifndef __PAGERBUTTON_H__
#define __PAGERBUTTON_H__

#include <FL/Fl_Button.H>

class PagerButton : public Fl_Button {
private:
	char *ttip;
	int   wlabel;
public:
	PagerButton(int X, int Y, int W, int H, const char *l = 0) : Fl_Button(X, Y, W, H), ttip(NULL), wlabel(0)
	{ }
	~PagerButton();

	void set_workspace_label(int l);
	int  get_workspace_label(void) { return wlabel; }

	void copy_tooltip(const char *t);
	void select_it(int i);
};

#endif
