/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <X11/Xlib.h> // XEvent

/* Purpose of this class is to
 * split and minimize already big Frame class
 */

class Frame;

class FrameEventHandler
{
	private:
		Frame* curr_frame;

		int map_event(const XMapRequestEvent& e);
		int unmap_event(const XUnmapEvent& e);
		int reparent_event(const XReparentEvent& e);
		int destroy_event(const XDestroyWindowEvent& e);
		int client_message(const XClientMessageEvent& e);
		int property_event(const XPropertyEvent& e);
		int enter_event(const XCrossingEvent& e);
		int configure_event(const XConfigureRequestEvent& e);
		int enter_leave_event(const XCrossingEvent& e);
	public:
		FrameEventHandler(Frame* f);
		~FrameEventHandler();
		int handle_fltk(int e);
		int handle_x(XEvent* e);
};

#endif
