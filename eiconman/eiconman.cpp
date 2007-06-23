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

#include "eiconman.h"
#include "DesktopIcon.h"
#include "Utils.h"
#include "Wallpaper.h"
#include "NotifyBox.h"

#include <edelib/Debug.h>
#include <edelib/File.h>
#include <edelib/Directory.h>
#include <edelib/MimeType.h>
#include <edelib/StrUtil.h>
#include <edelib/IconTheme.h>

#include <FL/Fl.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/x.h>
#include <FL/Fl_Box.h>

#include <signal.h>
#include <stdlib.h> // rand, srand
#include <time.h>   // time

#define CONFIG_NAME  "eiconman.conf"

#define SELECTION_SINGLE (Fl::event_button() == 1)
#define SELECTION_MULTI (Fl::event_button() == 1 && (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R)))

#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

/*
 * Which widgets Fl::belowmouse() should skip. This should be updated
 * when new non-icon child is added to Desktop window.
 */
#define NOT_SELECTABLE(widget) ((widget == this) || (widget == wallpaper) || (widget == notify))

Desktop* Desktop::pinstance = NULL;
bool running = false;

inline unsigned int random_pos(int max) { 
	return (rand() % max); 
}

inline bool intersects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (MAX(x1, x2) <= MIN(w1, w2) && 
			MAX(y1, y2) <= MIN(h1, h2));
}

// assume fl_open_display() is called before
inline void dpy_sizes(int& width, int& height) {
	width = DisplayWidth(fl_display, fl_screen);
	height = DisplayHeight(fl_display, fl_screen);
}

void exit_signal(int signum) {
    EDEBUG(ESTRLOC ": Exiting (got signal %d)\n", signum);
    running = false;
}

void restart_signal(int signum) {
	EDEBUG(ESTRLOC ": Restarting (got signal %d)\n", signum);
}

int desktop_xmessage_handler(int event) { 
	if(fl_xevent->type == PropertyNotify) {
		if(fl_xevent->xproperty.atom == _XA_NET_CURRENT_DESKTOP) {
			Desktop::instance()->notify_desktop_changed();
			return 1;
		}

		if(fl_xevent->xproperty.atom == _XA_EDE_DESKTOP_NOTIFY) {
			char buff[256];
			if(ede_get_desktop_notify(buff, 256) && buff[0] != '\0')
				Desktop::instance()->notify_box(buff, true);
			return 1;
		}

		//if(fl_xevent->xclient.message_type == _XA_EDE_DESKTOP_NOTIFY_COLOR) {
		if(fl_xevent->xproperty.atom == _XA_EDE_DESKTOP_NOTIFY_COLOR) {
			Desktop::instance()->notify_box_color(ede_get_desktop_notify_color());
			return 1;
		}

		if(fl_xevent->xproperty.atom == _XA_NET_WORKAREA) {
			Desktop::instance()->update_workarea();
			return 1;
		}
	}

	return 0; 
}

Desktop::Desktop() : Fl_Window(0, 0, 100, 100, "") {
	selection_x = selection_y = 0;
	moving = false;

	dsett = new DesktopSettings;
	dsett->color = FL_GRAY;
	dsett->wp_use = false;

	init_atoms();
	update_workarea();

	/*
	 * NOTE: order how childs are added is important. First
	 * non iconable ones should be added (wallpaper, menu, ...)
	 * then icons, so they can be drawn at top of them.
	 */
	begin();
		wallpaper = new Wallpaper(0, 0, w(), h());
		wallpaper->set("/home/sanel/wallpapers/katebig.jpg");
		//wallpaper->hide();
		//wallpaper->set("/home/sanelz/walls/katebig.jpg");
		//wallpaper->set("/home/sanelz/walls/nin/1024x768-04.jpg");
		//wallpaper->set("/home/sanelz/walls/nin/1024x768-02.jpg");
		notify = new NotifyBox(w(), h());
		notify->hide();
	end();

	read_config();

	set_bg_color(dsett->color, false);
	running = true;
}

Desktop::~Desktop() { 
	EDEBUG("Desktop::~Desktop()\n");

	delete dsett;
}

void Desktop::init(void) {
	if(Desktop::pinstance != NULL)
		return;
	Desktop::pinstance = new Desktop();
}

void Desktop::shutdown(void) {
	if(Desktop::pinstance == NULL)
		return;

	delete Desktop::pinstance;
	Desktop::pinstance = NULL;
}

Desktop* Desktop::instance(void) {
	EASSERT(Desktop::pinstance != NULL && "Desktop::init() should be run first");
	return Desktop::pinstance;
}

/*
 * This function must be overriden so window can inform
 * wm to see it as desktop. It will send data when window
 * is created, but before is shown.
 */
void Desktop::show(void) {
	if(!shown()) {
		Fl_X::make_xid(this);
		net_make_me_desktop(this);
	}

	//Fl::dnd_text_ops(1);
}

/*
 * If someone intentionaly hide desktop
 * then quit from it.
 */
void Desktop::hide(void) {
	running = false;
}

void Desktop::update_workarea(void) {
	int X, Y, W, H;
	if(!net_get_workarea(X, Y, W, H)) {
		EWARNING(ESTRLOC ": wm does not support _NET_WM_WORKAREA; using screen sizes...\n");
		X = Y = 0;
		dpy_sizes(W, H);
	}

	resize(X, Y, W, H);
}

void Desktop::set_bg_color(int c, bool do_redraw) {
	if(color() == c)
		return;

	dsett->color = c;
	color(c);
	if(do_redraw)
		redraw();
}

void Desktop::read_config(void) {
	edelib::Config conf;
	if(!conf.load(CONFIG_NAME)) {
		EWARNING(ESTRLOC ": Can't load %s, using default...\n", CONFIG_NAME);
		return;
	}

	/*
	 * TODO:
	 * Add IconArea[X,Y,W,H] so icons can live
	 * inside that area only (aka margins).
	 */
	int  default_bg_color = FL_BLUE;
	int  default_wp_use   = false;
	char wpath[256];

	// read Desktop section
	conf.get("Desktop", "Color", dsett->color, default_bg_color);

	if(conf.error() != edelib::CONF_ERR_SECTION) {
		conf.get("Desktop", "WallpaperUse", dsett->wp_use, default_wp_use);
		conf.get("Desktop", "Wallpaper", wpath, sizeof(wpath));

		// keep path but disable wallpaper if file does not exists
		if(!edelib::file_exists(wpath)) {
			EDEBUG(ESTRLOC ": %s as wallpaper does not exists\n", wpath);
			dsett->wp_use = false;
		}
	} else {
		// color is already filled
		dsett->wp_use = default_wp_use;
	}

	// read Icons section
	conf.get("Icons", "Label Background", gisett.label_background, FL_BLUE);
	conf.get("Icons", "Label Foreground", gisett.label_foreground, FL_WHITE);
	conf.get("Icons", "Label Fontsize",   gisett.label_fontsize, 12);
	conf.get("Icons", "Label Maxwidth",   gisett.label_maxwidth, 75);
	conf.get("Icons", "Label Transparent",gisett.label_transparent, false);
	conf.get("Icons", "Label Visible",    gisett.label_draw, true);
	conf.get("Icons", "OneClickExec",     gisett.one_click_exec, false);
	conf.get("Icons", "AutoArrange",      gisett.auto_arrange, true);

	/*
	 * Now try to load icons, first looking inside ~/Desktop directory
	 * then inside config since config could contain removed entries
	 * from $HOME/Desktop
	 *
	 * FIXME: dir_exists() can't handle '~/Desktop' ???
	 */
	edelib::String dd = edelib::dir_home();
	if(dd.empty()) {
		EWARNING(ESTRLOC ": Can't read home directory; icons will not be loaded\n");
		return;
	}
	dd += "/Desktop";
	load_icons(dd.c_str(), conf);
}

void Desktop::save_config(void) {
	// TODO
}

void Desktop::load_icons(const char* path, edelib::Config& conf) {
	EASSERT(path != NULL);

	if(!edelib::dir_exists(path)) {
		EDEBUG(ESTRLOC ": %s does not exists\n", path);
		return;
	}

	edelib::vector<edelib::String> lst;

	// list without full path, so we can use name as entry in config file
	if(!dir_list(path, lst)) {
		EDEBUG(ESTRLOC ": Can't read %s\n", path);
		return;
	}

	const char* name = NULL;
	int icon_x = 0;
	int icon_y = 0;
	edelib::String full_path;
	full_path.reserve(256);

	bool can_add = false;

	edelib::MimeType mt;
	
	unsigned int sz = lst.size();
	for(unsigned int i = 0; i < sz; i++) {
		name = lst[i].c_str();

		full_path = path;
		full_path += '/';
		full_path += name;

		can_add = false;

		IconSettings is;

		// see is possible .desktop file, icon, name fields are filled from read_desktop_file()
		if(edelib::str_ends(name, ".desktop")) {
			if(read_desktop_file(full_path.c_str(), is))
				can_add = true;
		} else {
			// then try to figure out it's mime; if fails, ignore it
			if(mt.set(full_path.c_str())) {
				EDEBUG(ESTRLOC ": Loading icon as mime-type %s\n", mt.icon_name().c_str());
				is.icon = mt.icon_name();
				// icon label is name of file
				is.name = name;
				is.type = ICON_FILE;

				can_add = true;
			} else {
				EDEBUG(ESTRLOC ": Failed mime-type for %s, ignoring...\n", name);
				can_add = false;
			}
		}

		if(can_add) {
			is.key_name = name;
			// random_pos() is used if X/Y keys are not found
			conf.get(name, "X", icon_x, random_pos(w() - 10));
			conf.get(name, "Y", icon_y, random_pos(h() - 10));

			EDEBUG(ESTRLOC ": %s found with: %i %i\n", name, icon_x, icon_y);
			is.x = icon_x;
			is.y = icon_y;

			DesktopIcon* dic = new DesktopIcon(&gisett, &is, dsett->color);
			add_icon(dic);
		}
	}
}

// read .desktop files
bool Desktop::read_desktop_file(const char* path, IconSettings& is) {
	EASSERT(path != NULL);

	if(!edelib::file_exists(path)) {
		EDEBUG(ESTRLOC ": %s don't exists\n");
		return false;
	}

	edelib::Config dconf;
	if(!dconf.load(path)) {
		EDEBUG(ESTRLOC ": Can't read %s\n", path);
		return false;
	}

	char buff[128];
	int buffsz = sizeof(buff);

	/*
	 * First check for 'EmptyIcon' key since via it is determined
	 * is icon trash type or not (in case of trash, 'Icon' key is used for full trash).
	 * FIXME: any other way to check for trash icons ???
	 */
	if(dconf.get("Desktop Entry", "EmptyIcon", buff, buffsz)) {
		is.type = ICON_TRASH;
		is.icon = buff;
	} else
		is.type = ICON_NORMAL;

	if(dconf.error() == edelib::CONF_ERR_SECTION) {
		EDEBUG(ESTRLOC ": %s is not valid .desktop file\n");
		return false;
	}

	dconf.get("Desktop Entry", "Icon", buff, buffsz);

	if(is.type == ICON_TRASH)
		is.icon2 = buff;
	else {
		is.icon = buff;
		is.type = ICON_NORMAL;
	}

	EDEBUG(ESTRLOC ": Icon is: %s\n", is.icon.c_str());

	char name[256];
	// FIXME: UTF-8 safety
	if(dconf.get_localized("Desktop Entry", "Name", name, sizeof(name))) {
		EDEBUG(ESTRLOC ": Name is: %s\n", name);
		is.name = name;
	}

	/*
	 * Specs (desktop entry file) said that Type=Link means there must
	 * be somewhere URL key. My thoughts is that in this case Exec key
	 * should be ignored, even if exists. Then I will follow my thoughts.
	 *
	 * FIXME: 'Type' should be seen as test for .desktop file; if key
	 * is not present, then file should not be considered as .desktop. This
	 * should be checked before all others.
	 */
	if(!dconf.get("Desktop Entry", "Type", buff, buffsz)) {
		EDEBUG(ESTRLOC ": Missing mandatory Type key\n");
		return false;
	}

	if(strncmp(buff, "Link", 4) == 0) {
		is.cmd_is_url = true;
		if(!dconf.get("Desktop Entry", "URL", buff, buffsz)) {
			EDEBUG(ESTRLOC ": Missing expected URL key\n");
			return false;
		}
		is.cmd = buff;
	} else if(strncmp(buff, "Application", 11) == 0) {
		is.cmd_is_url = false;
		if(!dconf.get("Desktop Entry", "Exec", buff, buffsz)) {
			EDEBUG(ESTRLOC ": Missing expected Exec key\n");
			return false;
		}
		is.cmd = buff;
	} else if(strncmp(buff, "Directory", 11) == 0) {
		EDEBUG(ESTRLOC ": Type = Directory is not implemented yet\n");
		return false;
	} else {
		EDEBUG(ESTRLOC ": Unknown %s type, ignoring...\n", buff);
		return false;
	}

	return true;
}

void Desktop::add_icon(DesktopIcon* ic) {
	EASSERT(ic != NULL);

	icons.push_back(ic);
	// FIXME: validate this
	add((Fl_Widget*)ic);
}

void Desktop::unfocus_all(void) { 
	unsigned int sz = icons.size();
	DesktopIcon* ic;

	for(unsigned int i = 0; i < sz; i++) {
		ic = icons[i];
		if(ic->is_focused()) {
			ic->do_unfocus();
			ic->fast_redraw();
		}
	}
}

void Desktop::select(DesktopIcon* ic) { 
	EASSERT(ic != NULL);

	if(in_selection(ic))
		return;

	selectionbuff.push_back(ic);

	if(!ic->is_focused()) {
		ic->do_focus();
		ic->fast_redraw();
	}
}

void Desktop::select_only(DesktopIcon* ic) { 
	EASSERT(ic != NULL);

	unfocus_all();
	selectionbuff.clear();
	selectionbuff.push_back(ic);

	ic->do_focus();
	ic->fast_redraw();
}

bool Desktop::in_selection(const DesktopIcon* ic) { 
	EASSERT(ic != NULL);

	unsigned int sz = selectionbuff.size();
	for(unsigned int i = 0; i < sz; i++) {
		if(ic == selectionbuff[i])
			return true;
	}

	return false;
}

void Desktop::move_selection(int x, int y, bool apply) { 
	unsigned int sz = selectionbuff.size();
	if(sz == 0)
		return;

	int prev_x, prev_y, tmp_x, tmp_y;
	DesktopIcon* ic;

	for(unsigned int i = 0; i < sz; i++) {
		ic = selectionbuff[i];

		prev_x = ic->drag_icon_x();
		prev_y = ic->drag_icon_y();

		tmp_x = x - selection_x;
		tmp_y = y - selection_y;

		ic->drag(prev_x + tmp_x, prev_y + tmp_y, apply);

		// very slow if not checked
		if(apply == true)
			ic->fast_redraw();
	}

	selection_x = x;
	selection_y = y;

	/*
	 * Redraw whole screen so it reflects
	 * new icon position
	 */
	if(apply)
		redraw();
}

#if 0
/*
 * Tries to figure out icon below mouse. It is alternative to
 * Fl::belowmouse() since with this we hunt only icons, not other
 * childs (wallpaper, menu), which can be returned by Fl::belowmouse()
 * and bad things be hapened.
 */
DesktopIcon* Desktop::below_mouse(int px, int py) {
	unsigned int sz = icons.size();

	DesktopIcon* ic = NULL;
	for(unsigned int i = 0; i < sz; i++) {
		ic = icons[i];
		if(ic->x() < px && ic->y() < py && px < (ic->x() + ic->h()) && py < (ic->y() + ic->h()))
			return ic;
	}

	return NULL;
}
#endif

void Desktop::notify_box(const char* msg, bool copy) {
	if(!msg)
		return;

	if(copy)
		notify->copy_label(msg);
	else	
		notify->label(msg);

	notify->show();
}

void Desktop::notify_box_color(Fl_Color col) {
	notify->color(col);
}

void Desktop::notify_desktop_changed(void) {
	int num = net_get_current_desktop();
	if(num == -1)
		return;

	char** names;
	int ret = net_get_workspace_names(names);
	if(num >= ret) {
		XFreeStringList(names);
		return;
	}

	notify_box(names[num], true);
	XFreeStringList(names);
}

void Desktop::drop_source(const char* src, int x, int y) {
	if(!src)
		return;
	IconSettings is;
	is.x = x;
	is.y = y;

	// absolute path is (for now) seen as non-url
	if(src[0] == '/')
		is.cmd_is_url = false;
	else
		is.cmd_is_url = true;

	is.name = get_basename(src);
	is.cmd = "(none)";
	is.type = ICON_NORMAL;

	edelib::MimeType mt;
	if(!mt.set(src)) {
		EWARNING(ESTRLOC ": MimeType for %s failed, not dropping icon\n", src);
		return;
	}

	is.icon = mt.icon_name();
	DesktopIcon* dic = new DesktopIcon(&gisett, &is, color());
	add_icon(dic);
}

int Desktop::handle(int event) {
	switch(event) {
		case FL_FOCUS:
		case FL_UNFOCUS:
		case FL_SHORTCUT:
			return 1;

		case FL_PUSH: {
			/*
			 * First check where we clicked. If we do it on desktop
			 * unfocus any possible focused childs, and handle
			 * specific clicks. Otherwise, do rest for childs.
			 */
			Fl_Widget* clicked = Fl::belowmouse();
			
			if(NOT_SELECTABLE(clicked)) {
				EDEBUG(ESTRLOC ": DESKTOP CLICK !!!\n");
				if(!selectionbuff.empty()) {
					/*
					 * only focused are in selectionbuff, so this is 
					 * fine to do; also will prevent full redraw when
					 * is clicked on desktop
					 */
					unfocus_all();
					selectionbuff.clear();
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
					/*
					 * for testing
					 * notify_box(tmp_icon->label());
					 */
					select_only(tmp_icon);
				}
			} else if(Fl::event_button() == 3) {
				select_only(tmp_icon);
				/*
				 * for testing
				 * notify_box(tmp_icon->label());
				 */
			}

			/* 
			 * Let child handle the rest.
			 * Also prevent click on other mouse buttons during move.
			 */
			if(!moving)
				tmp_icon->handle(FL_PUSH);

			EDEBUG(ESTRLOC ": FL_PUSH from desktop\n");
			selection_x = Fl::event_x_root();
			selection_y = Fl::event_y_root();
	
			return 1;
		 }

		case FL_DRAG:
			moving = true;
			if(!selectionbuff.empty()) {
				EDEBUG(ESTRLOC ": DRAG icon from desktop\n");
				move_selection(Fl::event_x_root(), Fl::event_y_root(), false);
			} else {
				EDEBUG(ESTRLOC ": DRAG from desktop\n");
			}

			return 1;

		case FL_RELEASE:
			EDEBUG(ESTRLOC ": RELEASE from desktop\n");
			EDEBUG(ESTRLOC ": clicks: %i\n", Fl::event_is_click());

			if(!selectionbuff.empty() && moving)
				move_selection(Fl::event_x_root(), Fl::event_y_root(), true);

			/* 
			 * Do not send FL_RELEASE during move
			 *
			 * TODO: should be alowed FL_RELEASE to multiple icons? (aka. run 
			 * command for all selected icons ?
			 */
			if(selectionbuff.size() == 1 && !moving)
				selectionbuff[0]->handle(FL_RELEASE);

			moving = false;
			return 1;

		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_LEAVE:
			return 1;

		case FL_DND_RELEASE:
			EDEBUG(ESTRLOC ": DND on desktop\n");
			return 1;

		case FL_PASTE:
			EDEBUG("================> PASTE: %s\n", Fl::event_text());
			drop_source(Fl::event_text(), Fl::event_x_root(), Fl::event_y_root());
			return 1;

		default:
			break;
	}

	return Fl_Window::handle(event);
}

int main() {
	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT,  exit_signal);
	signal(SIGHUP,  restart_signal);

	srand(time(NULL));

	// a lot of preparing code depends on this
	fl_open_display();

	fl_register_images();

	edelib::IconTheme::init("edeneu");

	Desktop::init();
	Desktop::instance()->show();

	/*
	 * XSelectInput will redirect PropertyNotify messages, which
	 * are listened for
	 */
	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), PropertyChangeMask | StructureNotifyMask | ClientMessage);

	Fl::add_handler(desktop_xmessage_handler);

	while(running) 
		Fl::wait();

	Desktop::shutdown();
	edelib::IconTheme::shutdown();

	return 0;
}
