#ifndef __NOTIFYWINDOW_H__
#define __NOTIFYWINDOW_H__

#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Multiline_Output.H>

/* just keep it greater than FL_WINDOW or FL_DOUBLE_WINDOW */
#define NOTIFYWINDOW_TYPE 0xF9

class NotifyWindow : public Fl_Window {
private:
	int       id, exp;
	Fl_Button *closeb;
	Fl_Box    *imgbox;
	Fl_Multiline_Output *summary, *body;

public:
	NotifyWindow();

	void set_id(int i) { id = i; }
	int  get_id(void) { return id; }

	void set_icon(const char *img);
	void set_summary(const char *s) { summary->value(s); }
	void set_body(const char *s) { body->value(s); }

	/*
	 * match to spec: if is -1, then we handle it, if is 0, then window will not be closed and
	 * the rest is sender specific
	 */
	void set_expire(int t) { exp = t; }
	void show(void);

	virtual void resize(int X, int Y, int W, int H);
};

#endif
