/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Based on xcompmgr (c) 2003 Keith Packard.
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __COMPOSITE_H__
#define __COMPOSITE_H__

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h> // XserverRegion already included in Xdamage.h
#include <edelib/List.h>

struct CWindow;
typedef edelib::list<CWindow*> CWindowList;
typedef edelib::list<CWindow*>::iterator CWindowListIter;

class Composite {
	private:
		bool manual_redirect;

		CWindowList window_list;
		void add_window(Window id, Window previous);
		void map_window(Window id, unsigned long sequence, bool fade);
		void unmap_window(Window id, bool fade);
		void finish_unmap_window(CWindow* win);
		CWindow* find_window(Window id);
		XserverRegion window_extents(CWindow* win);

		void configure_window(const XConfigureEvent* ce);
		void restack_window(CWindow* win, Window new_above);
		void property_notify(const XPropertyEvent* pe);
		void expose_event(const XExposeEvent* ee);
		void reparent_notify(const XReparentEvent* re);
		void circulate_window(const XCirculateEvent* ce);

		void damage_window(XDamageNotifyEvent* de);
		void repair_window(CWindow* win);
		void destroy_window(Window id, bool gone, bool fade);
		void finish_destroy_window(Window id, bool gone);

		void paint_root(void);

	public:
		Composite();
		~Composite();
		bool init(void);
		int handle_xevents(const XEvent* xev);

		void paint_all(XserverRegion region);
		void update_screen(void);
};

#endif
