/*
 * $Id: Utils.cpp 1705 2006-07-23 22:31:56Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Utils.h"
#include <efltk/x.h>

void SendMessage(Window win, Atom a, Atom l)
{
	XEvent ev;
	long mask;
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = long(l);
	ev.xclient.data.l[1] = long(fl_event_time);
	mask = 0L;
	XSendEvent(fl_display, win, False, mask, &ev);
}
