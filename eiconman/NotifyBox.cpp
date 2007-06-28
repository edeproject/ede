/*
 * $Id$
 *
 * Eiconman, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "NotifyBox.h"
#include "eiconman.h"
#include <edelib/Debug.h>

#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <FL/Fl_Group.h>

#define MAX_LABEL_WIDTH 200

#define TIMEOUT     (1.0f/60.0f)
#define TIME_SHOWN  3.0f

NotifyBox::NotifyBox(int aw, int ah) : Fl_Window(0, 0, 10, 10) { 
	area_w = aw;
	area_h = ah;
	lwidth = lheight = 0;
	is_shown = false;

	clear_border();
	set_non_modal();

	begin();
		txt_box = new Fl_Box(0, 0, w(), h());
		txt_box->box(FL_BORDER_BOX);
		txt_box->color(FL_WHITE);
		txt_box->align(FL_ALIGN_WRAP);
	end();
}

NotifyBox::~NotifyBox() {
	EDEBUG("NotifyBox::~NotifyBox()\n");
}

void NotifyBox::update_label_size(void) {
    lwidth = MAX_LABEL_WIDTH;
    lheight= 0;

	fl_font(txt_box->labelfont(), txt_box->labelsize());
	fl_measure(txt_box->label(), lwidth, lheight, txt_box->align());

    lwidth  += 10;
	lheight += 10;
}

void NotifyBox::resize_all(void) {
	update_label_size();

	// center box
	int x_pos = (area_w/2) - (lwidth/2);
	resize(x_pos, 0, lwidth, lheight);
	txt_box->resize(0, 0, w(), h());
}

void NotifyBox::show(void) {
	if(shown())
		return;

	EDEBUG(ESTRLOC ": %i %i\n", x(), y());
	resize_all();

	Fl_Window::show();
	is_shown = true;
	Fl::add_timeout(TIME_SHOWN, visible_timeout_cb, this);
}

void NotifyBox::hide(void) {
	if(!shown())
		return;

	Fl_Window::hide();
	is_shown = false;
	Fl::remove_timeout(visible_timeout_cb);
	Fl::remove_timeout(animate_cb);
}

void NotifyBox::label(const char* l) {
	txt_box->label(l);
	resize_all();
}

void NotifyBox::copy_label(const char* l) {
	txt_box->copy_label(l);
	resize_all();
}

const char* NotifyBox::label(void) {
	return txt_box->label();
}

void NotifyBox::animate(void) {
	if(y() > -lheight) {
		position(x(), y() - 1);
		Fl::repeat_timeout(TIMEOUT, animate_cb, this);
	} else
		hide();
}

void NotifyBox::visible_timeout(void) {
	Fl::remove_timeout(visible_timeout_cb);
	Fl::add_timeout(TIMEOUT, animate_cb, this);
}
