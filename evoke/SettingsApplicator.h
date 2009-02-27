/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __SETTINGSAPPLICATOR_H__
#define __SETTINGSAPPLICATOR_H__

#include <X11/Xlib.h>

/* 
 * Settings applicator are bunch of functions to run XSettingsClient
 * and apply known settings.
 */
void xsettings_applicator_init(Display* dpy, int scr);
void xsettings_applicator_shutdown(void);
void xsettings_applicator_process_event(const XEvent* xev);

#endif
