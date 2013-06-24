/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2013 Sanel Zukan
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

#ifndef __EDE_DESKTOP_H__
#define __EDE_DESKTOP_H__

#include <edelib/MenuButton.h>
#include <edelib/DesktopFile.h>
#include <edelib/Config.h>
#include <edelib/List.h>
#include <edelib/String.h>

#ifdef USE_EDELIB_WINDOW
# include <edelib/Window.h>
# define EDE_DESKTOP_WINDOW EDELIB_NS_PREPEND(Window)
#else
# include <FL/Fl_Window.H>
# define EDE_DESKTOP_WINDOW Fl_Window
#endif

#include "Globals.h"

#define EDE_DESKTOP_APP "ede-desktop"

struct SelectionOverlay;
struct IconOptions;
class  DesktopIcon;
class  Wallpaper;

typedef EDELIB_NS_PREPEND(list)<DesktopIcon*> DesktopIconList;
typedef EDELIB_NS_PREPEND(list)<DesktopIcon*>::iterator DesktopIconListIt;

class Desktop : public EDE_DESKTOP_WINDOW {
private:
	SelectionOverlay *selbox;
	DesktopConfig    *conf;
	IconOptions      *icon_opts;

	EDELIB_NS_PREPEND(String) dpath;
	EDELIB_NS_PREPEND(MenuButton) *dmenu;
	Wallpaper                     *wallpaper;
	
	int selection_x, selection_y;
	/* last recorded pointer position, so icon can be created at position where menu is clicked */
	int last_px, last_py;
	bool moving;

	/* so we can track selected icons; one or multiselection */
	DesktopIconList  selectionbuf;
public:
	Desktop();
	~Desktop();

	const char *desktop_path(void);

	void show(void);
	void update_workarea(void);
	void read_config(void);

	void         read_desktop_folder(const char *dpath = NULL);
	DesktopIcon *read_desktop_file(const char *path, const char *base = 0, DesktopConfig *positions = 0);
	
	void arrange_icons(void);
	bool remove_icon(DesktopIcon *di, bool real_delete);
	bool rename_icon(DesktopIcon *di, const char *name);
	void edit_icon(DesktopIcon *di);
	bool save_icons_positions(void);
	bool create_folder(const char *name);

	void unfocus_all(void);
	void select(DesktopIcon *ic, bool do_redraw = true);
	void select_only(DesktopIcon *ic);
	bool in_selection(const DesktopIcon *ic);
	void move_selection(int x, int y, bool apply);

	DesktopIcon *below_mouse(int px, int py);
	void select_in_area(void);

	void draw(void);
	int  handle(int event);
};

#endif
