/*
 * $Id$
 *
 * ede-conf, a control panel for EDE
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/fl_draw.H>

#include <edelib/Resource.h>
#include <edelib/StrUtil.h>
#include <edelib/Debug.h>
#include <edelib/ExpandableGroup.h>
#include <edelib/String.h>
#include <edelib/IconLoader.h>
#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/MessageBox.h>
#include <edelib/Run.h>

EDELIB_NS_USING(list)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(ExpandableGroup)
EDELIB_NS_USING(String)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(stringtok)
EDELIB_NS_USING(str_trim)
EDELIB_NS_USING(ICON_SIZE_LARGE)

typedef list<String> StrList;
typedef list<String>::iterator StrListIter;

class ControlButton : public Fl_Button {
private:
	String tipstr;
	String exec;
public:
	ControlButton(const String& ts, const String& e, int x, int y, int w, int h, const char* l = 0);
	int handle(int event);
};

ControlButton::ControlButton(const String& ts, const String& e, int x, int y, int w, int h, const char* l) : 
	Fl_Button(x, y, w, h, l), 
	tipstr(ts), 
	exec(e) 
{
	box(FL_FLAT_BOX);
	align(FL_ALIGN_WRAP);
	color(FL_BACKGROUND2_COLOR);

	if(!tipstr.empty())
		tooltip(tipstr.c_str());
}

int ControlButton::handle(int event) {
	switch(event) {
		case FL_PUSH:
			if(Fl::visible_focus() && handle(FL_FOCUS))
				Fl::focus(this);

			box(FL_DOWN_BOX);
			redraw();

			if(Fl::event_clicks()) {
				if(exec.empty())
					alert(_("Unable to execute command for '%s'. Command value is not set"), label());
				else
					run_async("ede-launch %s", exec.c_str());
			}
			return 1;

		case FL_RELEASE:
			box(FL_FLAT_BOX);
			redraw();
			return 1;
	}

	return Fl_Button::handle(event);
}

static void close_cb(Fl_Widget*, void* w) {
	Fl_Window* win = (Fl_Window*)w;
	win->hide();
}

static void load_buttons(Fl_Group* g) {
	edelib::Resource c;

	if(!c.load("ede-conf")) {
		E_WARNING(E_STRLOC ": Can't load config\n");
		return;
	}

	char buf[256];

	if(!c.get("EdeConf", "items", buf, sizeof(buf))) {
		E_WARNING("Can't find Items key\n");
		return;
	}

	StrList spl;
	/* get sections */
	stringtok(spl, buf, ",");
	if(spl.empty())
		return;

	const char* section;
	StrListIter it = spl.begin(), it_end = spl.end();
	edelib::String name, tip, exec;

	for(; it != it_end; ++it) {
		section = (*it).c_str();
		str_trim((char*)section);

		if(c.get_localized(section, "name", buf, sizeof(buf)))
			name = buf;
		else {
			E_WARNING(E_STRLOC ": No %s, skipping...\n", section);
			continue;
		}

		if(c.get_localized(section, "tip", buf, sizeof(buf)))
			tip = buf;
		if(c.get(section, "exec", buf, sizeof(buf)))
			exec = buf;

		ControlButton* cb = new ControlButton(tip, exec, 0, 0, 100, 100);
		cb->copy_label(name.c_str());

		c.get(section, "icon", buf, sizeof(buf));
		IconLoader::set(cb, buf, ICON_SIZE_LARGE);

		g->add(cb);
	}
}

int main(int argc, char** argv) {
	edelib::Window* win = new edelib::Window(455, 330, _("EDE Configuration Place"));
	win->begin();

		/* 
		 * Resizable invisible box. It is created first so (due stacking order) does not steal 
		 * FL_ENTER/FL_LEAVE events from ControlButton children
		 */
		Fl_Box* rbox = new Fl_Box(10, 220, 120, 65);
		win->resizable(rbox);

		Fl_Group* titlegrp = new Fl_Group(0, 0, 455, 50);
		titlegrp->box(FL_FLAT_BOX);
		titlegrp->color(138);
		titlegrp->begin();
			Fl_Box* title = new Fl_Box(10, 10, 435, 30, win->label());
			title->color(138);
			title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			title->labelcolor(23);
			title->labelfont(FL_HELVETICA_BOLD);
			title->labelsize(16);
		titlegrp->end();
		titlegrp->resizable(title);

		ExpandableGroup* icons = new ExpandableGroup(10, 60, 435, 225);
		icons->box(FL_DOWN_BOX);
		icons->color(FL_BACKGROUND2_COLOR);
		icons->begin();
		icons->end();

		/*
		Fl_Box* tipbox = new Fl_Box(10, 295, 240, 25, _("Double click on a desired item"));
		tipbox->box(FL_FLAT_BOX);
		tipbox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
		tipbox->hide();
		*/

		load_buttons(icons);

		/* Fl_Button* options = new Fl_Button(260, 295, 90, 25, _("&Options")); */
		Fl_Button* close = new Fl_Button(355, 295, 90, 25, _("&Close"));
		close->callback(close_cb, win);
		Fl::focus(close);
	win->end();
	win->show(argc, argv);
	return Fl::run();
}
