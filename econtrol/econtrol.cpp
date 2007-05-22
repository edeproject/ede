
#include "econtrol.h"

#include <edelib/Config.h>
#include <edelib/StrUtil.h>
#include <edelib/Debug.h>
#include <edelib/Nls.h>

#include <fltk/run.h>
#include <fltk/SharedImage.h>
#include <fltk/Rectangle.h>
#include <fltk/events.h>
#include <fltk/layout.h>
#include <fltk/damage.h>
#include <fltk/draw.h> // measure

using namespace edelib;
using namespace fltk;

ControlButton::ControlButton(InvisibleBox* t, String tv, int x, int y, int w, int h, const char* l) : 
Button(x, y, w, h, l) {
	tip = t;
	tipval = tv;
	//box(UP_BOX);
	box(FLAT_BOX);
	align(ALIGN_WRAP|ALIGN_CLIP|ALIGN_TOP);
	//align(ALIGN_WRAP);
	color(WHITE);
}

ControlButton::~ControlButton() {
}

void ControlButton::draw(void) {
	draw_box();

	int iw = 0;
	int ih = 0;

	if(image()) {
		((fltk::Image*)image())->measure(iw, ih);
		int ix = (w()/2)-(iw/2);
		int iy = 5;
		((fltk::Image*)image())->draw(ix, iy);

		Rectangle r(0, ih+10, w(),h()); 
		box()->inset(r);
		drawtext(label(), r, flags());
	} else
		draw_label();
}

int ControlButton::handle(int event) {
	switch(event) {
		case ENTER:
			tip->label(tipval.c_str());
			tip->redraw_label();
			return 1;
		case LEAVE:
			tip->label("");
			tip->redraw_label();
			return 1;
		case PUSH:
			/*color(fltk::GRAY85);
			box(fltk::DOWN_BOX);
			redraw();*/
			return 1;
		case RELEASE:
			/*color(fltk::WHITE);
			box(fltk::FLAT_BOX);
			redraw();*/
			return 1;
		default:
			return Button::handle(event);
	}
	return Button::handle(event);
}

void close_cb(Widget*, void* w) {
	ControlWin* cw = (ControlWin*)w;
	cw->do_close();
}

ControlWin::ControlWin(const char* title, int w, int h) : Window(w, h, title) {

	IconTheme::init("edeneu");

	register_images();
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

	vector<String> spl;
	stringtok(spl, buff, ",");
	char* sect;

	ControlIcon cicon;
	for(unsigned int i = 0; i < spl.size(); i++) {
		sect = (char*)spl[i].c_str();
		str_trim(sect);

		if(c.get(sect, "Name", buff, sizeof(buff)))
			cicon.name = buff;
		else {
			EWARNING("No %s, skipping...\n", spl[i].c_str());
			continue;
		}

		if(c.get(sect, "Tip", buff, sizeof(buff)))
			cicon.tip = buff;
		if(c.get(sect, "Exec", buff, sizeof(buff)))
			cicon.exec = buff;
		if(c.get(sect, "Icon", buff, sizeof(buff))) {
			cicon.icon = buff;
			EDEBUG("setting icon (%s): %s\n", spl[i].c_str(), cicon.icon.c_str());
		}

		c.get(spl[i].c_str(), "IconPathAbsolute", cicon.abspath, false);
		c.get(spl[i].c_str(), "Position", cicon.abspath, -1);

		iconlist.push_back(cicon);
	}
}

void ControlWin::init(void) {
	begin();
		titlegrp = new Group(0, 0, 455, 50);
		titlegrp->box(FLAT_BOX);
		titlegrp->color((Color)0x5271a200);
		titlegrp->begin();
			title = new InvisibleBox(10, 10, 435, 30, label());
			title->color((Color)0x5271a200);
			title->align(ALIGN_LEFT|ALIGN_INSIDE);
			title->labelcolor((Color)0xffffff00);
			title->labelfont(HELVETICA_BOLD);
			title->labelsize(16);
		titlegrp->end();
		titlegrp->resizable(title);

		icons = new ExpandableGroup(10, 60, 435, 225);
		icons->box(DOWN_BOX);
		icons->color((Color)0xffffff00);

		tipbox = new InvisibleBox(10, 295, 240, 25, _("Double click on desired item"));
		tipbox->box(FLAT_BOX);
		tipbox->align(ALIGN_LEFT|ALIGN_INSIDE|ALIGN_CLIP);

		for(unsigned int i = 0; i < iconlist.size(); i++) {
			ControlButton* b = new ControlButton(tipbox, iconlist[i].tip, 0, 0, 80, 100);
			//String iconpath = IconTheme::get(iconlist[i].icon.c_str(), ICON_SIZE_MEDIUM);
			String iconpath = IconTheme::get(iconlist[i].icon.c_str(), ICON_SIZE_LARGE);

			//EDEBUG("%s\n", iconpath.c_str());

			b->label(iconlist[i].name.c_str());

			if(!iconpath.empty())
				b->image(SharedImage::get(iconpath.c_str()));
			icons->add(b);
		}

		options = new Button(260, 295, 90, 25, _("&Options"));
		close = new Button(355, 295, 90, 25, _("&Close"));
		close->callback(close_cb, this);

		// resizable invisible box
		rbox = new InvisibleBox(10, 220, 120, 65);
		resizable(rbox);
	end();
}

void ControlWin::do_close(void) {
	hide();
}

int main() {
	ControlWin cw(_("EDE Control Panel"));
	cw.show();
	return run();
}
