
#include "econtrol.h"

#include <edelib/Config.h>
#include <edelib/StrUtil.h>
#include <edelib/Debug.h>
#include <edelib/Nls.h>

#include <FL/Fl.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/fl_draw.h>

using namespace edelib;

ControlButton::ControlButton(Fl_Box* t, String tv, int x, int y, int w, int h, const char* l) : 
Fl_Button(x, y, w, h, l) {
	tip = t;
	tipval = tv;
	box(FL_FLAT_BOX);
	align(FL_ALIGN_WRAP);
	color(FL_WHITE);
}

ControlButton::~ControlButton() {
}

int ControlButton::handle(int event) {
	switch(event) {
		case FL_ENTER:
			tip->label(tipval.c_str());
			return 1;
		case FL_LEAVE:
			tip->label("");
			return 1;
		case FL_PUSH:
			return 1;
		case FL_RELEASE:
			return 1;
		default:
			return Fl_Button::handle(event);
	}
	return Fl_Button::handle(event);
}

void close_cb(Fl_Widget*, void* w) {
	ControlWin* cw = (ControlWin*)w;
	cw->do_close();
}

ControlWin::ControlWin(const char* title, int w, int h) : Fl_Window(w, h, title) {

	IconTheme::init("edeneu");

	fl_register_images();
	load_icons();
	init();
}

ControlWin::~ControlWin() {
	IconTheme::shutdown();
}

void ControlWin::load_icons(void) {
	Config c;

	if(!c.load("econtrol.conf")) {
		EWARNING("Can't load config\n");
		return;
	}

	char buff[1024];
	if(!c.get("Control Panel", "Items", buff, sizeof(buff))) {
		EWARNING("Can't find Items key\n");
		return;
	}

	list<String> spl;
	list<String>::iterator it, it_end;
	stringtok(spl, buff, ",");

	if(spl.empty())
		return;

	const char* sect;
	ControlIcon cicon;
	it = spl.begin();
	it_end = spl.end();

	for(; it != it_end; ++it) {
		sect = (*it).c_str();
		str_trim((char*)sect);

		if(c.get(sect, "Name", buff, sizeof(buff)))
			cicon.name = buff;
		else {
			EWARNING("No %s, skipping...\n", sect);
			continue;
		}

		if(c.get(sect, "Tip", buff, sizeof(buff)))
			cicon.tip = buff;
		if(c.get(sect, "Exec", buff, sizeof(buff)))
			cicon.exec = buff;
		if(c.get(sect, "Icon", buff, sizeof(buff))) {
			cicon.icon = buff;
			EDEBUG("setting icon (%s): %s\n", sect, cicon.icon.c_str());
		}

		c.get(sect, "IconPathAbsolute", cicon.abspath, false);
		c.get(sect, "Position", cicon.abspath, -1);
	
		iconlist.push_back(cicon);
	}
}

void ControlWin::init(void) {
	begin();
		titlegrp = new Fl_Group(0, 0, 455, 50);
		titlegrp->box(FL_FLAT_BOX);
		titlegrp->color(138);
		titlegrp->begin();
			title = new Fl_Box(10, 10, 435, 30, label());
			title->color(138);
			title->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			title->labelcolor(23);
			title->labelfont(FL_HELVETICA_BOLD);
			title->labelsize(16);
		titlegrp->end();
		titlegrp->resizable(title);

		icons = new ExpandableGroup(10, 60, 435, 225);
		icons->box(FL_DOWN_BOX);
		icons->color(FL_BACKGROUND2_COLOR);
		icons->end();

		tipbox = new Fl_Box(10, 295, 240, 25, _("Double click on desired item"));
		tipbox->box(FL_FLAT_BOX);
		tipbox->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);

		list<ControlIcon>::iterator it, it_end;
		it = iconlist.begin();
		it_end = iconlist.end();

		for(; it != it_end; ++it) {
			ControlButton* b = new ControlButton(tipbox, (*it).tip, 0, 0, 80, 100);
			String iconpath = IconTheme::get((*it).icon.c_str(), ICON_SIZE_LARGE);
			b->label((*it).name.c_str());

			if(!iconpath.empty())
				b->image(Fl_Shared_Image::get(iconpath.c_str()));
			icons->add(b);
		}

		//options = new Fl_Button(260, 295, 90, 25, _("&Options"));
		close = new Fl_Button(355, 295, 90, 25, _("&Close"));
		close->callback(close_cb, this);

		// resizable invisible box
		rbox = new Fl_Box(10, 220, 120, 65);
		resizable(rbox);
	end();
}

void ControlWin::do_close(void) {
	hide();
}

int main() {
	ControlWin cw(_("EDE Control Panel"));
	cw.show();
	return Fl::run();
}
