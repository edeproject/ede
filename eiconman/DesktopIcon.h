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

#ifndef __DESKTOPICON_H__
#define __DESKTOPICON_H__

#include <fltk/Widget.h>
#include <fltk/Window.h>
#include <fltk/Image.h>
#include <fltk/PopupMenu.h>

#define ICON_NORMAL 1
#define ICON_TRASH  2

class GlobalIconSettings;
class IconSettings;
class MovableIcon;

class DesktopIcon : public fltk::Widget {
	private:
		IconSettings* settings;
		const GlobalIconSettings* globals;

		int  type;
		int  lwidth;
		int  lheight;
		bool focus;

		// pseudosizes (nor real icon sizes)
		int  image_w;
		int  image_h;

		fltk::PopupMenu* pmenu;
		MovableIcon* micon;

		void update_label_size(void);

	public:
		DesktopIcon(GlobalIconSettings* gisett, IconSettings* isett, int icon_type = NORMAL);
		~DesktopIcon();
		virtual void draw(void);
		virtual int  handle(int event);
		void drag(int x, int y, bool apply);
		int  drag_icon_x(void);
		int  drag_icon_y(void);

		/*
		 * Here is implemented localy focus schema avoiding
		 * messy fltk one. Focus/unfocus is handled from Desktop.
		 */
		void do_focus(void)   { focus = true;  }
		void do_unfocus(void) { focus = false; }
		bool is_focused(void) { return focus;  }

		fltk::Image* icon_image(void) { return (fltk::Image*)image(); }
};

class MovableIcon : public fltk::Window {
	private:
		DesktopIcon* icon;

	public:
		MovableIcon(DesktopIcon* i);
		~MovableIcon();
};

#endif
