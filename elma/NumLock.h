/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __NUMLOCK_H__
#define __NUMLOCK_H__

#include <X11/Xlib.h>

bool numlock_xkb_init(Display* dpy);
void numlock_on(Display* dpy);
void numlock_off(Display* dpy);

#endif
