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

#include "DesktopIcon.h"
#include "eiconman.h"
#include "Utils.h"

#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/x.h>

#include <edelib/Debug.h>
#include <edelib/IconTheme.h>

#include <X11/extensions/shape.h>

// minimal icon size
#define ICONSIZE 48

// spaces around box in case of large/small icons
#define OFFSET_W 16
#define OFFSET_H 16

// label offset from icon y()+h(), so selection box can be drawn nicely
#define LABEL_OFFSET 2

DesktopIcon::DesktopIcon(GlobalIconSettings* gs, IconSettings* is, int bg) : 
	Fl_Button(is->x, is->y, ICONSIZE, ICONSIZE) {

	EASSERT(gs != NULL);

	lwidth = lheight = 0;
	focus = false;
	micon = NULL;

	/*
	 * GlobalIconSettings is shared from desktop so we only
	 * reference it. On other hand IconSettings is not shared
	 * and we must construct a copy from given parameter
	 */
	globals = gs;

	settings = new IconSettings;
	settings->name = is->name;
	settings->cmd  = is->cmd;
	settings->icon = is->icon;
	settings->type = is->type;
	settings->key_name= is->key_name;

	// x,y are not needed since x(), y() are filled with it
	
	label(settings->name.c_str());

	if(!settings->icon.empty()) {
		const char* nn = settings->icon.c_str();

		edelib::String ipath = edelib::IconTheme::get(nn, edelib::ICON_SIZE_MEDIUM);
		Fl_Image* img = Fl_Shared_Image::get(ipath.c_str());
		if(img) {
			int img_w = img->w();
			int img_h = img->h();

			// resize box if icon is larger
			if(img_w > ICONSIZE || img_h > ICONSIZE)
				size(img_w + OFFSET_W, img_h + OFFSET_H);

			image(img);
		} else 
			EDEBUG(ESTRLOC ": Unable to load %s\n", ipath.c_str());
	}
/*
	EDEBUG(ESTRLOC ": Got label: %s\n", label());
	EDEBUG(ESTRLOC ": Got image: %s\n", settings->icon.c_str());
	EDEBUG(ESTRLOC ": Got x/y  : %i %i\n", x(), y());
*/
	//Use desktop color as color for icon background
	color(bg);

	align(FL_ALIGN_WRAP);
	update_label_size();
}

DesktopIcon::~DesktopIcon() { 
	EDEBUG("DesktopIcon::~DesktopIcon()\n");

	if(settings)
		delete settings;
	if(micon)
		delete micon;
}

void DesktopIcon::update_label_size(void) {
    lwidth = globals->label_maxwidth;
    lheight= 0;

	/*
	 * make sure current font size/type is set (internaly to fltk)
	 * so fl_measure() can correctly calculate label width and height
	 */
	fl_font(labelfont(), labelsize());

	fl_measure(label(), lwidth, lheight, align());

    lwidth += 8;
	lheight += 4;
}

void DesktopIcon::drag(int x, int y, bool apply) {
	if(!micon) {
		micon = new MovableIcon(this);
		micon->show();
	}

	EASSERT(micon != NULL);

	micon->position(x, y);

	if(apply) {
		position(micon->x(), micon->y());
		delete micon;
		micon = NULL;
	}
}

// Used only in Desktop::move_selection
int DesktopIcon::drag_icon_x(void) {
	if(!micon)
		return x();
	else 
		return micon->x();
}

// Used only in Desktop::move_selection
int DesktopIcon::drag_icon_y(void) {
	if(!micon)
		return y();
	else
		return micon->y();
}

void DesktopIcon::fast_redraw(void) {
	EASSERT(parent() != NULL && "Impossible !");

	// LABEL_OFFSET + 2 include selection box line height too
	parent()->damage(FL_DAMAGE_ALL, x(), y(), w(), h() + lheight + LABEL_OFFSET + 2);
}

void DesktopIcon::draw(void) { 
	//draw_box(FL_UP_BOX, FL_BLACK);

	if(image() && (damage() & FL_DAMAGE_ALL)) {
		Fl_Image* im = image();

		// center image in the box
		int ix = (w()/2) - (im->w()/2);
		int iy = (h()/2) - (im->h()/2);
		ix += x();
		iy += y();

		im->draw(ix, iy);
	}

	if(globals->label_draw && (damage() & (FL_DAMAGE_ALL))) {
		int X = x() + w()-(w()/2)-(lwidth/2);
		int Y = y() + h() + LABEL_OFFSET;

		Fl_Color old = fl_color();

		if(!globals->label_transparent) {
			fl_color(globals->label_background);
			fl_rectf(X, Y, lwidth, lheight);
		}

		fl_color(globals->label_foreground);
		fl_draw(label(), X, Y, lwidth, lheight, align(), 0, 0);

		if(is_focused()) {
			/* 
			 * draw focused box on our way so later
			 * this can be used to draw customised boxes
			 */
			fl_line_style(FL_DOT);

			fl_color(globals->label_foreground);

			fl_push_matrix();
			fl_begin_loop();
				fl_vertex(X,Y);
				fl_vertex(X+lwidth,Y);
				fl_vertex(X+lwidth,Y+lheight);
				fl_vertex(X,Y+lheight);
				fl_vertex(X,Y);
			fl_end_loop();
			fl_pop_matrix();

			// revert to default line style
			fl_line_style(0);
		}

		// revert to old color whatever that be
		fl_color(old);
	}
}

int DesktopIcon::handle(int event) {
	return Fl_Button::handle(event);
}

MovableIcon::MovableIcon(DesktopIcon* ic) : Fl_Window(ic->x(), ic->y(), ic->w(), ic->h()), icon(ic) {
	EASSERT(icon != NULL);

	set_override();
	color(ic->color());

	begin();
		icon_box = new Fl_Box(0, 0, w(), h());
		icon_box->image(ic->icon_image());
	end();
}

MovableIcon::~MovableIcon() {
}

void MovableIcon::show(void) {
	if(!shown())
		Fl_X::make_xid(this);
#if 0
	Pixmap mask = create_mask((Fl_RGB_Image*)icon->icon_image());
	XShapeCombineMask(fl_display, fl_xid(this), ShapeBounding, 0, 0, mask, ShapeSet);
#endif
}