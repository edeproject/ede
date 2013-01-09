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

#include "Applet.h"

#include <time.h>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <edelib/MenuBase.h>
#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/Debug.h>
#include <edelib/StrUtil.h>

#include "XdgMenuReader.h"
#include "ede-icon.h"

/* by default is enabled */
#define EDE_PANEL_MENU_AUTOUPDATE 1

#ifdef EDE_PANEL_MENU_AUTOUPDATE
 #include <edelib/DirWatch.h>
 EDELIB_NS_USING(DirWatch)
 EDELIB_NS_USING_LIST(4, (DW_CREATE, DW_MODIFY, DW_DELETE, DW_REPORT_RENAME))

 /* when menu needs to be update, after how long to do real update */
 #define MENU_UPDATE_TIMEOUT 5.0

 /* elapsed seconds between changes reports from DirWatch; to prevent event throttling */
 #define MENU_UPDATE_DIFF 5
#endif

EDELIB_NS_USING(MenuBase)
EDELIB_NS_USING(str_ends)

/* some of this code was ripped from Fl_Menu_Button.cxx */
class StartMenu : public MenuBase {
private:
	XdgMenuContent *mcontent, *mcontent_pending;

	time_t   last_reload;
	bool     menu_opened;

	void setup_menu(XdgMenuContent *m);
public:
	StartMenu();
	~StartMenu();

	void popup(void);
	void draw(void);
	int  handle(int e);

	void reload_menu(void);
	bool can_reload(void);
};

#ifdef EDE_PANEL_MENU_AUTOUPDATE
static void menu_update_cb(void *data) {
	StartMenu *m = (StartMenu*)data;
	m->reload_menu();
	E_DEBUG(E_STRLOC ": Scheduled menu update done\n");
}

static void folder_changed_cb(const char *dir, const char *w, int flags, void *data) {
	StartMenu *m = (StartMenu*)data;

	/* skip file renaming */
	if(flags == DW_REPORT_RENAME) return;

	if(w == NULL) w = "<none>";

	/* add timeout for update so all filesystem changes gets applied */
	if(str_ends(w, ".desktop") && m->can_reload()) {
		E_DEBUG(E_STRLOC ": Scheduled menu update due changes inside inside '%s' folder ('%s':%i) in %i secs.\n", dir, w, flags, MENU_UPDATE_TIMEOUT);
		Fl::add_timeout(MENU_UPDATE_TIMEOUT, menu_update_cb, m);
	}
}
#endif

StartMenu::StartMenu() : MenuBase(0, 0, 80, 25, "EDE"), mcontent(NULL), mcontent_pending(NULL), last_reload(0), menu_opened(false) {
	down_box(FL_NO_BOX);
	labelfont(FL_HELVETICA_BOLD);
	labelsize(14);
	image(ede_icon_image);

	tooltip(_("Click here to choose and start common programs"));

	/* load menu */
	mcontent = xdg_menu_load();
	setup_menu(mcontent);

#ifdef EDE_PANEL_MENU_AUTOUPDATE	
	/*
	 * setup listeners on menu folders
	 * TODO: this list is constructed twice: the first time when menu was loaded and now
	 */
	StrList lst;
	xdg_menu_applications_location(lst);

	DirWatch::init();

	StrListIt it = lst.begin(), ite = lst.end();
	for(; it != ite; ++it)
		DirWatch::add(it->c_str(), DW_CREATE | DW_MODIFY | DW_DELETE);

	DirWatch::callback(folder_changed_cb, this);
#endif
}

StartMenu::~StartMenu() {
	if(mcontent)         xdg_menu_delete(mcontent);
	if(mcontent_pending) xdg_menu_delete(mcontent_pending);

#ifdef EDE_PANEL_MENU_AUTOUPDATE
	DirWatch::shutdown();
#endif
}

void StartMenu::setup_menu(XdgMenuContent *m) {
	if(m == NULL) {
		menu(NULL);
		return;
	}

	MenuItem *item = xdg_menu_to_fltk_menu(m);

	/* skip the first item, since it often contains only one submenu */
	if(item && item->submenu()) {
		menu(item + 1);
	} else {
		menu(item);
	}
}

static StartMenu *pressed_menu_button = 0;

void StartMenu::draw(void) {
	if(!box() || type()) 
		return;

	draw_box(pressed_menu_button == this ? fl_down(box()) : box(), color());

	if(image()) {
		int X, Y, lw, lh;

		X = x() + 5;
		Y = (y() + h() / 2) - (image()->h() / 2);
		image()->draw(X, Y);

		X += image()->w() + 10;

		fl_font(labelfont(), labelsize());
		fl_color(labelcolor());
		fl_measure(label(), lw, lh, align());

		fl_draw(label(), X, Y, lw, lh, align(), 0, 0);
	} else {
		draw_label();
	}
}

void StartMenu::popup(void) {
	menu_opened = true;

	const MenuItem *m;

	pressed_menu_button = this;
	redraw();

#if (FL_MAJOR_VERSION >= 1) && (FL_MINOR_VERSION >= 3)
	Fl_Widget *mb = this;
	Fl::watch_widget_pointer(mb);
#endif

	if(!box() || type())
		m = menu()->popup(Fl::event_x(), Fl::event_y(), label(), mvalue(), this);
	else
		m = menu()->pulldown(x(), y(), w(), h(), 0, this);

	picked(m);
	pressed_menu_button = 0;

#if (FL_MAJOR_VERSION >= 1) && (FL_MINOR_VERSION >= 3)
	Fl::release_widget_pointer(mb);
#endif

	menu_opened = false;
#ifdef EDE_PANEL_MENU_AUTOUPDATE
	/* if we have menu that wants to be updated, swap them as soon as menu window was closed */
	if(mcontent_pending) {
		XdgMenuContent *tmp = mcontent;

		mcontent = mcontent_pending;
		setup_menu(mcontent);

		mcontent_pending = tmp;
		xdg_menu_delete(mcontent_pending);
		mcontent_pending = NULL;
	}
#endif
}

int StartMenu::handle(int e) {
	if(!menu() || !menu()->text)
		return 0;

	switch(e) {
		case FL_ENTER:
		case FL_LEAVE:
			return (box() && !type()) ? 1 : 0;
		case FL_PUSH:
			if(!box()) {
				if(Fl::event_button() != 3) 
					return 0;
			} else if(type()) {
				if(!(type() & (1 << (Fl::event_button() - 1))))
					return 0;
			}

			if(Fl::visible_focus()) 
				Fl::focus(this);

			popup();
			return 1;

		case FL_KEYBOARD:
			if(!box()) return 0;

			/* 
			 * Win key will show the menu.
			 *
			 * In FLTK, Meta keys are equivalent to Alt keys. On other hand, eFLTK has them different, much
			 * the same as on M$ Windows. Here, I'm explicitly using hex codes, since Fl_Meta_L and Fl_Meta_R
			 * are unusable.
			 */
			if(Fl::event_key() == 0xffeb || Fl::event_key() == 0xffec) {
				popup();
				return 1;
			}

			return 0;

		case FL_SHORTCUT:
			if(Fl_Widget::test_shortcut()) { 
				popup();
				return 1;
			}

			return test_shortcut() != 0;

		case FL_FOCUS:
		case FL_UNFOCUS:
			if(box() && Fl::visible_focus()) {
				redraw();
				return 1;
			}

		default:
			return 0;
	}

	/* not reached */
	return 0;
}

/* to prevent throttling of dirwatch events */
bool StartMenu::can_reload(void) {
#ifdef EDE_PANEL_MENU_AUTOUPDATE
	time_t c, diff;

	c = time(NULL);
	diff = (time_t)difftime(c, last_reload);
	last_reload = c;

	if(diff >= MENU_UPDATE_DIFF) return true;
#endif

	return false;
}

void StartMenu::reload_menu(void) {
#ifdef EDE_PANEL_MENU_AUTOUPDATE
	if(menu_opened) {
		mcontent_pending = xdg_menu_load();
	} else {
		xdg_menu_delete(mcontent);

		mcontent = xdg_menu_load();
		setup_menu(mcontent);
	}
#endif
}

EDE_PANEL_APPLET_EXPORT (
 StartMenu, 
 EDE_PANEL_APPLET_OPTION_ALIGN_LEFT,
 "Main menu",
 "0.2",
 "empty",
 "Sanel Zukan"
)
