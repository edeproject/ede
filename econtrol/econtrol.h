#ifndef __ECONTROL_H__
#define __ECONTROL_H__

#include <fltk/Window.h>
#include <fltk/Group.h>
#include <fltk/InvisibleBox.h>
#include <fltk/Button.h>

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

class ControlButton : public fltk::Button {
	private:
		fltk::InvisibleBox* tip;
		edelib::String tipval;
	public:
		ControlButton(fltk::InvisibleBox* t, edelib::String tv, int x, int y, int w, int h, const char* l = 0);
		~ControlButton();
		void draw(void);
		int handle(int event);
};

class ControlWin : public fltk::Window {
	private:
		fltk::Group* titlegrp;
		fltk::InvisibleBox* title;
		fltk::Button* close;
		fltk::Button* options;
		edelib::ExpandableGroup* icons;
		fltk::InvisibleBox* rbox;
		fltk::InvisibleBox* tipbox;

		edelib::vector<ControlIcon> iconlist;

		void init(void);
		void load_icons(void);

	public:
		ControlWin(const char* title, int w = 455, int h = 330);
		~ControlWin();
		void do_close(void);
};


#endif
