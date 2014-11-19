/*
 * $Id$
 *
 * Copyright (C) 2012 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <FL/Fl_RGB_Image.H>

#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/MenuItem.h>
#include <edelib/IconLoader.h>
#include <edelib/Netwm.h>

#include "TaskButton.h"
#include "Taskbar.h"
#include "icons/window.xpm"

#define TASKBUTTON_ICON_W 16
#define TASKBUTTON_ICON_H 16

EDELIB_NS_USING(MenuItem)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(ICON_SIZE_TINY)
EDELIB_NS_USING(netwm_window_close)
EDELIB_NS_USING(netwm_window_set_active)
EDELIB_NS_USING(netwm_window_get_title)
EDELIB_NS_USING(netwm_window_get_icon)
EDELIB_NS_USING(netwm_window_set_state)
EDELIB_NS_USING(NETWM_STATE_ACTION_TOGGLE)
EDELIB_NS_USING(NETWM_STATE_MAXIMIZED)

EDELIB_NS_USING(wm_window_set_state)
EDELIB_NS_USING(WM_WINDOW_STATE_ICONIC)

static Fl_Pixmap image_window(window_xpm);

static void close_cb(Fl_Widget*, void*);
static void restore_cb(Fl_Widget*, void*);
static void minimize_cb(Fl_Widget*, void*);
static void maximize_cb(Fl_Widget*, void*);

static MenuItem menu_[] = {
	{_("Restore"), 0, restore_cb, 0},
	{_("Minimize"), 0, minimize_cb, 0},
	{_("Maximize"), 0, maximize_cb, 0, FL_MENU_DIVIDER},
	{_("Close"), 0, close_cb, 0},
	{0}
};

static void redraw_whole_panel(TaskButton *b) {
	/* Taskbar specific member */
	Taskbar *tb = (Taskbar*)b->parent();
	tb->panel_redraw();
}

static void close_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;
	netwm_window_close(bb->get_window_xid());
	/* no need to redraw whole panel since taskbar elements are recreated again */
}

static void restore_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;

	netwm_window_set_active(bb->get_window_xid(), 1);
	redraw_whole_panel(bb);
}

static void minimize_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;

	/* WM_WINDOW_STATE_ICONIC is safer on other window managers than NETWM_STATE_HIDDEN */
	wm_window_set_state(bb->get_window_xid(), WM_WINDOW_STATE_ICONIC);
}

static void maximize_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;

	netwm_window_set_active(bb->get_window_xid(), 1);
	netwm_window_set_state(bb->get_window_xid(), NETWM_STATE_MAXIMIZED, NETWM_STATE_ACTION_TOGGLE);

	redraw_whole_panel(bb);
}

TaskButton::TaskButton(int X, int Y, int W, int H, const char *l) : Fl_Button(X, Y, W, H, l),
																	xid(0), wspace(0), old_value(0), image_alloc(false), dragged(false), net_wm_icon(0)
{ 
	box(FL_UP_BOX);

	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);

	if(IconLoader::inited()) {
		Fl_Shared_Image *img = IconLoader::get("process-stop", ICON_SIZE_TINY);
		menu_[3].image((Fl_Image*)img);
	}

	net_wm_icon = XInternAtom(fl_display, "_NET_WM_ICON", False);
	image(image_window);
}

TaskButton::~TaskButton() {
	clear_image();
}

void TaskButton::clear_image(void) {
	if(image_alloc && image())
		delete image();

	image(NULL);
	image_alloc = false;
}	

void TaskButton::draw(void) {
	Fl_Color col = value() ? selection_color() : color();
	draw_box(value() ? (down_box() ? down_box() : fl_down(box())) : box(), col);

	if(image()) {
		int X, Y, lw, lh;

		X = x() + 5;
		Y = (y() + h() / 2) - (image()->h() / 2);
		image()->draw(X, Y);

		X += image()->w() + 5;

		if(label()) {
			fl_font(labelfont(), labelsize());
			fl_color(labelcolor());

			lw = lh = 0;
			fl_measure(label(), lw, lh, 0);

			/* use clipping so long labels do not be drawn on the right border, which looks ugly */
			fl_push_clip(x() + Fl::box_dx(box()), 
						 y() + Fl::box_dy(box()), 
						 w() - Fl::box_dw(box()) - 5, 
						 h() - Fl::box_dh(box()));

			Y = (y() + h() / 2) - (lh / 2);
			fl_draw(label(), X, Y, lw, lh, align(), 0, 0);

			fl_pop_clip();
		}
	} else {
		draw_label();
	}

	if(Fl::focus() == this)
		draw_focus();
}

void TaskButton::display_menu(void) {
	const char *t = tooltip();

	/* do not popup tooltip when the menu is on */
	tooltip(NULL);

	/* parameters for callbacks; this is done here, since menu_ is static and shared between buttons */
	menu_[0].user_data(this);
	menu_[1].user_data(this);
	menu_[2].user_data(this);
	menu_[3].user_data(this);

	const MenuItem *item = menu_->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
	if(item && item->callback())
		item->do_callback(this);

	tooltip(t);
}

void TaskButton::update_title_from_xid(void) {
	E_RETURN_IF_FAIL(xid >= 0);

	char *title = netwm_window_get_title(xid);
	if(!title) {
		label("...");
		tooltip("...");
	} else {
		copy_label(title);
		tooltip(label());
		free(title);
	}
}

void TaskButton::update_image_from_xid(void) {
	E_RETURN_IF_FAIL(xid >= 0);

	Fl_RGB_Image *img = netwm_window_get_icon(xid, TASKBUTTON_ICON_W);
	if(!img) return;
	
	int width = img->w(), height = img->h();

	/* some safety, scale it if needed */
	if((width  > TASKBUTTON_ICON_W) || (height > TASKBUTTON_ICON_H)) {
		width  = (width  > TASKBUTTON_ICON_W) ? TASKBUTTON_ICON_W : width;
		height = (height > TASKBUTTON_ICON_H) ? TASKBUTTON_ICON_H : height;

		/* safe casting */
		Fl_RGB_Image *scaled = (Fl_RGB_Image*)img->copy(width, height);
		delete img;

		img = scaled;
	}

	clear_image();
	image(img);
	image_alloc = true;
}

int TaskButton::handle(int e) {
	switch(e) {
		case FL_PUSH:
			/*
			 * Remember old value, as value() result affect will box be drawn as FL_UP_BOX or FL_DOWN_BOX.
			 * This trick should return box to the old state in case of dragging.
			 */
			old_value = value();
			return Fl_Button::handle(e);
		case FL_DRAG:
			dragged = true;
			return 1;
		case FL_RELEASE:
			if(dragged) {
				Taskbar *taskbar = (Taskbar*)parent();
				taskbar->try_dnd(this, Fl::event_x(), Fl::event_y());
				dragged = false;

				value(old_value);
				return 1;
			}
			/* fallthrough */
	}
	
	return Fl_Button::handle(e);
}
