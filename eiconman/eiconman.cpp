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
#include "DesktopConfig.h"
#include "Utils.h"

#include <edelib/Nls.h>
#include <edelib/Debug.h>
#include <edelib/Config.h>
#include <edelib/Directory.h>
#include <edelib/StrUtil.h>
#include <edelib/File.h>
#include <edelib/IconTheme.h>
#include <edelib/Item.h>
#include <edelib/MimeType.h>

#include <fltk/Divider.h>
#include <fltk/damage.h>
#include <fltk/Color.h>
#include <fltk/events.h>
#include <fltk/run.h>
#include <fltk/x11.h>
#include <fltk/SharedImage.h>

#include <signal.h>
#include <X11/Xproto.h> // CARD32

#include <stdlib.h> // rand, srand
#include <time.h>   // time
#include <stdio.h>  // snprintf

/*
 * NOTE: DO NOT set 'using namespace fltk' here
 * since fltk::Window will collide with Window from X11
 * resulting compilation errors.
 *
 * This is why I hate this namespace shit !
 */

#define CONFIG_NAME          "eiconman.conf"

#define SELECTION_SINGLE (fltk::event_button() == 1)
#define SELECTION_MULTI (fltk::event_button() == 1 && \
	 (fltk::get_key_state(fltk::LeftShiftKey) ||\
	  fltk::get_key_state(fltk::RightShiftKey)))

#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

/*
 * Added since fltk DAMAGE_OVERLAY value is used in few different contexts
 * and re-using it will do nothing. Yuck!
 */
#define EDAMAGE_OVERLAY  2

Desktop* Desktop::pinstance = NULL;
bool running = true;

inline unsigned int random_pos(int max) { 
	return (rand() % max); 
}

inline bool intersects(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (MAX(x1, x2) <= MIN(w1, w2) && 
			MAX(y1, y2) <= MIN(h1, h2));
}

inline void dpy_sizes(int& width, int& height) {
	fltk::open_display();
	width = DisplayWidth(fltk::xdisplay, fltk::xscreen);
	height = DisplayHeight(fltk::xdisplay, fltk::xscreen);
}

void exit_signal(int signum) {
    EDEBUG(ESTRLOC ": Exiting (got signal %d)\n", signum);
    running = false;
}

/*
 * It is used to notify desktop when _NET_CURRENT_DESKTOP is triggered.
 * FIXME: _NET_WORKAREA is nice thing that could set here too :)
 *
 * FIXME: XInternAtom should be placed somewhere else
 */
int desktop_xmessage_handler(int e, fltk::Window*) {
	Atom nd = XInternAtom(fltk::xdisplay, "_NET_CURRENT_DESKTOP", False);

	if(fltk::xevent.type == PropertyNotify) {
		if(fltk::xevent.xproperty.atom == nd) {
			EDEBUG(ESTRLOC ": Desktop changed !!!\n");
			return 1;
		}
	}

	return 0;
}

void background_cb(fltk::Widget*, void*) {
	DesktopConfig dc;
	dc.run();
}

Desktop::Desktop() : fltk::Window(0, 0, 100, 100, "")
{
	moving = false;

	desktops_num = 0;
	curr_desktop = 0;

	selbox = new SelectionOverlay;
	selbox->x = selbox->y = selbox->w = selbox->h = 0;
	selbox->show = false;

	dsett = new DesktopSettings;
	dsett->color = 0;
	dsett->wp_use = false;
	dsett->wp_image = NULL;

	// fallback if update_workarea() fails
	int dw, dh;
	dpy_sizes(dw, dh);
	resize(dw, dh);

	update_workarea();

	read_config();

	begin();

	pmenu = new fltk::PopupMenu(0, 0, 450, 50);
	pmenu->begin();
		edelib::Item* it = new edelib::Item(_("&New desktop item"));
		it->offset_x(12, 12);
		new fltk::Divider();
		it = new edelib::Item(_("&Line up vertical"));
		it->offset_x(12, 12);
		it = new edelib::Item(_("&Line up horizontal"));
		it->offset_x(12, 12);
		it = new edelib::Item(_("&Arrange by name"));
		it->offset_x(12, 12);
		new fltk::Divider();
		it = new edelib::Item(_("&Icon settings"));
		it->offset_x(12, 12);

		edelib::Item* itbg = new edelib::Item(_("&Background settings"));
		itbg->offset_x(12, 12);
		itbg->callback(background_cb);
	pmenu->end();
	pmenu->type(fltk::PopupMenu::POPUP3);

	end();

	if(dsett->wp_use)
		set_wallpaper(dsett->wp_path.c_str(), false);
	else
		set_bg_color(dsett->color, false);
}

Desktop::~Desktop() {
	EDEBUG(ESTRLOC ": Desktop::~Desktop()\n");

	save_config();

	if(selbox)
		delete selbox;

	/*
	 * icons member deleting is not needed, since add_icon will
	 * append to icons and to fltk::Group array. Desktop at the end
	 * will cleanup fltk::Group array.
	 */
	icons.clear();

	if(dsett)
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

void Desktop::update_workarea(void) {
	int X,Y,W,H;
	if(net_get_workarea(X, Y, W, H))
    	resize(X,Y,W,H);
}

void Desktop::set_bg_color(unsigned int c, bool do_redraw) {
	EASSERT(dsett != NULL);

	dsett->color = c;
	color(c);

	if(do_redraw)
		redraw();
}

void Desktop::set_wallpaper(const char* path, bool do_redraw) {
	EASSERT(path != NULL);
	EASSERT(dsett != NULL);
	/*
	 * Prevent cases 'set_wallpaper(dsett->wp_path.c_str())' since assignement
	 * will nullify pointers. Very hard to find bug! (believe me, after few hours)
	 */
	if(dsett->wp_path.c_str() != path)
		dsett->wp_path = path;

	dsett->wp_image = fltk::SharedImage::get(path);
	/*
	 * SharedImage::get() will return NULL if is unable to read the image
	 * and that is exactly what is wanted here since draw() function will
	 * skip drawing image in nulled case. Blame user for this :)
	 */
	image(dsett->wp_image);

	if(do_redraw)
		redraw();
}

void Desktop::set_wallpaper(fltk::Image* im, bool do_redraw) {
	if(dsett->wp_image == im)
		return;

	image(dsett->wp_image);
	if(do_redraw)
		redraw();
}

void Desktop::read_config(void) {
	edelib::Config conf;
	if(!conf.load(CONFIG_NAME)) {
		EDEBUG(ESTRLOC ": Can't load %s, using default...\n", CONFIG_NAME);
		return;
	}

	/*
	 * TODO:
	 * Add IconArea[X,Y,W,H] so icons can live
	 * inside that area only (aka margins).
	 */

	// read Desktop section
	int  default_bg_color = fltk::BLUE;
	int  default_wp_use   = false;
	char wpath[256];

	conf.get("Desktop", "Color", dsett->color, default_bg_color);

	if(conf.error() != edelib::CONF_ERR_SECTION) {
		conf.get("Desktop", "WallpaperUse", dsett->wp_use, default_wp_use);
		conf.get("Desktop", "Wallpaper", wpath, sizeof(wpath));

		dsett->wp_path = wpath;

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
	conf.get("Icons", "Label Background", gisett.label_background, 46848);
	conf.get("Icons", "Label Foreground", gisett.label_foreground, fltk::WHITE);
	conf.get("Icons", "Label Fontsize",   gisett.label_fontsize, 12);
	conf.get("Icons", "Label Maxwidth",   gisett.label_maxwidth, 75);
	conf.get("Icons", "Label Transparent",gisett.label_transparent, false);
	conf.get("Icons", "Label Visible",    gisett.label_draw, true);
	conf.get("Icons", "Gridspacing",      gisett.gridspacing, 16);
	conf.get("Icons", "OneClickExec",     gisett.one_click_exec, false);
	conf.get("Icons", "AutoArrange",      gisett.auto_arr, true);

	/*
	 * Now try to load icons, first looking inside ~/Desktop directory
	 * then inside config since config could contain removed entries
	 * from $HOME/Desktop
	 *
	 * FIXME: dir_exists() can't handle '~/Desktop' ???
	 */
	//load_icons("/home/sanel/Desktop", conf);
	edelib::String dd = edelib::dir_home();
	dd += "/Desktop";
	load_icons(dd.c_str(), conf);

#if 0
	EDEBUG("----------------------------------------------------------\n");
	EDEBUG("d Color       : %i\n", bg_color);
	EDEBUG("d WallpaperUse: %i\n", wp_use);
	EDEBUG("i label bkg   : %i\n", gisett.label_background);
	EDEBUG("i label fg    : %i\n", gisett.label_foreground);
	EDEBUG("i label fsize : %i\n", gisett.label_fontsize);
	EDEBUG("i label maxw  : %i\n", gisett.label_maxwidth);
	EDEBUG("i label trans : %i\n", gisett.label_transparent);
	EDEBUG("i label vis   : %i\n", gisett.label_draw);
	EDEBUG("i gridspace   : %i\n", gisett.gridspacing);
	EDEBUG("i oneclick    : %i\n", gisett.one_click_exec);
	EDEBUG("i auto_arr    : %i\n", gisett.auto_arr);
#endif
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
				/*
				 * FIXME: MimeType fails for directories
				 * Temp solution untill that is fixed in edelib
				 */
				if(edelib::dir_exists(full_path.c_str()))
					is.icon = "folder";
				else
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

			DesktopIcon* dic = new DesktopIcon(&gisett, &is);
			add_icon(dic);
		}
	}
}

// reads .desktop file content
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

void Desktop::save_config(void) {
	edelib::Config conf;

	conf.set("Desktop", "Color", dsett->color);
	conf.set("Desktop", "WallpaperUse", dsett->wp_use);
	conf.set("Desktop", "Wallpaper", dsett->wp_path.c_str());

	conf.set("Icons", "Label Background", gisett.label_background);
	conf.set("Icons", "Label Foreground", gisett.label_foreground);
	conf.set("Icons", "Label Fontsize",   gisett.label_fontsize);
	conf.set("Icons", "Label Maxwidth",   gisett.label_maxwidth);
	conf.set("Icons", "Label Transparent",gisett.label_transparent);
	conf.set("Icons", "Label Visible",    gisett.label_draw);
	conf.set("Icons", "Gridspacing",      gisett.gridspacing);
	conf.set("Icons", "OneClickExec",     gisett.one_click_exec);
	conf.set("Icons", "AutoArrange",      gisett.auto_arr);

	unsigned int sz = icons.size();
	const IconSettings* is = NULL;

	for(unsigned int i = 0; i < sz; i++) {
		is = icons[i]->get_settings();
		conf.set(is->key_name.c_str(), "X", icons[i]->x());
		conf.set(is->key_name.c_str(), "Y", icons[i]->y());
	}

	if(!conf.save(CONFIG_NAME))
		EDEBUG(ESTRLOC ": Unable to save to %s\n", CONFIG_NAME);
}

void Desktop::add_icon(DesktopIcon* ic) {
	EASSERT(ic != NULL);

	icons.push_back(ic);
	add(ic);
}

void Desktop::move_selection(int x, int y, bool apply) {
	unsigned int sz = selectionbuff.size();	
	if(sz == 0)
		return;

	int prev_x, prev_y, tmp_x, tmp_y;

	for(unsigned int i = 0; i < sz; i++) {
		prev_x = selectionbuff[i]->drag_icon_x();
		prev_y = selectionbuff[i]->drag_icon_y();

		tmp_x = x - selection_x;
		tmp_y = y - selection_y;

		selectionbuff[i]->drag(prev_x+tmp_x, prev_y+tmp_y, apply);

		// very slow if is not checked
		if(apply == true)
			selectionbuff[i]->redraw();
	}

	selection_x = x;
	selection_y = y;
}

void Desktop::unfocus_all(void) {
	unsigned int sz = icons.size();

	for(unsigned int i = 0; i < sz; i++) {
		if(icons[i]->is_focused()) {
			icons[i]->do_unfocus();
			icons[i]->redraw();
		}
	}

	redraw();
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

void Desktop::select(DesktopIcon* ic) {
	EASSERT(ic != NULL);

	if(in_selection(ic))
		return;

	selectionbuff.push_back(ic);

	if(!ic->is_focused()) {
		ic->do_focus();
		ic->redraw();
	}

	redraw();
}

void Desktop::select_noredraw(DesktopIcon* ic) {
	EASSERT(ic != NULL);

	if(in_selection(ic))
		return;
	selectionbuff.push_back(ic);
}

void Desktop::select_only(DesktopIcon* ic) {
	EASSERT(ic != NULL);

	unfocus_all();

	selectionbuff.clear();
	selectionbuff.push_back(ic);

	ic->do_focus();
	ic->redraw();

	redraw();
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
	unsigned int sz = icons.size();
	DesktopIcon* ic = NULL;

	for(unsigned int i = 0; i < sz; i++) {
		ic = icons[i];
		EASSERT(ic != NULL && "Impossible to happen");

		if(intersects(ax, ay, ax+aw, ay+ah, ic->x(), ic->y(), ic->w()+ic->x(), ic->h()+ic->y())) {
			if(!ic->is_focused()) {
				ic->do_focus();
				ic->redraw();
			}
		} else {
			if(ic->is_focused()) {
				ic->do_unfocus();
				ic->redraw();
			}
		}
	}
}

/*
 * Tries to figure out icon below mouse (used for DND)
 * If fails, return NULL
 */
DesktopIcon* Desktop::below_mouse(int px, int py) {
	unsigned int sz = icons.size();

	DesktopIcon* ic = NULL;
	for(unsigned int i = 0; i < sz; i++) {
		ic = icons[i];
		EASSERT(ic != NULL && "Impossible to happen");

		if(ic->x() < px && ic->y() < py && px < (ic->x() + ic->h()) && py < (ic->y() + ic->h()))
			return ic;
	}

	return NULL;
}


// used to drop dnd context on desktop figuring out what it can be
void Desktop::drop_source(const char* src, int x, int y) {
	if(!src)
		return;
	IconSettings is;
	is.x = x;
	is.y = y;

	// absolute path we (for now) see as non-url
	if(src[0] == '/')
		is.cmd_is_url = false;
	else
		is.cmd_is_url = true;

	is.name = "XXX";
	is.cmd  = "(none)";
	is.type = ICON_NORMAL;
	
	edelib::MimeType mt;
	if(!mt.set(src)) {
		EDEBUG("MimeType for %s failed, not dropping icon\n", src);
		return;
	}

	is.icon = mt.icon_name();

	EDEBUG("---------> %s\n", is.icon.c_str());

	DesktopIcon* dic = new DesktopIcon(&gisett, &is);
	add_icon(dic);
}

void Desktop::draw(void) {
#if 0
	if(damage() & fltk::DAMAGE_ALL) {
		fltk::Window::draw();
	}
#endif

	if(damage() & fltk::DAMAGE_ALL) {
		clear_flag(fltk::HIGHLIGHT);

		int nchild = children();
		if(damage() & ~fltk::DAMAGE_CHILD) {
			draw_box();
			draw_label();

			for(int i = 0; i < nchild; i++) {
				fltk::Widget& ch = *child(i);
				draw_child(ch);
				draw_outside_label(ch);
			}
		} else {
			for(int i = 0; i < nchild; i++) {
				fltk::Widget& ch = *child(i);
				if(ch.damage() & fltk::DAMAGE_CHILD_LABEL) {
					draw_outside_label(ch);
					ch.set_damage(ch.damage() & ~fltk::DAMAGE_CHILD_LABEL);
				}
				update_child(ch);
			}
		}
	}

	if(damage() & (fltk::DAMAGE_ALL|EDAMAGE_OVERLAY)) {
		clear_xoverlay();

		if(selbox->show)
			draw_xoverlay(selbox->x, selbox->y, selbox->w, selbox->h);
		/*
		 * now scan all icons and see if they needs redraw, and if do
		 * just update their label since it is indicator of selection
		 */
		for(int i = 0; i < children(); i++) {
			if(child(i)->damage() == fltk::DAMAGE_ALL) {
				child(i)->set_damage(fltk::DAMAGE_CHILD_LABEL);
				update_child(*child(i));
			}
		}
	}
}

int Desktop::handle(int event) {
	switch(event) {
		case fltk::FOCUS:
		case fltk::UNFOCUS:
			return 1;

		case fltk::PUSH: {
			/*
			 * First check where we clicked. If we do it on desktop
			 * unfocus any possible focused childs, and handle
			 * specific clicks. Otherwise, do rest for childs.
			 */
			fltk::Widget* clicked = fltk::belowmouse();
			EDEBUG(ESTRLOC ": %i\n", fltk::event_button());

			if(clicked == this) {
				unfocus_all();
				EDEBUG(ESTRLOC ": DESKTOP CLICK !!!\n");

				if(!selectionbuff.empty())
					selectionbuff.clear();

				if(fltk::event_button() == 3)
					pmenu->popup();

				// track position so moving can be deduced
				if(fltk::event_button() == 1) {
					selbox->x = fltk::event_x();
					selbox->y = fltk::event_y();
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
				fltk::event_is_click(0);
				select(tmp_icon);
				return 1;
			} else if (SELECTION_SINGLE) {

				if(!in_selection(tmp_icon))
					select_only(tmp_icon);

			} else if (fltk::event_button() == 3)
				select_only(tmp_icon);

			/* 
			 * Let child handle the rest.
			 * Also prevent click on other mouse buttons during move.
			 */
			if(!moving)
				tmp_icon->handle(fltk::PUSH);

			selection_x = fltk::event_x_root();
			selection_y = fltk::event_y_root();
			EDEBUG(ESTRLOC ": fltk::PUSH from desktop\n");
	
			return 1;
		 }

		case fltk::DRAG:
			moving = true;

			if(!selectionbuff.empty()) {
				EDEBUG(ESTRLOC ": DRAG icon from desktop\n");
				move_selection(fltk::event_x_root(), fltk::event_y_root(), false);
			} else {
				EDEBUG(ESTRLOC ": DRAG from desktop\n");

				/*
				 * Moving is started with pushed button.
				 * From this point selection box is created and is rolled until release
				 */
				if(selbox->x != 0 || selbox->y != 0) {
					selbox->w = fltk::event_x() - selbox->x;
					selbox->h = fltk::event_y() - selbox->y;

					selbox->show = true;

					// see if there some icons inside selection area
					select_in_area();

					// redraw selection box
					redraw(EDAMAGE_OVERLAY);
				}
			}
			return 1;

		case fltk::RELEASE:
			EDEBUG(ESTRLOC ": RELEASE from desktop\n");
			EDEBUG(ESTRLOC ": clicks: %i\n", fltk::event_is_click());

			if(selbox->show) {
				selbox->x = selbox->y = selbox->w = selbox->h = 0;
				selbox->show = false;
				redraw(EDAMAGE_OVERLAY);
				/*
				 * Now pickup those who are in is_focused() state.
				 * Here is not used select() since it will fill selectionbuff with
				 * redrawing whole window each time. This is not what we want.
				 *
				 * Possible flickers due overlay will be later removed when is
				 * called move_selection(), which will in turn redraw icons again
				 * after position them.
				 */
				if(!selectionbuff.empty())
					selectionbuff.clear();

				for(unsigned int i = 0; i < icons.size(); i++) {
					if(icons[i]->is_focused())
						select_noredraw(icons[i]);
				}
				return 1;
			}

			if(!selectionbuff.empty() && moving)
				move_selection(fltk::event_x_root(), fltk::event_y_root(), true);

			/* 
			 * Do not send fltk::RELEASE during move
			 *
			 * TODO: should be alowed fltk::RELEASE to multiple icons? (aka. run 
			 * command for all selected icons ?
			 */
			if(selectionbuff.size() == 1 && !moving)
				selectionbuff[0]->handle(fltk::RELEASE);

			moving = false;
			return 1;

		case fltk::DND_ENTER:
		case fltk::DND_DRAG:
		case fltk::DND_LEAVE:
			return 1;

		case fltk::DND_RELEASE: {
			// fltk::belowmouse() can't be used within DND context :)
			DesktopIcon* di = below_mouse(fltk::event_x_root(), fltk::event_y_root());
			if(di) {
				di->handle(event);
			} else {
				EDEBUG("DND on DESKTOP\n");
			}
			return 1;
		}
		case fltk::PASTE: {
			DesktopIcon* di = below_mouse(fltk::event_x_root(), fltk::event_y_root());
			if(di) {
				di->handle(event);
			} else {
				EDEBUG("PASTE on desktop with %s\n", fltk::event_text());
				drop_source(fltk::event_text(), fltk::event_x_root(), fltk::event_y_root());
			}
	  	}
			return 1;

		default:
			break;
	}

	return fltk::Window::handle(event);
}

/*
 * This function is executed before desktop is actually showed
 * but after is internally created so net_make_me_desktop() specific code can
 * be executed correctly. 
 *
 * Calling net_make_me_desktop() after show() will confuse many wm's which will 
 * in turn partialy register us as desktop type.
 */
void Desktop::create(void) {
	fltk::Window::create();
	net_make_me_desktop(this);
}

int main() {
	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT, exit_signal);

	srand(time(NULL));

	// TODO: init_locale_support()

	//edelib::IconTheme::init("crystalsvg");
	edelib::IconTheme::init("edeneu");

	fltk::register_images();

	Desktop::init();
	Desktop::instance()->show();

	/*
	 * XSelectInput will redirect PropertyNotify messages, which
	 * we are listen for
	 */
	XSelectInput(fltk::xdisplay, RootWindow(fltk::xdisplay, fltk::xscreen), PropertyChangeMask | StructureNotifyMask );
	fltk::add_event_handler(desktop_xmessage_handler);

	while(running) 
		fltk::wait();

	Desktop::shutdown();
	edelib::IconTheme::shutdown();

	return 0;
}
