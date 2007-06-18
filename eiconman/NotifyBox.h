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

#define TIMEOUT     (1.0f/60.0f)
#define TIME_SHOWN  3.0f

class NotifyBox : public Fl_Box {
	private:
		bool is_shown;
		bool in_animate;
		int lwidth, lheight;
		void update_label_size(void);

	public:
		NotifyBox();
		~NotifyBox();

		virtual void show(void);
		virtual void hide(void);
		bool shown(void) { return is_shown; }

		static void animate_show_cb(void* b) { ((NotifyBox*)b)->animate_show(); }
		void animate_show(void);

		static void animate_hide_cb(void* b) { ((NotifyBox*)b)->animate_hide(); }
		void animate_hide(void);

		static void visible_timeout_cb(void* b) { ((NotifyBox*)b)->visible_timeout(); }
		void visible_timeout(void);
};

#endif
