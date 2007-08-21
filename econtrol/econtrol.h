#ifndef __ECONTROL_H__
#define __ECONTROL_H__

#include <FL/Fl_Window.h>
#include <FL/Fl_Group.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Button.h>

#include <edelib/IconTheme.h>
#include <edelib/ExpandableGroup.h>

struct ControlIcon {
	edelib::String name;
	edelib::String tip;
	edelib::String exec;
	edelib::String icon;
	bool abspath;
	int  pos;
};

class ControlButton : public Fl_Button {
	private:
		Fl_Box* tip;
		edelib::String tipval;
	public:
		ControlButton(Fl_Box* t, edelib::String tv, int x, int y, int w, int h, const char* l = 0);
		~ControlButton();
		int handle(int event);
};

class ControlWin : public Fl_Window {
	private:
		Fl_Group* titlegrp;
		Fl_Box* title;
		Fl_Button* close;
		//Fl_Button* options;
		edelib::ExpandableGroup* icons;
		Fl_Box* rbox;
		Fl_Box* tipbox;

		edelib::list<ControlIcon> iconlist;

		void init(void);
		void load_icons(void);

	public:
		ControlWin(const char* title, int w = 455, int h = 330);
		~ControlWin();
		void do_close(void);
};


#endif
