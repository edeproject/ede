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

#include <FL/Fl_Widget.h>
#include <FL/Fl_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Image.h>

class GlobalIconSettings;
class IconSettings;
class MovableIcon;

class DesktopIcon : public Fl_Button {
	private:
		IconSettings* settings;
		const GlobalIconSettings* globals;

		int  lwidth;
		int  lheight;
		bool focus;

		MovableIcon* micon;

		void update_label_size(void);

	public:
		DesktopIcon(GlobalIconSettings* gisett, IconSettings* isett, int bg);
		~DesktopIcon();

		virtual void draw(void);
		virtual int  handle(int event);

		void drag(int x, int y, bool apply);
		int  drag_icon_x(void);
		int  drag_icon_y(void);

		/*
		 * This is 'enhanced' (in some sense) redraw(). Redrawing
		 * icon will not fully redraw label nor focus box, which laid outside 
		 * icon box. It will use damage() on given region, but called from
		 * parent, so parent can redraw that region on itself (since label does
		 * not laid on any box)
		 *
		 * Alternative way would be to redraw whole parent, but it is pretty unneeded
		 * and slow.
		 */
		void fast_redraw(void);

		/*
		 * Here is implemented localy focus schema avoiding
		 * messy fltk one. Focus/unfocus is handled from Desktop.
		 */
		void do_focus(void)   { focus = true;  }
		void do_unfocus(void) { focus = false; }
		bool is_focused(void) { return focus;  }

		Fl_Image* icon_image(void) { return image(); }

		const IconSettings* get_settings(void) const { return settings; }
};

class MovableIcon : public Fl_Window {
	private:
		DesktopIcon* icon;
		Fl_Box* icon_box;

	public:
		MovableIcon(DesktopIcon* i);
		~MovableIcon();
		virtual void show(void);
};

#endif
