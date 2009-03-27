/*
 * $Id$
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h> // sleep
#include <signal.h>
#include <stdlib.h> // rand, srand
#include <time.h>   // time

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Menu_Button.H>

#include <edelib/Debug.h>
#include <edelib/File.h>
#include <edelib/DesktopFile.h>
#include <edelib/Directory.h>
#include <edelib/DirWatch.h>
#include <edelib/MimeType.h>
#include <edelib/StrUtil.h>
#include <edelib/IconLoader.h>
#include <edelib/Run.h>
#include <edelib/Util.h>
#include <edelib/Nls.h>
#include <edelib/MessageBox.h>

#include "ede-desktop.h"
#include "DesktopIcon.h"
#include "Utils.h"
#include "Wallpaper.h"

#define EDE_DESKTOP_UID    0x10
#define CONFIG_NAME        "ede-desktop"
#define ICONS_CONFIG_NAME  "ede-desktop-icons"

#define EDE_DESKTOP_INTERFACE "org.equinoxproject.Desktop"
#define EDE_DESKTOP_OBJECT    "/org/equinoxproject/Desktop"

#define SELECTION_SINGLE (Fl::event_button() == 1)
#define SELECTION_MULTI  (Fl::event_button() == 1 && (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)))

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

/*
 * Which widgets Fl::belowmouse() should skip. This should be updated
 * when new non-icon child is added to Desktop window.
 */
#define NOT_SELECTABLE(widget) ((widget == this) || (widget == wallpaper) || (widget == dmenu))

static void background_conf_cb(Fl_Widget*, void*);
static void icons_conf_cb(Fl_Widget*, void*);

Fl_Menu_Item desktop_menu[] = {
	{_("    Create &launcher...    "), 0, 0},
	{_("    Create &folder...    "), 0, 0, 0, FL_MENU_DIVIDER},
	{_("    &Arrange    "), 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER},
		{_("    &Vertical line up    "), 0, 0},
		{_("    &Horizontal line up   "), 0, 0},
		{0},
	{_("    &Copy    "), 0, 0},
	{_("    &Paste    "), 0, 0, 0, FL_MENU_DIVIDER},
	{_("    &Icons settings...    "), 0, icons_conf_cb, 0},
	{_("    &Background...    "), 0, background_conf_cb, 0},
	{0}
};

Desktop* Desktop::pinstance = NULL;
static bool running = false;

inline unsigned int random_pos(int max) { 
	return (rand() % max); 
}

inline bool intersects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (MAX(x1, x2) <= MIN(w1, w2) && 
			MAX(y1, y2) <= MIN(h1, h2));
}

static void exit_signal(int signum) {
    E_DEBUG(E_STRLOC ": Exiting (got signal %d)\n", signum);
    running = false;
}

static void restart_signal(int signum) {
	E_DEBUG(E_STRLOC ": Restarting (got signal %d)\n", signum);
}

static void dir_watch_cb(const char* dir, const char* changed, int flags, void* data) {
	Desktop::instance()->dir_watch(dir, changed, flags);
}

static void settings_changed_cb(void* data) {
	Desktop::instance()->read_config();
	Desktop::instance()->redraw();
}

static void background_conf_cb(Fl_Widget*, void*) {
	Desktop::instance()->execute("ede-desktop-conf");
}

static void icons_conf_cb(Fl_Widget*, void*) {
	Desktop::instance()->execute("ede-desktop-conf --icons");
}

static int desktop_xmessage_handler(int event) { 
	if(fl_xevent->type == PropertyNotify) {
		if(fl_xevent->xproperty.atom == _XA_NET_CURRENT_DESKTOP) {
			Desktop::instance()->notify_desktop_changed();
			return 1;
		}

		if(fl_xevent->xproperty.atom == _XA_NET_WORKAREA) {
			Desktop::instance()->update_workarea();
			return 1;
		}
	}

	return 0; 
}

Desktop::Desktop() : DESKTOP_WINDOW(0, 0, 100, 100, "") {
	selection_x = selection_y = 0;
	moving = false;
	do_dirwatch = true;

#ifdef USE_EDELIB_WINDOW
	window_id(EDE_DESKTOP_UID);
	foreign_callback(settings_changed_cb);
	/* DESKTOP_WINDOW::single_bufer(true); */
#endif

	selbox = new SelectionOverlay;
	selbox->x = selbox->y = selbox->w = selbox->h = 0;
	selbox->show = false;

	gisett = new GlobalIconSettings;
	/* gnome light blue */
	gisett->label_background = 138;
	gisett->label_foreground = FL_WHITE;
	gisett->label_fontsize = 12;
	gisett->label_maxwidth = 75;
	gisett->label_transparent = false;
	gisett->label_draw = true;
	gisett->one_click_exec = false;
	gisett->auto_arrange = true;

	/* default background color */
	color(FL_GRAY);
}

Desktop::~Desktop() { 
	E_DEBUG("Desktop::~Desktop()\n");

	save_icons_positions();

	delete gisett;
	delete selbox;
	delete dbus;

	edelib::DirWatch::shutdown();
}

void Desktop::init_internals(void) {
	edelib::String p;

	init_atoms();
	update_workarea();

	/*
	 * NOTE: order how childs are added is important. First non iconable ones should be 
	 * added (wallpaper, menu, ...) then icons, so they can be drawn at top of them.
	 */
	begin();
		dmenu = new Fl_Menu_Button(0, 0, 0, 0);
		dmenu->menu(desktop_menu);

		wallpaper = new Wallpaper(0, 0, w(), h());
	end();

	dbus = new edelib::EdbusConnection();
	if(!dbus->connect(edelib::EDBUS_SESSION)) {
		E_WARNING(E_STRLOC ": Unable to connect to session bus. Disabling dbus interface...\n");
		delete dbus;
		dbus = NULL;
	}

	/* read main config */
	read_config();

	/* now try to load icons from "Desktop" directory */
	p = edelib::dir_home();
	edelib::String desktop_path = edelib::build_filename(p.c_str(), "Desktop");

	/* setup watcher used for Desktop and Trash directories */
	edelib::DirWatch::init();

	if(edelib::dir_exists(desktop_path.c_str())) {
		load_icons(desktop_path.c_str());

		if(!edelib::DirWatch::add(desktop_path.c_str(), 
					edelib::DW_CREATE | edelib::DW_MODIFY | edelib::DW_RENAME | edelib::DW_DELETE)) 
		{
			E_WARNING(E_STRLOC ": Unable to watch %s\n", desktop_path.c_str());
		}
	}

	/*
	 * For watching Trash, we will check Trash/files only with create/delete flags and
	 * in callback will be checked if directory is empty (trash empty) or not (trash full).
	 */
	p = edelib::user_data_dir();
	trash_path = edelib::build_filename(p.c_str(), "Trash/files");

	if(edelib::dir_exists(trash_path.c_str())) {
		if(!edelib::DirWatch::add(trash_path.c_str(), edelib::DW_CREATE | edelib::DW_DELETE))
			E_WARNING(E_STRLOC ": Unable to watch %s\n", trash_path.c_str());
	}

	/* draw appropriate trash icon */
	update_trash_icons();

	edelib::DirWatch::callback(dir_watch_cb);
	running = true;
}

void Desktop::init(void) {
	if(Desktop::pinstance != NULL)
		return;

	Desktop::pinstance = new Desktop();
	Desktop::pinstance->init_internals();
}

void Desktop::shutdown(void) {
	if(Desktop::pinstance == NULL)
		return;

	delete Desktop::pinstance;
	Desktop::pinstance = NULL;
}

Desktop* Desktop::instance(void) {
	E_ASSERT(Desktop::pinstance != NULL && "Desktop::init() should be run first");
	return Desktop::pinstance;
}

/*
 * This function must be overriden so window can inform wm to see it as desktop. It will send data when window
 * is created, but before is shown.
 */
void Desktop::show(void) {
	if(!shown()) {
		Fl_X::make_xid(this);
		net_make_me_desktop(this);
	}
}

/* If someone intentionaly hide desktop then quit from it */
void Desktop::hide(void) {
	running = false;
}

void Desktop::update_workarea(void) {
	int X, Y, W, H;
	if(!net_get_workarea(X, Y, W, H)) {
		E_DEBUG(E_STRLOC ": wm does not support _NET_WM_WORKAREA; using screen sizes...\n");
		X = Y = 0;

		W = DisplayWidth(fl_display, fl_screen);
		H = DisplayHeight(fl_display, fl_screen);
	}

	resize(X, Y, W, H);
}

void Desktop::read_config(void) {
	edelib::Resource conf;

	if(!conf.load(CONFIG_NAME)) {
		E_WARNING(E_STRLOC ": Can't load %s, using default values\n", CONFIG_NAME);
		return;
	}

	bool wuse;
	char wpath[256];
	int  bcolor, wmode;

	conf.get("Desktop", "color", bcolor, FL_GRAY);
	conf.get("Desktop", "wallpaper_use", wuse, false);
	conf.get("Desktop", "wallpaper", wpath, sizeof(wpath));
	conf.get("Desktop", "wallpaper_mode", wmode, WALLPAPER_CENTER);

	/* '138' is gnome light blue */
	conf.get("Icons", "label_background", gisett->label_background, 138);
	conf.get("Icons", "label_foreground", gisett->label_foreground, FL_WHITE);
	conf.get("Icons", "label_fontsize",   gisett->label_fontsize, 12);
	conf.get("Icons", "label_maxwidth",   gisett->label_maxwidth, 75);
	conf.get("Icons", "label_transparent",gisett->label_transparent, false);
	conf.get("Icons", "label_visible",    gisett->label_draw, true);
	conf.get("Icons", "one_click_exec",   gisett->one_click_exec, false);
	conf.get("Icons", "auto_arrange",     gisett->auto_arrange, true);

	/* prevent wild values */
	if(wmode != WALLPAPER_CENTER && wmode != WALLPAPER_TILE && wmode != WALLPAPER_STRETCH)
		wmode = WALLPAPER_CENTER;

	if(wuse) {
		if(!wallpaper->load(wpath, (WallpaperState)wmode))
			E_WARNING(E_STRLOC ": unable to load wallpaper '%s'\n", wpath);
		else
			wallpaper->show();
	}

	if(!wuse && wallpaper->visible())
		wallpaper->hide();

	/* change background color too */
	color(bcolor);
}

void Desktop::load_icons(const char* path) {
	E_ASSERT(path != NULL);
	edelib::Resource conf, *conf_ptr = NULL;

	if(!conf.load(ICONS_CONFIG_NAME))
		E_WARNING(E_STRLOC ": Can't load icons positions from %s, randomizing them...\n", ICONS_CONFIG_NAME);
	else 
		conf_ptr = &conf;

	StringList lst;

	/* list with full path; icon basename is extracted in add_icon_by_path() */
	if(!dir_list(path, lst, true)) {
		E_DEBUG(E_STRLOC ": Can't read %s\n", path);
		return;
	}

	StringListIter it, it_end;
	for(it = lst.begin(), it_end = lst.end(); it != it_end; ++it)
		add_icon_by_path((*it).c_str(), conf_ptr);
}

void Desktop::save_icons_positions(void) {
	edelib::Resource conf;
	char* icon_base;
	DesktopIconListIter it = icons.begin(), it_end = icons.end();

	for(; it != it_end; ++it) {
		if((*it)->path().empty())
			continue;

		icon_base = get_basename((*it)->path().c_str());
		conf.set(icon_base, "X", (*it)->x());
		conf.set(icon_base, "Y", (*it)->y());
	}

	if(!conf.save(ICONS_CONFIG_NAME))
		E_WARNING(E_STRLOC ": Unable to store icons positions\n");
}

bool Desktop::read_desktop_file(const char* path, IconSettings& is) {
	E_ASSERT(path != NULL);

	if(!edelib::file_exists(path)) {
		E_DEBUG(E_STRLOC ": %s don't exists\n");
		return false;
	}

	edelib::DesktopFile dconf;
	if(!dconf.load(path)) {
		E_WARNING(E_STRLOC ": Can't read %s (%s)\n", path, dconf.strerror());
		return false;
	}

	char buf[128];
	int bufsz = sizeof(buf);

	/*
	 * Check for 'EmptyIcon' key since via it is determined is icon trash type or not 
	 * (in case of trash, 'Icon' key is used for full trash).
	 *
	 * FIXME: any other way to check for trash icons ???
	 */
	if(dconf.get("Desktop Entry", "EmptyIcon", buf, bufsz)) {
		is.type = ICON_TRASH;
		is.icon = buf;
	} else
		is.type = ICON_NORMAL;
	
	if(!dconf.icon(buf, bufsz)) {
		E_WARNING(E_STRLOC ": No Icon key, balling out\n");
		return false;
	}

	if(is.type == ICON_TRASH)
		is.icon2 = buf;
	else
		is.icon = buf;

	edelib::DesktopFileType dtype = dconf.type();
	if(dtype == edelib::DESK_FILE_TYPE_LINK) {
		is.cmd_is_url = true;
		dconf.url(buf, bufsz);
	}
	else {
		is.cmd_is_url = false;
		dconf.exec(buf, bufsz);
	}

	is.cmd = buf;

	if(!dconf.name(buf, bufsz)) {
		E_DEBUG(E_STRLOC ": No Name key\n");
		is.name = "(none)";
	} else
		is.name = buf;

	return true;
}

bool Desktop::add_icon_by_path(const char* path, edelib::Resource* conf) {
	E_ASSERT(path != NULL);

	IconSettings is;
	bool can_add = false;
	const char* base = get_basename(path);

	/*
	 * see is possible .desktop file; icon, name fields are filled from read_desktop_file()
	 *
	 * FIXME: maybe the good thing is to use MimeType to check .desktop; extension does not have 
	 * to be always present, or .desktop does not have to be Desktop File at all
	 */
	if(edelib::str_ends(path, ".desktop")) {
		if(read_desktop_file(path, is))
			can_add = true;
	} else {
		edelib::MimeType mt;

		/* then try to figure out it's mime; if fails, ignore it */
		if(mt.set(path)) {
			E_DEBUG(E_STRLOC ": Loading icon as mime-type %s\n", mt.icon_name().c_str());
			is.icon = mt.icon_name();
			/* icon label path's basename */
			is.name = base;
			is.type = ICON_FILE;

			can_add = true;
		} else {
			E_DEBUG(E_STRLOC ": Failed mime-type for %s, ignoring...\n", path);
			can_add = false;
		}
	}

	if(can_add) {
		/*
		 * key_name is section in config file with icon X/Y values
		 * FIXME: this should be named 'section_name'
		 */
		is.key_name = base;

		/* random_pos() is used if X/Y keys are not found */
		int icon_x = random_pos(w() - 10);
		int icon_y = random_pos(w() - 10);

		if(conf) {
			/* we load positions from used ede-desktop-icos.conf only */
			conf->get(base, "X", icon_x, icon_x, edelib::RES_USER_ONLY);
			conf->get(base, "Y", icon_y, icon_y, edelib::RES_USER_ONLY);
		}

		E_DEBUG(E_STRLOC ": %s found with: %i %i\n", base, icon_x, icon_y);
		is.x = icon_x;
		is.y = icon_y;
		is.full_path = path;

		DesktopIcon* dic = new DesktopIcon(gisett, &is, color());
		add_icon(dic);
	}

	return can_add;
}

DesktopIcon* Desktop::find_icon_by_path(const char* path, DesktopIconListIter* ret) {
	E_ASSERT(path != NULL);

	if(icons.empty())
		return NULL;

	DesktopIconListIter it, it_end;

	for(it = icons.begin(), it_end = icons.end(); it != it_end; ++it) {
		if((*it)->path() == path) {
			/* store iterator */
			if(ret)
				*ret = it;

			/* return iterator value */
			return (*it);
		}
	}

	return NULL;
}

bool Desktop::remove_icon_by_path(const char* path) {
	DesktopIconListIter pos;
	DesktopIcon* ic = find_icon_by_path(path, &pos);

	if(!ic) {
		E_DEBUG(">>> Didn't find %s\n", path);
		return false;
	}

	icons.erase(pos);

	/* Fl_Group::remove() does not delete child, just pops it out */
	remove(ic);
	delete ic;

	return true;
}

void Desktop::update_trash_icons(void) {
	bool is_empty = edelib::dir_empty(trash_path.c_str());
	DesktopIconListIter it = icons.begin(), it_end = icons.end();

	for(; it != it_end; ++it) {
		if((*it)->icon_type() == ICON_TRASH) {
			if(is_empty)
				(*it)->use_icon1();
			else
				(*it)->use_icon2();
		}
	}
}

void Desktop::add_icon(DesktopIcon* ic) {
	E_ASSERT(ic != NULL);

	icons.push_back(ic);
	add((Fl_Widget*)ic);
}

void Desktop::unfocus_all(void) { 
	DesktopIcon* ic;
	DesktopIconListIter it, it_end;

	for(it = icons.begin(), it_end = icons.end(); it != it_end; ++it) {
		ic = (*it);

		if(ic->is_focused()) {
			ic->do_unfocus();
			ic->fast_redraw();
		}
	}
}

void Desktop::select(DesktopIcon* ic, bool do_redraw) { 
	E_ASSERT(ic != NULL);

	if(in_selection(ic))
		return;

	selectionbuf.push_back(ic);

	if(!ic->is_focused()) {
		ic->do_focus();

		if(do_redraw)
			ic->fast_redraw();
	}
}

void Desktop::select_only(DesktopIcon* ic) { 
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

	DesktopIconListIter it, it_end;

	for(it = selectionbuf.begin(), it_end = selectionbuf.end(); it != it_end; ++it) {
		if((*it) == ic)
			return true;
	}

	return false;
}

void Desktop::move_selection(int x, int y, bool apply) { 
	if(selectionbuf.empty())
		return;

	int prev_x, prev_y, tmp_x, tmp_y;
	DesktopIcon* ic;

	DesktopIconListIter it, it_end;

	for(it = selectionbuf.begin(), it_end = selectionbuf.end(); it != it_end; ++it) {
		ic = (*it);

		prev_x = ic->drag_icon_x();
		prev_y = ic->drag_icon_y();

		tmp_x = x - selection_x;
		tmp_y = y - selection_y;

		ic->drag(prev_x + tmp_x, prev_y + tmp_y, apply);
#if 0
		// very slow if not checked
		if(apply == true)
			ic->fast_redraw();
#endif
	}

	selection_x = x;
	selection_y = y;

	/* Redraw whole screen so it reflects new icon position */
	if(apply)
		redraw();
}

/*
 * Tries to figure out icon below mouse. It is alternative to Fl::belowmouse() since with this we hunt 
 * only icons, not other childs (wallpaper, menu), which can be returned by Fl::belowmouse() and bad 
 * things be hapened.
 */
DesktopIcon* Desktop::below_mouse(int px, int py) {
	DesktopIcon* ic = NULL;
	DesktopIconListIter it, it_end;
	
	for(it = icons.begin(), it_end = icons.end(); it != it_end; ++it) {
		ic = (*it);
		if(ic->x() < px && ic->y() < py && px < (ic->x() + ic->h()) && py < (ic->y() + ic->h()))
			return ic;
	}

	return NULL;
}

void Desktop::select_in_area(void) {
	if(!selbox->show)
		return;

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
	DesktopIcon* ic = NULL;
	DesktopIconListIter it, it_end;

	for(it = icons.begin(), it_end = icons.end(); it != it_end; ++it) {
		ic = (*it);
		E_ASSERT(ic != NULL && "Impossible to happen");

		if(intersects(ax, ay, ax+aw, ay+ah, ic->x(), ic->y(), ic->w()+ic->x(), ic->h()+ic->y())) {
			if(!ic->is_focused()) {
				ic->do_focus();
				// updated from Desktop::draw()
				ic->damage(EDAMAGE_CHILD_LABEL);
			}
		} else {
			if(ic->is_focused()) {
				ic->do_unfocus();
				// updated from Desktop::draw()
				ic->fast_redraw();
				//ic->damage(EDAMAGE_CHILD_LABEL);
				//ic->redraw();
				//ic->damage(EDAMAGE_CHILD_LABEL);
			}
		}
	}
}

void Desktop::notify_desktop_changed(void) {
	int num = net_get_current_desktop();
	if(num == -1)
		return;

	char** names;
	int ret = net_get_workspace_names(names);
	if(!ret)
		return;

	if(num >= ret) {
		XFreeStringList(names);
		return;
	}

	if(dbus) {
		edelib::EdbusMessage msg;
		/* send org.equinoxproject.Desktop.DesktopChanged(int32, string) signal */
		msg.create_signal(EDE_DESKTOP_OBJECT, EDE_DESKTOP_INTERFACE, "DesktopChanged");
		msg << num << names[num];
		dbus->send(msg);
	}

	XFreeStringList(names);
}

void Desktop::dnd_drop_source(const char* src, int src_len, int x, int y) {
	if(!src)
		return;

	if(src_len < 1)
		return;

	char* src_copy = new char[src_len + 1];
	int real_len = 0;

	// mozilla sends UTF-16 form; for now use this hack untill some utf8 code is ready
	for(int i = 0, j = 0; i < src_len; i++) {
		if(src[i] != 0) {
			src_copy[j++] = src[i];
			real_len++;
		}
	}
	src_copy[real_len] = '\0';

	if(real_len < 1) {
		delete [] src_copy;
		return;
	}

	E_DEBUG(E_STRLOC ": DND on Desktop, got: %s\n", src_copy);

	// valid url's (if got) ends with \r\n, clean that
	char* pp = strstr(src_copy, "\r\n");
	if(pp)
		*pp = '\0';

	char* sptr = 0;
	if((real_len > 7) && (strncmp(src_copy, "file://", 7) == 0))
		sptr = src_copy + 7;
	else
		sptr = src_copy;

	if(!edelib::file_exists(sptr) && !edelib::dir_exists(sptr)) {
		edelib::message("Droping file content is not implemented yet. Soon, soon... :)");
		delete [] src_copy;
		return;
	}

	IconSettings is;
	is.x = x;
	is.y = y;

	bool is_read = false;

	if(edelib::str_ends(src_copy, ".desktop")) {
		edelib::DesktopFile dconf;

		edelib::String path = sptr;
		edelib::DesktopFile dfile;

		if(dfile.load(path.c_str())) {
			char buf[256];

			if(dfile.type() == edelib::DESK_FILE_TYPE_LINK) {
				dfile.url(buf, 256);
				is.cmd_is_url = true;
			}
			else {
				dfile.exec(buf, 256);
				is.cmd_is_url = false;
			}
			is.cmd = buf;
			is.type = ICON_NORMAL;

			dfile.name(buf, 256);
			is.name = buf;
			dfile.icon(buf, 256);
			is.icon = buf;

			is_read = true;
		}
	}

	if(!is_read) {
		/* absolute path is (for now) seen as non-url */
		if(sptr[0] == '/')
			is.cmd_is_url = false;
		else
			is.cmd_is_url = true;

		is.cmd = "(none)";
		is.type = ICON_NORMAL;

		edelib::MimeType mt;
		if(!mt.set(sptr)) {
			E_WARNING(E_STRLOC ": MimeType for %s (%s) failed, not dropping icon\n", sptr, src_copy);
			delete [] src_copy;
			return;
		}

		is.name = get_basename(sptr);
		is.icon = mt.icon_name();
	}

	DesktopIcon* dic = new DesktopIcon(gisett, &is, color());
	add_icon(dic);

	delete [] src_copy;

	redraw();
}

void Desktop::draw(void) {
	if(!damage())
		return;

	if(damage() & (FL_DAMAGE_ALL | FL_DAMAGE_EXPOSE)) {
		/*
		 * If any overlay was previously visible during full redraw, it will not be cleared because of fast flip.
		 * This will assure that does not happened.
		 */
		clear_xoverlay();

		DESKTOP_WINDOW::draw();
		E_DEBUG(E_STRLOC ": REDRAW ALL\n");
	}

	if(damage() & EDAMAGE_OVERLAY) {
		if(selbox->show)
			draw_xoverlay(selbox->x, selbox->y, selbox->w, selbox->h);
		else
			clear_xoverlay();

		/*
		 * now scan all icons and see if they needs redraw, and if do
		 * just update their label since it is indicator of selection
		 */
		for(int i = 0; i < children(); i++) {
			if(child(i)->damage() == EDAMAGE_CHILD_LABEL) {
				update_child(*child(i));
				child(i)->clear_damage();
				E_DEBUG(E_STRLOC ": ICON REDRAW \n");
			}
		}
	}

	clear_damage();
}

void Desktop::dir_watch(const char* dir, const char* changed, int flags) {
	if(!do_dirwatch || !changed || flags == edelib::DW_REPORT_NONE)
		return;

	/* skip temporary files */
	if(is_temp_filename(changed))
		return;

	if(trash_path == dir) {
		update_trash_icons();
		E_DEBUG(E_STRLOC ": event on trash dir %s\n", dir);
		return;
	}

	if(flags == edelib::DW_REPORT_CREATE) {
		E_DEBUG(E_STRLOC ": adding %s\n", changed);

		if(find_icon_by_path(changed, 0)) {
			E_DEBUG(E_STRLOC ": %s already registered; skipping...\n", changed);
			return;
		}

		if(add_icon_by_path(changed, 0))
			redraw();

	} else if(flags == edelib::DW_REPORT_MODIFY) {
		E_DEBUG(E_STRLOC ": modified %s\n", changed);
	} else if(flags == edelib::DW_REPORT_DELETE) {
		E_DEBUG(E_STRLOC ": deleted %s\n", changed);
		if(remove_icon_by_path(changed))
			redraw();
	} else
		E_DEBUG(E_STRLOC ": %s changed with %i\n", changed, flags);
}

void Desktop::execute(const char* cmd) {
	E_ASSERT(cmd != NULL);
	edelib::run_async(cmd);
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
				E_DEBUG(E_STRLOC ": DESKTOP CLICK !!!\n");
				if(!selectionbuf.empty()) {
					/*
					 * Only focused are in selectionbuf, so this is fine to do; also will prevent 
					 * full redraw when is clicked on desktop
					 */
					unfocus_all();
					selectionbuf.clear();
				}

				// track position so moving can be deduced
				if(Fl::event_button() == 1) {
					selbox->x = Fl::event_x();
					selbox->y = Fl::event_y();
				} else if(Fl::event_button() == 3) {
					const Fl_Menu_Item* item = dmenu->menu()->popup(Fl::event_x(), Fl::event_y());
					dmenu->picked(item);
				}

				return 1;
			}

			// from here, all events are managed for icons
			DesktopIcon* tmp_icon = (DesktopIcon*)clicked;

			/*
			 * do no use assertion on this, since
			 * fltk::belowmouse() can miss our icon
			 */
			if(!tmp_icon)
				return 1;

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

			E_DEBUG(E_STRLOC ": FL_PUSH from desktop\n");
			selection_x = Fl::event_x_root();
			selection_y = Fl::event_y_root();
	
			return 1;
		 }

		case FL_DRAG:
			moving = true;
			if(!selectionbuf.empty()) {
				E_DEBUG(E_STRLOC ": DRAG icon from desktop\n");
				move_selection(Fl::event_x_root(), Fl::event_y_root(), false);
			} else {
				E_DEBUG(E_STRLOC ": DRAG from desktop\n");
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
					damage(EDAMAGE_OVERLAY);
				}
			}

			return 1;

		case FL_RELEASE:
			E_DEBUG(E_STRLOC ": RELEASE from desktop\n");
			E_DEBUG(E_STRLOC ": clicks: %i\n", Fl::event_is_click());

			if(selbox->show) {
				selbox->x = selbox->y = selbox->w = selbox->h = 0;
				selbox->show = false;
				damage(EDAMAGE_OVERLAY);

				/*
				 * Now pickup those who are in is_focused() state.
				 *
				 * Possible flickers due overlay will be later removed when is called move_selection(), which 
				 * will in turn redraw icons again after position them.
				 */
				if(!selectionbuf.empty())
					selectionbuf.clear();

				DesktopIconListIter it, it_end;
				for(it = icons.begin(), it_end = icons.end(); it != it_end; ++it) {
					if((*it)->is_focused())
						select((*it), false);
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
				(*selectionbuf.begin())->handle(FL_RELEASE);

			moving = false;
			return 1;

		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_LEAVE:
			return 1;

		case FL_DND_RELEASE: {
			DesktopIcon* di = below_mouse(Fl::event_x(), Fl::event_y());
			if(di)
				return di->handle(event);
			return 1;
		}

		case FL_PASTE: {
			DesktopIcon* di = below_mouse(Fl::event_x(), Fl::event_y());
			if(di)
				return di->handle(event);

			dnd_drop_source(Fl::event_text(), Fl::event_length(), Fl::event_x(), Fl::event_y());
			return 1;
		}

		case FL_ENTER:
		case FL_LEAVE:
		case FL_MOVE:
			return DESKTOP_WINDOW::handle(event);

		default:
			break;
	}

	return 0;
}

int main() {
	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT,  exit_signal);
	signal(SIGHUP,  restart_signal);

	srand(time(NULL));

	/* a lot of preparing code depends on this */
	fl_open_display();

#ifndef USE_EDELIB_WINDOW
	fl_register_images();
	edelib::IconLoader::init();
#endif

	Desktop::init();
	Desktop::instance()->show();

	/* XSelectInput will redirect PropertyNotify messages, which are listened for */
	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), PropertyChangeMask | StructureNotifyMask | ClientMessage);

	Fl::add_handler(desktop_xmessage_handler);

	while(running) 
		Fl::wait();

	Desktop::shutdown();

#ifndef USE_EDELIB_WINDOW
	edelib::IconLoader::shutdown();
#endif
	return 0;
}
