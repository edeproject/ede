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

#ifndef __NOTIFYBOX_H__
#define __NOTIFYBOX_H__

#include <FL/Fl_Box.h>

class NotifyBox : public Fl_Box {
	private:
		bool is_shown;
		int state;
		int lwidth, lheight;
		int area_w, area_h;
		void update_label_size(void);

	public:
		NotifyBox(int aw, int ah);
		~NotifyBox();

		virtual void show(void);
		virtual void hide(void);
		bool shown(void) { return is_shown; }

		const char* label(void);
		void label(const char* l);
		void copy_label(const char* l);

		static void animate_cb(void* b) { ((NotifyBox*)b)->animate(); }
		void animate(void);

		static void visible_timeout_cb(void* b) { ((NotifyBox*)b)->visible_timeout(); }
		void visible_timeout(void);
};

#endif
