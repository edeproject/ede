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

#ifndef __EICONMAN_H__
#define __EICONMAN_H__

#include <fltk/Window.h>
#include <fltk/PopupMenu.h>
#include <fltk/Image.h>
#include <edelib/String.h>
#include <edelib/Config.h>
#include <edelib/Vector.h>

struct DesktopSettings {
	int  color;                   // background color
	bool wp_use;                  // use wallpaper or not
	fltk::Image*   wp_image;      // wallpaper image (can be NULL)
	edelib::String wp_path;       // wallpaper path
};

struct GlobalIconSettings {
	int  label_background;
	int  label_foreground;
	int  label_fontsize;
	int  label_maxwidth;
	int  gridspacing;
	bool label_transparent;
	bool label_draw;
	bool one_click_exec;
	bool auto_arr;
	bool same_size;
};

// settings related to .desktop files
struct IconSettings {
	int x, y;

    bool cmd_is_url;             // interpret cmd as url, like system:/,trash:/,$HOME
	
	edelib::String name;
	edelib::String cmd;
	edelib::String icon;
	edelib::String icon2;        // for stateable icons, like trash (empty/full)
	edelib::String desktop_name; // name used as key when storing positions
};

// selection overlay
struct SelectionOverlay {
	int  x, y, w, h;
	bool show;
};

class DesktopIcon;

typedef edelib::vector<DesktopIcon*>     DesktopIconList;

class Desktop : public fltk::Window {
	private:
		static Desktop* pinstance;

		int  desktops_num;
		int  curr_desktop;
		int  bg_color;
		bool wp_use;

		bool moving;
		int selection_x;
		int selection_y;

		SelectionOverlay*  selbox;

		GlobalIconSettings gisett;
		IconSettings       isett;

		DesktopSettings*   dsett;

		DesktopIconList icons;
		DesktopIconList selectionbuff;

		fltk::PopupMenu* pmenu;

		void init_desktops(void);

		void load_icons(const char* path, edelib::Config& conf);
		bool read_desktop_file(const char* path, IconSettings& is, int& icon_type);

		void add_icon(DesktopIcon* ic);
		void unfocus_all(void);

		void select(DesktopIcon* ic);
		void select_only(DesktopIcon* ic);
		bool in_selection(const DesktopIcon* ic);

		void select_in_area(void);
		void select_noredraw(DesktopIcon* ic);

		void move_selection(int x, int y, bool apply);

		DesktopIcon* below_mouse(int x, int y);
		void drop_source(const char* src, int x, int y);

	public:
		static void init(void);
		static void shutdown(void);
		static Desktop* instance(void);

		Desktop();
		~Desktop();
		void update_workarea(void);
		void read_config(void);
		void save_config(void);

		void set_wallpaper(const char* path, bool do_redraw = true);
		void set_wallpaper(fltk::Image* im, bool do_redraw = true);
		void set_bg_color(unsigned int c, bool do_redraw = true);

		void create(void);
		virtual void draw(void);
		virtual int handle(int event);
};

#endif
