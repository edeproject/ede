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

#include <edelib/Debug.h>
#include <edelib/IconTheme.h>
#include <edelib/Nls.h>
#include <edelib/Item.h>
#include <edelib/Ask.h>

#include <fltk/SharedImage.h>
#include <fltk/Color.h>
#include <fltk/Divider.h>
#include <fltk/draw.h>
#include <fltk/events.h>
#include <fltk/damage.h>

#include <fltk/x11.h> // Pixmap

// fuck !
#define Window XWindow
  #include <X11/extensions/shape.h>
#undef Window

// default icon size
#define ICONSIZE 48

// spaces around box in case of large/small icons
#define OFFSET_W 16
#define OFFSET_H 16

void icon_delete_cb(fltk::Widget*, void* di) {
	DesktopIcon* dicon = (DesktopIcon*)di;
	edelib::ask(_("Delete '%s' ?"), dicon->label());
}

DesktopIcon::DesktopIcon(GlobalIconSettings* gisett, IconSettings* isett, int icon_type) :
	fltk::Widget(isett->x, isett->y, ICONSIZE, ICONSIZE), settings(NULL) 
{		
	EASSERT(gisett != NULL); EASSERT(isett != NULL);

	type = icon_type;

	lwidth = lheight = 0;
	focus = false;
	micon = false;
	
	image_w = image_h = ICONSIZE;

	/*
	 * GlobalIconSettings is shared from desktop so we only
	 * reference it. On other hand IconSettings is not shared
	 * and we must construct a copy from given parameter
	 */
	globals = gisett;

	settings = new IconSettings;
	settings->name = isett->name;
	settings->cmd  = isett->cmd;
	settings->icon = isett->icon;

	// x,y are not needed since x(), y() are filled with it
	
	label(settings->name.c_str());

	if(!settings->icon.empty()) {
		const char* nn = settings->icon.c_str();

		edelib::IconSizes isize = edelib::ICON_SIZE_MEDIUM;
		edelib::String ipath = edelib::IconTheme::get(nn, isize);

		/*
		 * Resize box according to size from IconSizes.
		 *
		 * Here is avoided image()->measure() since looks like
		 * that function messes up icon's alpha blending if is widget
		 * resizes (but not icon). This is a bug in fltk.
		 */
		if(isize >= ICONSIZE) {
			image_w = isize;
			image_h = isize;

			resize(image_w + OFFSET_W, image_h + OFFSET_H);
		}

		nn = ipath.c_str();
		image(fltk::SharedImage::get(nn));
	}

	EDEBUG(ESTRLOC ": Got label: %s\n", label());
	EDEBUG(ESTRLOC ": Got image: %s\n", settings->icon.c_str());
	EDEBUG(ESTRLOC ": Got x/y  : %i %i\n", x(), y());

	align(fltk::ALIGN_WRAP);
	update_label_size();

	pmenu = new fltk::PopupMenu(0, 0, 100, 100);
	pmenu->begin();
		if(type == ICON_TRASH) {
			edelib::Item* it = new edelib::Item(_("&Open"));
			it->offset_x(12, 12);
			it = new edelib::Item(_("&Properties"));
			it->offset_x(12, 12);
			new fltk::Divider();
			it = new edelib::Item(_("&Empty"));
			it->offset_x(12, 12);
		} else {
			edelib::Item* it = new edelib::Item(_("&Open"));
			it->offset_x(12, 12);
			it = new edelib::Item(_("&Rename"));
			it->offset_x(12, 12);
			it = new edelib::Item(_("&Delete"));
			it->offset_x(12, 12);
			it->callback(icon_delete_cb, this);
			new fltk::Divider();
			it = new edelib::Item(_("&Properties"));
			it->offset_x(12, 12);
		}
	pmenu->end();
	pmenu->type(fltk::PopupMenu::POPUP3);
}

DesktopIcon::~DesktopIcon() 
{
	if(settings) {
		EDEBUG(ESTRLOC ": IconSettings clearing from ~DesktopIcon()\n");
		delete settings;
	}

	if(micon)
		delete micon;

	if(pmenu)
		delete pmenu;
}

void DesktopIcon::update_label_size(void) {
    lwidth = globals->label_maxwidth;
    lheight= 0;

	// FIXME: really needed ???
	fltk::setfont(labelfont(), labelsize());

	//fltk::measure(label(), lwidth, lheight, fltk::ALIGN_WRAP);
	fltk::measure(label(), lwidth, lheight, flags());

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
		micon = 0;
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

void DesktopIcon::draw(void) {
	if(image() && (damage() & fltk::DAMAGE_ALL)) {
		fltk::Image* ii = (fltk::Image*)image();
		int ix = (w()/2) - (ii->w()/2);
		/*
		 * Make sure to pickup image's w() and h() since if fltk is compiled 
		 * with Xrender it will be scaled to rectangle's w()/h(). 
		 *
		 * On other hand, if fltk is _not_ compiled with Xrender, 
		 * scaling will not be done.
		 * Yuck !
		 */
		fltk::Rectangle ir(ix, 5, ii->w(), ii->h());
		ii->draw(ir);
	}

	if(globals->label_draw && (damage() & (fltk::DAMAGE_ALL | fltk::DAMAGE_CHILD_LABEL))) {
		int X = w()-(w()/2)-(lwidth/2);
		int Y = h()+2;

		if(!globals->label_transparent) {
			fltk::setcolor(globals->label_background);
			fltk::fillrect(X, Y, lwidth, lheight);
		}

		Rectangle r(X, Y, lwidth, lheight); 

		fltk::setcolor(globals->label_foreground);
		drawtext(label(), r, flags());

		if(is_focused()) {
			/* 
			 * draw focused box on our way so later
			 * this can be used to draw customised boxes
			 */
			fltk::line_style(fltk::DOT);

			fltk::push_matrix();

			//fltk::setcolor(fltk::WHITE);
			fltk::setcolor(globals->label_foreground);
			fltk::addvertex(X,Y);
			fltk::addvertex(X+lwidth,Y);
			fltk::addvertex(X+lwidth,Y+lheight);
			fltk::addvertex(X,Y+lheight);
			fltk::addvertex(X,Y);
			fltk::strokepath();

			fltk::pop_matrix();

			// revert to default line style
			fltk::line_style(0);
		}

		set_damage(damage() & ~fltk::DAMAGE_CHILD_LABEL);
	}
}

int DesktopIcon::handle(int event) {
	switch(event) {
		case fltk::FOCUS:
		case fltk::UNFOCUS:
			return 1;

		case fltk::ENTER:
		case fltk::LEAVE:
			return 1;
		/* 
		 * We have to handle fltk::MOVE too, if
		 * want to get only once fltk::ENTER when
		 * entered or fltk::LEAVE when leaved.
		 */
		case fltk::MOVE:
			return 1;
		case fltk::PUSH:
			if(fltk::event_button() == 3)
				pmenu->popup();
			return 1;
		case fltk::RELEASE:
			if(fltk::event_clicks() > 0)
				EDEBUG(ESTRLOC ": EXECUTE: %s\n", settings->cmd.c_str());
			return 1;

		case fltk::DND_ENTER:
			EDEBUG("Icon DND_ENTER\n");
			return 1;
		case fltk::DND_DRAG:
			EDEBUG("Icon DND_DRAG\n");
			return 1;
		case fltk::DND_LEAVE:
			EDEBUG("Icon DND_LEAVE\n");
			return 1;
		case fltk::DND_RELEASE:
			EDEBUG("Icon DND_RELEASE\n");
			return 1;
	}

	return fltk::Widget::handle(event);
}

MovableIcon::MovableIcon(DesktopIcon* ic) : fltk::Window(ic->x(), ic->y(), ic->w(), ic->h()), icon(ic) {
	EASSERT(icon != NULL);

	set_override();
	create();

	fltk::Image* img = icon->icon_image();
	if(img)
		image(img);
#if 0
	if(img) {
#ifdef SHAPE
		Pixmap mask = create_pixmap_mask(img->width(), img->height());
		XShapeCombineMask(fltk::xdisplay, fltk::xid(this), ShapeBounding, 0, 0, mask, ShapeSet);
#else
		image(img);
#endif
	}
#endif
}

MovableIcon::~MovableIcon()
{
}
