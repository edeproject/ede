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

#ifndef __DESKTOP_ICON_H__
#define __DESKTOP_ICON_H__

#include <FL/Fl_Widget.H>
#include <FL/Fl_Image.H>
#include <edelib/MenuButton.h>
#include <edelib/String.h>

#include "Globals.h"

enum {
	DESKTOP_ICON_TYPE_NORMAL,
	DESKTOP_ICON_TYPE_TRASH,
	DESKTOP_ICON_TYPE_LINK,
	DESKTOP_ICON_TYPE_FILE,
	DESKTOP_ICON_TYPE_FOLDER
};

/* default icon sizes */
#define DESKTOP_ICON_SIZE_W 48
#define DESKTOP_ICON_SIZE_H 48

/*
 * Each DesktopIcon shares IconOptions, which is content of [Icons] section
 * from configuration file. With this, 'Desktop' needs only to update content so all
 * icons see it.
 */
struct IconOptions {
	int  label_background;
	int  label_foreground;
	int  label_font;
	int  label_fontsize;
	int  label_maxwidth;
	int  label_transparent;
	int  label_visible;
	bool one_click_exec;

	IconOptions() {
		label_background  = FL_BLACK;
		label_foreground  = FL_WHITE;
		label_maxwidth    = 75;
		label_transparent = 1;
		label_visible     = 1;
		label_font        = FL_HELVETICA;
		label_fontsize    = 12;
		one_click_exec    = false;
	}
	
	/* should be called only when values are assigned to fonts */
	void sanitize_font(void) {
		if(label_font < 0)      label_font = FL_HELVETICA;
		if(label_fontsize < 8)  label_fontsize = 12;
		if(label_maxwidth < 30) label_maxwidth = 75;
	}
};

class MovableIcon;

class DesktopIcon : public Fl_Widget {
private:
	int  icon_type;
	int  lwidth, lheight;
	bool focused;

	Fl_Image    *darker_img;
	IconOptions *opts;
	MovableIcon *micon;

	/* location of .desktop file and command to be executed */
	EDELIB_NS_PREPEND(String) path, cmd;
	EDELIB_NS_PREPEND(MenuButton) *imenu;

#if !((FL_MAJOR_VERSION >= 1) && (FL_MINOR_VERSION >= 3))
	EDELIB_NS_PREPEND(String) ttip;
#endif

public:
	DesktopIcon(const char *l, int W = DESKTOP_ICON_SIZE_W, int H = DESKTOP_ICON_SIZE_H);
	~DesktopIcon();

	void set_icon_type(int c) { icon_type = c; }
	int  get_icon_type(void) { return icon_type;}
	void set_image(const char *name);
	void set_tooltip(const char *tip);

	void update_label_font_and_size(void);

	void set_options(IconOptions *o) {
		opts = o;
		update_label_font_and_size();
	}

	/* Here is implemented localy focus schema avoiding messy fltk one. Focus/unfocus is handled from Desktop. */
	void do_focus(void)   { focused = true;  }
	void do_unfocus(void) { focused = false; }
	bool is_focused(void) { return focused;  }
	
	void        set_path(const char *p) { path = p; }
	const char *get_path(void) { return path.c_str(); }

	void        set_cmd(const char *c) { cmd = c; }
	const char *get_cmd(void) { return cmd.c_str(); }

	void fix_position(int X, int Y);
	void drag(int x, int y, bool apply);
	int  drag_icon_x(void);
	int  drag_icon_y(void);

	/*
	 * This is 'enhanced' (in some sense) redraw(). Redrawing icon will not fully redraw label nor
	 * focus box, which laid outside icon box. It will use damage() on given region, but called from
	 * parent, so parent can redraw that region on itself (since label does not laid on any box)
	 *
	 * Alternative way would be to redraw the whole parent, but it is pretty unneeded and slow.
	 */
	void fast_redraw(void);

	void draw(void);
	int handle(int event);
};

#endif
