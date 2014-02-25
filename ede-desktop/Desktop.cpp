/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
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

#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include <edelib/Netwm.h>
#include <edelib/WindowUtils.h>
#include <edelib/ForeignCallback.h>
#include <edelib/MenuItem.h>
#include <edelib/Nls.h>
#include <edelib/StrUtil.h>
#include <edelib/Directory.h>
#include <edelib/Util.h>
#include <edelib/Run.h>
#include <edelib/MessageBox.h>
#include <edelib/IconLoader.h>
#include <edelib/FontCache.h>
#include <edelib/FileTest.h>
#include <edelib/File.h>

#include "Desktop.h"
#include "DesktopIcon.h"
#include "Wallpaper.h"
#include "Utils.h"
#include "IconDialog.h"

EDELIB_NS_USING(MenuButton)
EDELIB_NS_USING(MenuItem)
EDELIB_NS_USING(String)
EDELIB_NS_USING(DesktopFile)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(window_xid_create)
EDELIB_NS_USING(netwm_window_set_type)
EDELIB_NS_USING(netwm_workarea_get_size)
EDELIB_NS_USING(netwm_callback_add)
EDELIB_NS_USING(foreign_callback_add)
EDELIB_NS_USING(str_ends)
EDELIB_NS_USING(dir_home)
EDELIB_NS_USING(dir_empty)
EDELIB_NS_USING(dir_remove)
EDELIB_NS_USING(build_filename)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(input)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(dir_create)
EDELIB_NS_USING(file_test)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(font_cache_find)
EDELIB_NS_USING(FILE_TEST_IS_DIR)
EDELIB_NS_USING(FILE_TEST_IS_REGULAR)
EDELIB_NS_USING(FILE_TEST_IS_SYMLINK)
EDELIB_NS_USING(NETWM_WINDOW_TYPE_DESKTOP)
EDELIB_NS_USING(NETWM_CHANGED_CURRENT_WORKAREA)
EDELIB_NS_USING(DESK_FILE_TYPE_UNKNOWN)
EDELIB_NS_USING(ICON_SIZE_TINY)

#define DOT_OR_DOTDOT(base) (base[0] == '.' && (base[1] == '\0' || (base[1] == '.' && base[2] == '\0')))
#define NOT_SELECTABLE(widget) ((widget == this) || (widget == wallpaper) || (widget == dmenu))

#define SELECTION_SINGLE (Fl::event_button() == 1)
#define SELECTION_MULTI  (Fl::event_button() == 1 && (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)))

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

#define ICONS_POS_FILE "ede-desktop-icons"

static void background_conf_cb(Fl_Widget*, void*);
static void icons_conf_cb(Fl_Widget*, void*);
static void folder_create_cb(Fl_Widget*, void*);
static void launcher_create_cb(Fl_Widget*, void*);
static void arrange_icons_cb(Fl_Widget*, void*);

struct SelectionOverlay {
	int x, y, w, h;
	bool show;
	SelectionOverlay() : x(0), y(0), w(0), h(0), show(false) { }
};

static MenuItem desktop_menu[] = {
	{_("Create &launcher..."), 0, launcher_create_cb},
	{_("Create &folder..."), 0, folder_create_cb, 0, FL_MENU_DIVIDER},
	{_("A&rrange icons"), 0, arrange_icons_cb, 0, FL_MENU_DIVIDER},
	{_("&Icons settings..."), 0, icons_conf_cb, 0},
	{_("&Background..."), 0, background_conf_cb, 0},
	{0}
};

inline bool intersects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (MAX(x1, x2) <= MIN(w1, w2) && 
			MAX(y1, y2) <= MIN(h1, h2));
}

static void make_me_desktop(Fl_Window *win) {
	netwm_window_set_type(fl_xid(win), NETWM_WINDOW_TYPE_DESKTOP);
}

static void settings_changed_cb(Fl_Window *win, void *data) {
	Desktop *d = (Desktop*)win;
	d->read_config();
}

static void desktop_message_handler(int action, Window xid, void *data) {
	Desktop *d = (Desktop*)data;

	switch(action) {
		case NETWM_CHANGED_CURRENT_WORKAREA:
			d->update_workarea();
			break;
	}
}

static void background_conf_cb(Fl_Widget*, void*) {
	run_async("ede-launch ede-desktop-conf");
}

static void icons_conf_cb(Fl_Widget*, void*) {
	run_async("ede-launch ede-desktop-conf --icons");
}

static void launcher_create_cb(Fl_Widget*, void* d) {
	Desktop *self = (Desktop*)d;
	icon_dialog_icon_create(self);
}

static void arrange_icons_cb(Fl_Widget*, void *d) {
	E_RETURN_IF_FAIL(d != NULL);

	Desktop *self = (Desktop*)d;
	self->arrange_icons();
	self->redraw();
}

static void folder_create_cb(Fl_Widget*, void *d) {
	E_RETURN_IF_FAIL(d != NULL);

	const char *n = input(_("Create a new folder with the name"));
	if(!n) return;
	
	Desktop *self = (Desktop*)d;
	self->create_folder(n);
}

Desktop::Desktop() : EDE_DESKTOP_WINDOW(0, 0, 100, 100, EDE_DESKTOP_APP) {
	end();
	/* use nice darker blue color as default for background */
	color(fl_rgb_color(73, 64, 102));

	conf = NULL;
	selbox = new SelectionOverlay;
	icon_opts = NULL;

	wallpaper = NULL;
	dmenu = NULL;
	
	selection_x = selection_y = 0;
	last_px = last_py = -1;

	moving = false;

	/*
	 * first update workarea _then_ read configuration, as it will have correct size for
	 * wallpaper which will prevent wallpaper load and scalle image twice
	 */
	update_workarea();
	read_config();
	read_desktop_folder(desktop_path());

	foreign_callback_add(this, EDE_DESKTOP_APP, settings_changed_cb);

	dmenu = new MenuButton(0, 0, 500, 0);
	dmenu->menu(desktop_menu);
	desktop_menu[1].image((Fl_Image*)IconLoader::get("folder", ICON_SIZE_TINY));
	desktop_menu[0].user_data(this);
	desktop_menu[1].user_data(this);
	desktop_menu[2].user_data(this);
	add(dmenu);
}

Desktop::~Desktop() {
	E_DEBUG("Desktop::~Desktop()\n");

	delete conf;
	delete icon_opts;
	delete selbox;
}

const char *Desktop::desktop_path(void) {
	if(dpath.empty())
		dpath = build_filename(dir_home().c_str(), "Desktop");
	return dpath.c_str();
}

void Desktop::show(void) {
	if(shown()) {
		EDE_DESKTOP_WINDOW::show();
		return;
	}

	window_xid_create(this, make_me_desktop);
	netwm_callback_add(desktop_message_handler, this);
}

void Desktop::update_workarea(void) {
	int X, Y, W, H;

	if(!netwm_workarea_get_size(X, Y, W, H))
		Fl::screen_xywh(X, Y, W, H);

	E_DEBUG(E_STRLOC ": resizing to %i %i %i %i\n", X, Y, W, H);
	resize(X, Y, W, H);
	
	/* also resize wallpaper if given */
	if(wallpaper && wallpaper->visible())
		wallpaper->resize(0, 0, w(), h());
}

void Desktop::read_config(void) {
	E_DEBUG(E_STRLOC ": Reading desktop config...\n");

	if(!conf) conf = new DesktopConfig();
	conf->load(EDE_DESKTOP_APP);

	char buf[PATH_MAX];
	bool wp_use = true;
	int  bcolor;

	if(conf->get("Desktop", "color", bcolor, color()))
	   color(bcolor);

	conf->get("Desktop", "wallpaper_use", wp_use, wp_use);

	/* get wallpaper */
	if(wp_use) {
		if(conf->get("Desktop", "wallpaper", buf, sizeof(buf))) {
			if(!wallpaper)
				wallpaper = new Wallpaper(0, 0, w(), h());

			int s;
			bool rootpmap_use;
			conf->get("Desktop", "wallpaper_mode", s, WALLPAPER_CENTER);
			conf->get("Desktop", "wallpaper_rootpmap", rootpmap_use, true);
			wallpaper->load(buf, s, rootpmap_use);

			/*
			 * wallpaper is another widget, but we add it to the bottom of the list
			 * to preserve stacking order
			 */
			if(find(*wallpaper) == children())
				insert(*wallpaper, 0);

			/* show it in case it got hidden before */
			wallpaper->show();
		}
	} else {
		if(wallpaper) wallpaper->hide();
	}

	if(!icon_opts) icon_opts = new IconOptions;

#define ICON_CONF_GET(var) conf->get("Icons", #var, icon_opts->var, icon_opts->var)
	ICON_CONF_GET(label_background);
	ICON_CONF_GET(label_foreground);
	ICON_CONF_GET(label_maxwidth);
	ICON_CONF_GET(label_transparent);
	ICON_CONF_GET(label_visible);
	ICON_CONF_GET(one_click_exec);

	if(ICON_CONF_GET(label_font) && ICON_CONF_GET(label_fontsize))
		E_WARNING(E_STRLOC ": 'label_font' && 'label_fontsize' are deprecated. Use 'label_fontname' with full font name and size instead (e.g 'sans 12')\n");

	/* if found new 'label_fontname' variable, overwrite 'label_font' && 'label_fontsize' */
	if(conf->get("Icons", "label_fontname", buf, sizeof(buf))) {
		if(!font_cache_find(buf, (Fl_Font&)icon_opts->label_font, icon_opts->label_fontsize))
			E_WARNING(E_STRLOC ": Unable to find '%s' font. Using default values...\n", buf);
	}

	icon_opts->sanitize_font();

	/* doing this will redraw _all_ children, and they will in turn read modified 'icon_opts' */
	if(visible()) redraw();
}

void Desktop::read_desktop_folder(const char *dpath) {
	E_RETURN_IF_FAIL(dpath != NULL);
	String path;

	DIR *dir = opendir(dpath);
	E_RETURN_IF_FAIL(dir != NULL);
	
	DesktopConfig pos;
	pos.load(ICONS_POS_FILE);

	dirent *d;
	while((d = readdir(dir)) != NULL) {
		if(DOT_OR_DOTDOT(d->d_name))
			continue;

		if(d->d_type > 0) {
			if(d->d_type != DT_REG && d->d_type != DT_LNK && d->d_type != DT_DIR)
				continue;

			path = dpath;
			path += E_DIR_SEPARATOR;
			path += d->d_name;
		} else {
			/* 
			 * If we got here, it means d_type isn't set and we must do it via file_test() which could be much slower.
			 * By POSIX standard, only d_name must be set, but many modern *nixes set all dirent members correctly. Except Slackware ;)
			 */
			path = dpath;
			path += E_DIR_SEPARATOR;
			path += d->d_name;

			if(!(file_test(path.c_str(), FILE_TEST_IS_REGULAR) ||
			     file_test(path.c_str(), FILE_TEST_IS_DIR)     ||
			     file_test(path.c_str(), FILE_TEST_IS_SYMLINK)))
				continue;
		}

		DesktopIcon *o = read_desktop_file(path.c_str(), (const char*)d->d_name, &pos);
		if(o) add(o);
	}

	closedir(dir);
}

DesktopIcon *Desktop::read_desktop_file(const char *path, const char *base, DesktopConfig *pos) {
	DesktopIcon *ret = NULL;
	
	if(file_test(path, FILE_TEST_IS_DIR)) {
		ret = new DesktopIcon(_("No Name"));
		ret->set_icon_type(DESKTOP_ICON_TYPE_FOLDER);
		/* hardcoded */
		ret->set_image("folder");

		/* copy label explicitly, as DesktopIcon() will only store a pointer */
		ret->copy_label(base);
	} else {
		/*
		 * try to load it as plain .desktop file by looking only at extension
		 * TODO: MimeType can be used here too
		 */
		if(!str_ends(path, EDE_DESKTOP_DESKTOP_EXT))
			return ret;

		DesktopFile df;
		char        buf[PATH_MAX];

		E_RETURN_VAL_IF_FAIL(df.load(path), ret);
		E_RETURN_VAL_IF_FAIL(df.type() != DESK_FILE_TYPE_UNKNOWN, ret);

		ret = new DesktopIcon(_("No Name"));
		ret->set_icon_type(DESKTOP_ICON_TYPE_NORMAL);

		if(df.name(buf, sizeof(buf))) ret->copy_label(buf);
		ret->set_image(df.icon(buf, sizeof(buf)) ? buf : NULL);
		if(df.comment(buf, sizeof(buf))) ret->set_tooltip(buf);
	}

	/* try to put random icon position in the middle of the desktop, so it is easier to notice */
	int X = (rand() % (w() / 4)) + (w() / 4);
	int Y = (rand() % (h() / 4)) + (h() / 4);

	/* lookup icons locations if possible */
	if(base && pos && *pos) {
		pos->get(base, "X", X, X);
		pos->get(base, "Y", Y, Y);
	}

	E_DEBUG("Setting icon '%s' (%i,%i)\n", base, X, Y);
	ret->position(X, Y);

	/* use loaded icon options */
	ret->set_options(icon_opts);
	ret->set_path(path);
	return ret;
}


void Desktop::arrange_icons(void) {
	int lw = icon_opts ? icon_opts->label_maxwidth : 75;
	int X = (lw / 5) + 10;
	int Y = 10;
	int H = h();
	DesktopIcon *o;
	
	for(int i = 0; i < children(); i++) {
		if(NOT_SELECTABLE(child(i))) continue;
		
		o = (DesktopIcon*)child(i);
		o->position(X, Y);
		Y += o->h() + (o->h() / 2);
		
		if(Y + (o->h() / 2) > H) {
			Y = 10;
			X += lw + (o->w() / 2);
		}
	}
}

bool Desktop::remove_icon(DesktopIcon *di, bool real_delete) {
	bool ret = true;

	if(real_delete) {
		if(di->get_icon_type() == DESKTOP_ICON_TYPE_FOLDER) {
			if(!dir_empty(di->get_path())) {
				alert(_("This folder is not empty. Recursive removal of not empty folders is not yet supported"));
				return false;
			}
			ret = dir_remove(di->get_path());
		} else {
			ret = file_remove(di->get_path());
		}
	}

	remove(di);
	redraw();
	return ret;
}

bool Desktop::rename_icon(DesktopIcon *di, const char *name) {
	di->copy_label(name);
	di->update_label_font_and_size();
	di->fast_redraw();
	
	/* open file and try to change the name */
	DesktopFile df;
	E_RETURN_VAL_IF_FAIL(df.load(di->get_path()) == true, false);
	
	df.set_name(name);
	return df.save(di->get_path());
}

void Desktop::edit_icon(DesktopIcon *di) { 
	icon_dialog_icon_edit(this, di);
}

bool Desktop::save_icons_positions(void) {
	DesktopConfig pos;
	DesktopIcon   *o;
	char          *base;

	for(int i = 0; i < children(); i++) {
		if(NOT_SELECTABLE(child(i))) continue;

		o = (DesktopIcon*)child(i);
		base = get_basename(o->get_path());
		pos.set(base, "X", o->x());
		pos.set(base, "Y", o->y());
	}
	
	return pos.save(ICONS_POS_FILE);
}

bool Desktop::create_folder(const char *name) {
	String path = desktop_path();
	path += E_DIR_SEPARATOR;
	path += name;

	if(!dir_create(path.c_str())) {
		alert(_("Unable to create directory '%s'! Please check if directory already exists or you have enough permissions to create it"), path.c_str());
		return false;
	}
	
	DesktopIcon *ic = read_desktop_file(path.c_str(), name);
	if(ic) {
		/* use mouse position so folder icon gets created where was clicked */
		ic->position(last_px, last_py);
		add(ic);
		redraw();
	}
	
	return true;
}

void Desktop::unfocus_all(void) {
	DesktopIcon *o;
	for(int i = 0; i < children(); i++) {
		if(NOT_SELECTABLE(child(i))) continue;

		o = (DesktopIcon*)child(i);
		o->do_unfocus();
		o->fast_redraw();
	}
}

void Desktop::select(DesktopIcon *ic, bool do_redraw) { 
	E_ASSERT(ic != NULL);

	if(in_selection(ic)) return;
	selectionbuf.push_back(ic);

	if(!ic->is_focused()) {
		ic->do_focus();
		if(do_redraw) ic->fast_redraw();
	}
}

void Desktop::select_only(DesktopIcon *ic) { 
	E_ASSERT(ic != NULL);

	unfocus_all();
	selectionbuf.clear();
	selectionbuf.push_back(ic);

	ic->do_focus();
	ic->fast_redraw();
}

bool Desktop::in_selection(const DesktopIcon* ic) { 
	E_ASSERT(ic != NULL);

	if(selectionbuf.empty())
		return false;

	for(DesktopIconListIt it = selectionbuf.begin(), ite = selectionbuf.end(); it != ite; ++it) {
		if((*it) == ic)
			return true;
	}

	return false;
}

void Desktop::move_selection(int x, int y, bool apply) { 
	if(selectionbuf.empty()) return; 

	int prev_x, prev_y, tmp_x, tmp_y;
	DesktopIcon *ic;

	for(DesktopIconListIt it = selectionbuf.begin(), ite = selectionbuf.end(); it != ite; ++it) {
		ic = (*it);

		prev_x = ic->drag_icon_x();
		prev_y = ic->drag_icon_y();

		tmp_x = x - selection_x;
		tmp_y = y - selection_y;

		ic->drag(prev_x + tmp_x, prev_y + tmp_y, apply);
	}

	selection_x = x;
	selection_y = y;

	/*
	 * move the last moved icon on the top of the stack, so it be drawn the top most; also
	 * when called arrange_icons(), last moved icon will be placed last in the list
	 */
	ic = selectionbuf.back();
	insert(*ic, children());
	
	if(apply) {
		redraw();
		save_icons_positions();
	}
}

/*
 * Tries to figure out icon below mouse. It is alternative to Fl::belowmouse() since with this we hunt 
 * only icons, not other childs (wallpaper, menu), which can be returned by Fl::belowmouse() and bad 
 * things be hapened.
 */
DesktopIcon* Desktop::below_mouse(int px, int py) {
	DesktopIcon *o;

	for(int i = 0; i < children(); i++) {
		if(NOT_SELECTABLE(child(i))) continue;
		
		o = (DesktopIcon*)child(i);
		if(o->x() < px && o->y() < py && px < (o->x() + o->h()) && py < (o->y() + o->h()))
			return o;
	}

	return NULL;
}

void Desktop::select_in_area(void) {
	if(!selbox->show) return;

	int ax = selbox->x;
	int ay = selbox->y;
	int aw = selbox->w;
	int ah = selbox->h;

	if(aw < 0) {
		ax += aw;
		aw = -aw;
	} else if(!aw)
		aw = 1;

	if(ah < 0) {
		ay += ah;
		ah = -ah;
	} else if(!ah)
		ah = 1;

	/*
	 * XXX: This function can fail since icon coordinates are absolute (event_x_root)
	 * but selbox use relative (event_root). It will work as expected if desktop is at x=0 y=0.
	 * This should be checked further.
	 */
	DesktopIcon* o;
	for(int i = 0; i < children(); i++) {
		if(NOT_SELECTABLE(child(i))) continue;
		o = (DesktopIcon*)child(i);
		
		if(intersects(ax, ay, ax+aw, ay+ah, o->x(), o->y(), o->w()+o->x(), o->h()+o->y())) {
			if(!o->is_focused()) {
				o->do_focus();
				/* updated from Desktop::draw() */
				o->damage(EDE_DESKTOP_DAMAGE_CHILD_LABEL);
			}
		} else {
			if(o->is_focused()) {
				o->do_unfocus();
				o->fast_redraw();
			}
		}
	} 
}

void Desktop::draw(void) {
	if(!damage()) return;
	
	if(damage() & (FL_DAMAGE_ALL | FL_DAMAGE_EXPOSE)) {
		/*
		 * If any overlay was previously visible during full redraw, it will not be cleared because of fast flip.
		 * This will assure that does not happened.
		 */
		fl_overlay_clear();
		EDE_DESKTOP_WINDOW::draw();
		//E_DEBUG(E_STRLOC ": REDRAW ALL\n");
	}

	if(damage() & EDE_DESKTOP_DAMAGE_OVERLAY) {
		if(selbox->show)
			fl_overlay_rect(selbox->x, selbox->y, selbox->w, selbox->h);
		else
			fl_overlay_clear();

		/*
		 * now scan all icons and see if they needs redraw, and if do
		 * just update their label since it is indicator of selection
		 */
		for(int i = 0; i < children(); i++) {
			if(child(i)->damage() == EDE_DESKTOP_DAMAGE_CHILD_LABEL) {
				update_child(*child(i));
				child(i)->clear_damage();
				//E_DEBUG(E_STRLOC ": ICON REDRAW \n");
			}
		}
	}

	clear_damage();
}

int Desktop::handle(int event) {
	switch(event) {
		case FL_FOCUS:
		case FL_UNFOCUS:
		case FL_SHORTCUT:
			return 1;

		case FL_PUSH: {
			/*
			 * First check where we clicked. If we do it on desktop unfocus any possible focused childs, and handle
			 * specific clicks. Otherwise, do rest for childs.
			 */
			Fl_Widget* clicked = Fl::belowmouse();
			
			if(NOT_SELECTABLE(clicked)) {
				//E_DEBUG(E_STRLOC ": DESKTOP CLICK !!!\n");
				if(!selectionbuf.empty()) {
					/*
					 * Only focused are in selectionbuf, so this is fine to do; also will prevent 
					 * full redraw when is clicked on desktop
					 */
					unfocus_all();
					selectionbuf.clear();
				}

				/* track position so moving can be deduced */
				if(Fl::event_button() == 1) {
					selbox->x = Fl::event_x();
					selbox->y = Fl::event_y();
				} else if(Fl::event_button() == 3) {
					last_px = Fl::event_x();
					last_py = Fl::event_y();

					damage(EDE_DESKTOP_DAMAGE_OVERLAY);
					const edelib::MenuItem* item = dmenu->menu()->popup(Fl::event_x(), Fl::event_y());
					dmenu->picked(item);
				}

				return 1;
			}

			/* from here, all events are managed for icons */
			DesktopIcon* tmp_icon = (DesktopIcon*)clicked;

			/* do no use assertion on this, since fltk::belowmouse() can miss our icon */
			if(!tmp_icon) return 1;

			if(SELECTION_MULTI) {
				Fl::event_is_click(0);
				select(tmp_icon);
				return 1;
			} else if(SELECTION_SINGLE) {
				if(!in_selection(tmp_icon)) {
					select_only(tmp_icon);
				}
			} else if(Fl::event_button() == 3) {
				select_only(tmp_icon);
			}

			/* 
			 * Let child handle the rest.
			 * Also prevent click on other mouse buttons during move.
			 */
			if(!moving)
				tmp_icon->handle(FL_PUSH);

			//E_DEBUG(E_STRLOC ": FL_PUSH from desktop\n");
			selection_x = Fl::event_x_root();
			selection_y = Fl::event_y_root();
	
			return 1;
		 }

		case FL_DRAG:
			moving = true;
			if(!selectionbuf.empty()) {
				//E_DEBUG(E_STRLOC ": DRAG icon from desktop\n");
				move_selection(Fl::event_x_root(), Fl::event_y_root(), false);
			} else {
				//E_DEBUG(E_STRLOC ": DRAG from desktop\n");
				/*
				 * Moving is started with pushed button.
				 * From this point selection box is created and is rolled until release
				 */
				if(selbox->x != 0 || selbox->y != 0) {
					selbox->w = Fl::event_x() - selbox->x;
					selbox->h = Fl::event_y() - selbox->y;

					selbox->show = true;

					/* see if there some icons inside selection area */
					select_in_area();

					/* redraw selection box */
					damage(EDE_DESKTOP_DAMAGE_OVERLAY);
				}
			}

			return 1;

		case FL_RELEASE:
			//E_DEBUG(E_STRLOC ": RELEASE from desktop\n");
			//E_DEBUG(E_STRLOC ": clicks: %i\n", Fl::event_is_click());

			if(selbox->show) {
				selbox->x = selbox->y = selbox->w = selbox->h = 0;
				selbox->show = false;
				damage(EDE_DESKTOP_DAMAGE_OVERLAY);

				/*
				 * Now pickup those who are in is_focused() state.
				 *
				 * Possible flickers due overlay will be later removed when is called move_selection(), which 
				 * will in turn redraw icons again after position them.
				 */
				if(!selectionbuf.empty())
					selectionbuf.clear();

				DesktopIcon *o;
				for(int i = 0; i < children(); i++) {
					if(NOT_SELECTABLE(child(i))) continue;

					o = (DesktopIcon*)child(i);
					if(o->is_focused())
						select(o, false);
				}

				return 1;
			}

			if(!selectionbuf.empty() && moving)
				move_selection(Fl::event_x_root(), Fl::event_y_root(), true);

			/* 
			 * Do not send FL_RELEASE during move
			 * TODO: should be alowed FL_RELEASE to multiple icons? (aka. run command for all selected icons)?
			 */
			if(selectionbuf.size() == 1 && !moving)
				selectionbuf.front()->handle(FL_RELEASE);

			moving = false;
			return 1;

		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_LEAVE:
			return 1;

		case FL_DND_RELEASE: {
			DesktopIcon* di = below_mouse(Fl::event_x(), Fl::event_y());
			if(di) return di->handle(event);
			return 1;
		}

		case FL_PASTE: {
			DesktopIcon* di = below_mouse(Fl::event_x(), Fl::event_y());
			if(di) return di->handle(event);
			return 1;
		}

		case FL_ENTER:
		case FL_LEAVE:
		case FL_MOVE:
			return EDE_DESKTOP_WINDOW::handle(event);

		default:
			break;
	}

	return 0;
}
