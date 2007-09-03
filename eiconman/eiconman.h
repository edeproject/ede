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

#include <FL/Fl_Window.h>
#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Image.h>

#include <edelib/String.h>
#include <edelib/Config.h>
#include <edelib/List.h>

#define EDAMAGE_CHILD_LABEL    0x10
#define EDAMAGE_OVERLAY        0x20

struct DesktopSettings {
	int  color;                   // background color
	bool wp_use;                  // use wallpaper or not
};

struct GlobalIconSettings {
	int  label_background;
	int  label_foreground;
	int  label_fontsize;
	int  label_maxwidth;
	bool label_transparent;
	bool label_draw;
	bool one_click_exec;
	bool auto_arrange;
	bool same_size;
};

#define ICON_NORMAL  1     // .desktop file
#define ICON_TRASH   2     // trash.desktop (specific since have two icons for empty/full)
#define ICON_FILE    3     // real file
#define ICON_SYMLINK 4     // symbolic link

/*
 * Settings representing related to icon on desktop. To complicate our lives
 * (and to, at least, simplify code) it can be: 
 *  - .desktop file content (normal or trash like)
 *  - real file copied/moved inside ~/Desktop directory
 *  - symlink in ~/Desktop directory pointing to the real file
 */
struct IconSettings {
	int  x, y;
    int  type;                // ICON_NORMAL, ICON_TRASH,...
    bool cmd_is_url;          // interpret cmd as url, like system:/,trash:/,$HOME
	
	edelib::String name;
	edelib::String cmd;
	edelib::String icon;
	edelib::String icon2;     // for stateable icons, like trash (empty/full)
	edelib::String key_name;  // name used as key when storing positions
	edelib::String full_path; // for tracking changes
};

// Selection overlay
struct SelectionOverlay {
	int x, y, w, h;
	bool show;
};

class Wallpaper;
class DesktopIcon;
class NotifyBox;

class Fl_Menu_Button;

typedef edelib::list<DesktopIcon*> DesktopIconList;
typedef edelib::list<DesktopIcon*>::iterator DesktopIconListIter;

#define DESKTOP_WINDOW Fl_Window

class Desktop : public DESKTOP_WINDOW {
	private:
		static Desktop* pinstance;

		int selection_x, selection_y;
		bool moving;
		bool do_dirwatch;

		SelectionOverlay*  selbox;

		GlobalIconSettings gisett;
		DesktopSettings*   dsett;

		Fl_Menu_Button* dmenu;
		Wallpaper*      wallpaper;
		NotifyBox*      notify;

		DesktopIconList icons;
		DesktopIconList selectionbuff;

		void init_internals(void);

		void load_icons(const char* path, edelib::Config* conf);
		bool read_desktop_file(const char* path, IconSettings& is);

		void add_icon(DesktopIcon* ic);
		bool add_icon_pathed(const char* path, edelib::Config* conf);
		DesktopIcon* find_icon_pathed(const char* path);
		bool remove_icon_pathed(const char* path);
		bool update_icon_pathed(const char* path);

		void unfocus_all(void);

		void select(DesktopIcon* ic, bool do_redraw = true);
		void select_only(DesktopIcon* ic);
		bool in_selection(const DesktopIcon* ic);
		void move_selection(int x, int y, bool apply);

		void select_in_area(void);

		void drop_source(const char* src, int src_len, int x, int y);

		DesktopIcon* below_mouse(int px, int py);

	public:
		Desktop();
		~Desktop();

		virtual void show(void);
		virtual void hide(void);
		virtual void draw(void);
		virtual int handle(int event);

		static void init(void);
		static void shutdown(void);
		static Desktop* instance(void);

		void read_config(void);
		void save_config(void);

		void update_workarea(void);
		void area(int& X, int& Y, int& W, int& H) { X = x(); Y = y(); W = w(); H = h(); }

		void set_bg_color(int c, bool do_redraw = true);

		void notify_box(const char* msg, bool copy = false);
		void notify_box_color(Fl_Color col);
		void notify_desktop_changed(void);

		void dir_watch(const char* dir, const char* changed, int flags);
		void dir_watch_on(void) { do_dirwatch = true; }
		void dir_watch_off(void) { do_dirwatch = false; }
};

#endif
