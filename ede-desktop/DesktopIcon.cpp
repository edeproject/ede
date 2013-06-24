/*
 * $Id$
 *
 * Copyright (C) 2006-2013 Sanel Zukan
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FL_LIBRARY 1

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include <edelib/IconLoader.h>
#include <edelib/MenuItem.h>
#include <edelib/MessageBox.h>
#include <edelib/Nls.h>
#include <edelib/Run.h>

#include "DesktopIcon.h"
#include "MovableIcon.h"
#include "Desktop.h"

/* label offset from icon y() + h(), so selection box can be drawn nicely */
#define LABEL_OFFSET 2

/* minimal icon sizes */
#define ICON_SIZE_MIN_W 48
#define ICON_SIZE_MIN_H 48

/* spaces around box in case of large/small icons */
#define OFFSET_W 16
#define OFFSET_H 16

EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(String)
EDELIB_NS_USING(MenuButton)
EDELIB_NS_USING(MenuItem)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(ask)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(input)
EDELIB_NS_USING(ICON_SIZE_TINY)
EDELIB_NS_USING(ICON_SIZE_HUGE)

static void open_cb(Fl_Widget*, void* d);
static void edit_cb(Fl_Widget*, void* d);
static void delete_cb(Fl_Widget*, void* d);

static MenuItem icon_menu[] = {
	{_("&Open"),   0, open_cb, 0},
	{_("&Edit"),   0, edit_cb, 0},
	{_("&Delete"), 0, delete_cb, 0},
	{0}
};

#if 0
static MenuItem icon_trash_menu[] = {
	{_("&Open"),       0, 0},
	{_("&Properties"), 0, 0, 0, FL_MENU_DIVIDER},
	{_("&Empty"),      0, 0},
	{0}
};
#endif

static void open_cb(Fl_Widget*, void* d) {
	DesktopIcon *o = (DesktopIcon*)d;
	run_async("ede-launch %s", o->get_cmd());
}

static void edit_cb(Fl_Widget*, void* d) {
	DesktopIcon *di = (DesktopIcon*)d;
	((Desktop*)di->parent())->edit_icon(di);
}

# if 0
static void rename_cb(Fl_Widget*, void* d) {
	DesktopIcon *di = (DesktopIcon*)d;

	const char *new_name = input(_("Change desktop icon name to:"), di->label());
	if(new_name) {
		bool saved = ((Desktop*)di->parent())->rename_icon(di, new_name);
		if(!saved)
			alert(_("Unable to rename this icon. Please check if you have enough permissions to do so"));
	}
}
#endif

static void delete_cb(Fl_Widget*, void* d) {
	DesktopIcon *di = (DesktopIcon*)d;

	if(ask(_("This icon will be permanently deleted. Are you sure?")))
		((Desktop*)di->parent())->remove_icon(di, true);
}

DesktopIcon::DesktopIcon(const char *l, int W, int H) : Fl_Widget(1, 1, W, H, l) {
	box(FL_FLAT_BOX);
	color(FL_RED);
	label(l);
	align(FL_ALIGN_WRAP);
	
	/* default values unless given explicitly */
	labelfont(FL_HELVETICA);
	labelsize(12);
	
	darker_img = 0;
	micon = 0;
	lwidth = lheight = 0;
	focused = false;
	imenu = 0;

	icon_menu[2].image((Fl_Image*)IconLoader::get("edit-delete", ICON_SIZE_TINY));
}

DesktopIcon::~DesktopIcon() {
	delete darker_img;
	delete micon;
	delete imenu;
}

void DesktopIcon::set_image(const char *name) {
	E_RETURN_IF_FAIL(IconLoader::inited());

	/* if not name give, use false name to trick IconLoader to use fallback icons */
	if(!name) name = "THIS-ICON-DOES-NOT-EXISTS";
	E_RETURN_IF_FAIL(IconLoader::set(this, name, ICON_SIZE_HUGE));

	/* fetch image object for sizes */
	Fl_Image *img = image();

	int img_w = img->w(),
		img_h = img->h();

	/* resize box if icon is larger */
	if(img_w > ICON_SIZE_MIN_W || img_h > ICON_SIZE_MIN_H)
		size(img_w + OFFSET_W, img_h + OFFSET_H);
}

void DesktopIcon::update_label_font_and_size(void) {
	E_RETURN_IF_FAIL(opts != 0);

	labelfont(opts->label_font);
	labelsize(opts->label_fontsize);
    lwidth  = opts->label_maxwidth;
    lheight = 0;
	
	/*
	 * make sure current font size/type is set (internaly to fltk)
	 * so fl_measure() can correctly calculate label width and height
	 */
	int old = fl_font(), old_sz = fl_size();

	fl_font(labelfont(), labelsize());
	fl_measure(label(), lwidth, lheight, align());
	fl_font(old, old_sz);

    lwidth  += 12;
	lheight += 5;
}

void DesktopIcon::fix_position(int X, int Y) {
	int dx = parent()->x(),
		dy = parent()->y(),
		dw = parent()->w(),
		dh = parent()->h();

	if(X < dx) X = dx;
	if(Y < dy) Y = dy;
	if(X + w() > dw) X = (dx + dw) - w();
	if(Y + h() > dh) Y = (dy + dh) - h();

	position(X, Y);
}

void DesktopIcon::drag(int x, int y, bool apply) {
	if(!micon) {
		micon = new MovableIcon(this);
#if HAVE_SHAPE
		/* 
		 * This is used to calculate correct window startup/ending
		 * position since icon is placed in the middle of the box.
		 */
		int ix = 0, iy = 0;
		if(image()) {
			ix = (w()/2) - (image()->w()/2);
			iy = (h()/2) - (image()->h()/2);
			
			/* include parent offset since x/y are absolute locations */
			if(parent()) {
				ix += parent()->x();
				iy += parent()->y();
			}
		}

		micon->position(micon->x() + ix, micon->y() + iy);
#endif
		micon->show();
	} else {
		E_ASSERT(micon != NULL);
		micon->position(x, y);
	}

	if(apply) {
#if HAVE_SHAPE
		int ix = 0, iy = 0;
		if(image()) {
			ix = (w()/2) - (image()->w()/2);
			iy = (h()/2) - (image()->h()/2);

			/* also take into account offsets, as below we subtract it */
			if(parent()) {
				ix += parent()->x();
				iy += parent()->y();
			}
		}

		fix_position(micon->x() - ix, micon->y() - iy);
#else
		fix_position(micon->x(), micon->y());
#endif
		delete micon;
		micon = NULL;
	}
}

/* used only in Desktop::move_selection */
int DesktopIcon::drag_icon_x(void) {
	if(!micon)
		return x();
	else 
		return micon->x();
}

/* used only in Desktop::move_selection */
int DesktopIcon::drag_icon_y(void) {
	if(!micon)
		return y();
	else
		return micon->y();
}
	
void DesktopIcon::fast_redraw(void) {
	int wsz = w();
	int xpos = x();

	if(lwidth > w()) {
		wsz = lwidth + 4;
		xpos = x() - 4;
	}

	/* LABEL_OFFSET + 2 include selection box line height too; same for xpos */
	parent()->damage(FL_DAMAGE_ALL, xpos, y(), wsz, h() + lheight + LABEL_OFFSET + 2);
}

void DesktopIcon::draw(void) {
	if(image() && (damage() & FL_DAMAGE_ALL)) {
		Fl_Image *im = image();

		/* center image in the box */
		int ix = (w()/2) - (im->w()/2);
		int iy = (h()/2) - (im->h()/2);
		ix += x();
		iy += y();

		if(is_focused()) {
			/* create darker image only when needed */
			if(!darker_img) {
				darker_img = im->copy(im->w(), im->h());
				darker_img->color_average(FL_BLUE, 0.6);
			}

			darker_img->draw(ix, iy);
		} else {
			im->draw(ix, iy);
		}
	}
	
	if(opts && opts->label_visible && (damage() & (FL_DAMAGE_ALL | EDE_DESKTOP_DAMAGE_CHILD_LABEL))) {
		int X = x() + w()-(w()/2)-(lwidth/2);
		int Y = y() + h() + LABEL_OFFSET;

		Fl_Color old = fl_color();

		if(!opts->label_transparent) {
			fl_color(opts->label_background);
			fl_rectf(X, Y, lwidth, lheight);
		}

		int old_font = fl_font();
		int old_font_sz = fl_size();

		/* draw with icon's font */
		fl_font(labelfont(), labelsize());
		
		/* pseudo-shadow */
		fl_color(FL_BLACK);
		fl_draw(label(), X+1, Y+1, lwidth, lheight, align(), 0, 0);

		fl_color(opts->label_foreground);
		fl_draw(label(), X, Y, lwidth, lheight, align(), 0, 0);

		/* restore old font */
		fl_font(old_font, old_font_sz);

		if(is_focused()) {
			/* draw focused box on our way so later this can be used to draw customised boxes */
			fl_color(opts->label_foreground);
			fl_line_style(FL_DOT);

			fl_push_matrix();
			fl_begin_loop();
				fl_vertex(X, Y);
				fl_vertex(X + lwidth, Y);
				fl_vertex(X + lwidth, Y + lheight);
				fl_vertex(X, Y + lheight);
				fl_vertex(X, Y);
			fl_end_loop();
			fl_pop_matrix();

			/* revert to default line style */
			fl_line_style(0);
		}

		/* revert to old color whatever that be */
		fl_color(old);
	}
}

int DesktopIcon::handle(int event) {
	switch(event) {
		case FL_FOCUS:
		case FL_UNFOCUS:
		case FL_ENTER:
		case FL_LEAVE:
		/* We have to handle FL_MOVE too, if want to get only once FL_ENTER when entered or FL_LEAVE when leaved */
		case FL_MOVE:
			return 1;

		case FL_PUSH:
			if(Fl::event_button() == 3) {
				/*
				 * init icon specific menu only when needed
				 * TODO: since menu for all icons is mostly the same, it can be shared; Desktop can
				 * have menu instance seeded to all icons
				 */
				if(!imenu) {
					imenu = new MenuButton(0, 0, 0, 0);
					imenu->menu(icon_menu);
				}

				/* MenuItem::popup() by default does not call callbacks */
				const MenuItem *m = imenu->menu()->popup(Fl::event_x(), Fl::event_y());

				/* call menu callbacks, passing correct parameters */
				if(m && m->callback())
					m->do_callback(0, this);
			}
			return 1;

		case FL_RELEASE:
			if(Fl::event_clicks() > 0 && !cmd.empty())
				run_async("ede-launch %s", cmd.c_str());
			return 1;

		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_LEAVE:
			return 1;

		case FL_DND_RELEASE:
			E_DEBUG(E_STRLOC ": FL_DND_RELEASE on icon\n");
			return 1;
		case FL_PASTE:
			E_DEBUG(E_STRLOC ": FL_PASTE on icon with %s\n", Fl::event_text());
			return 1;
		default:
			break;
	}

	return 0;
}
