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

#include <edelib/Config.h>
#include <edelib/StrUtil.h>
#include <edelib/Debug.h>
#include <edelib/ExpandableGroup.h>
#include <edelib/String.h>
#include <edelib/IconTheme.h>
#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/MessageBox.h>
#include <edelib/Run.h>
#include <edelib/File.h>

typedef edelib::list<edelib::String> StrList;
typedef edelib::list<edelib::String>::iterator StrListIter;

static bool file_can_execute(const edelib::String& f) {
	if(edelib::file_executable(f.c_str()))
		return true;

	/* find full path then */
	edelib::String fp = edelib::file_path(f.c_str());
	if(fp.empty())
		return false;
	return edelib::file_executable(fp.c_str());
}

class ControlButton : public Fl_Button {
	private:
		Fl_Box* tipbox;
		edelib::String tipstr;
		edelib::String exec;
	public:
		ControlButton(Fl_Box* t, const edelib::String& ts, const edelib::String& e, int x, int y, int w, int h, const char* l = 0) : 
		Fl_Button(x, y, w, h, l), tipbox(t), tipstr(ts), exec(e) {
			box(FL_FLAT_BOX);
			align(FL_ALIGN_WRAP);
			color(FL_BACKGROUND2_COLOR);
		}

		~ControlButton() { }
		int handle(int event);
};

int ControlButton::handle(int event) {
	switch(event) {
		case FL_ENTER:
			tipbox->label(tipstr.c_str());
			return 1;

		case FL_LEAVE:
			tipbox->label("");
			return 1;

		case FL_DRAG:
			return 1;

		case FL_PUSH:
			if(Fl::visible_focus() && handle(FL_FOCUS))
				Fl::focus(this);

			box(FL_DOWN_BOX);
			if(Fl::event_clicks()) {
				if(exec.empty())
					edelib::alert(_("Unable to execute command for '%s'. Command value is not set"), label());
				else if(!file_can_execute(exec.c_str()))
					edelib::alert(_("Unable to run program '%s'. Program not found"), exec.c_str());
				else
					edelib::run_program(exec.c_str(), false);
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

static void fetch_icon(ControlButton* btn, const char* path, bool abs) {
	Fl_Image* img = NULL;

	if(abs) {
		img = Fl_Shared_Image::get(path);
	} else {
		edelib::String p = edelib::IconTheme::get(path, edelib::ICON_SIZE_LARGE);
		if(!p.empty()) {
			img = Fl_Shared_Image::get(p.c_str());
		} else {
			// nothing found, try "empty" icon
			p = edelib::IconTheme::get("empty", edelib::ICON_SIZE_LARGE);
			if(!p.empty())
				img = Fl_Shared_Image::get(p.c_str());
		}
	}

	if(img)
		btn->image(img);
}

static void load_buttons(Fl_Group* g, Fl_Box* tipbox) {
	edelib::Config c;

	if(!c.load("ede-conf.conf")) {
		E_WARNING("Can't load config\n");
		return;
	}

	char buff[1024];
	if(!c.get("edeconf", "Items", buff, sizeof(buff))) {
		E_WARNING("Can't find Items key\n");
		return;
	}

	StrList spl;
	// get sections
	edelib::stringtok(spl, buff, ",");
	if(spl.empty())
		return;

	bool abspath;
	const char* section;
	//ControlIcon cicon;
	StrListIter it = spl.begin(), it_end = spl.end();
	edelib::String name, tip, exec;

	for(; it != it_end; ++it) {
		section = (*it).c_str();
		edelib::str_trim((char*)section);

		if(c.get(section, "Name", buff, sizeof(buff)))
			name = buff;
		else {
			E_WARNING("No %s, skipping...\n", section);
			continue;
		}

		if(c.get(section, "Tip", buff, sizeof(buff)))
			tip = buff;
		if(c.get(section, "Exec", buff, sizeof(buff)))
			exec = buff;

		ControlButton* cb = new ControlButton(tipbox, tip, exec, 0, 0, 100, 100);
		cb->copy_label(name.c_str());

		c.get(section, "IconPathAbsolute", abspath, false);

		if(c.get(section, "Icon", buff, sizeof(buff)))
			fetch_icon(cb, buff, abspath);

		g->add(cb);
	}
}

int main(int argc, char** argv) {
	edelib::Window* win = new edelib::Window(455, 330, _("EDE Configuration Place"));
	win->init();
	win->begin();

		/* 
		 * Resizable invisible box.
		 * It is created first so (due stacking order) does not steal FL_ENTER/FL_LEAVE 
		 * events from ControlButton children
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

		edelib::ExpandableGroup* icons = new edelib::ExpandableGroup(10, 60, 435, 225);
		icons->box(FL_DOWN_BOX);
		icons->color(FL_BACKGROUND2_COLOR);
		icons->begin();
		icons->end();

		Fl_Box* tipbox = new Fl_Box(10, 295, 240, 25, _("Double click on a desired item"));
		tipbox->box(FL_FLAT_BOX);
		tipbox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

		load_buttons(icons, tipbox);

		// Fl_Button* options = new Fl_Button(260, 295, 90, 25, _("&Options"));
		Fl_Button* close = new Fl_Button(355, 295, 90, 25, _("&Close"));
		close->callback(close_cb, win);
		Fl::focus(close);
	win->end();
	win->show(argc, argv);
	return Fl::run();
}
