/*
 * $Id: Hints.h 1736 2006-08-19 00:38:53Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __HINTS_H__
#define __HINTS_H__

#include <X11/Xlib.h>

struct FrameData;
class WindowManager;

struct Hints
{
	void  icccm_size(FrameData* f);
	void  icccm_wm_hints(FrameData* f);
	char* icccm_label(Window win, bool* allocated);
	void  icccm_set_iconsizes(WindowManager* wm);
	void  icccm_configure(FrameData* f) const;

	char* netwm_label(Window win, bool* allocated);
	void  netwm_window_type(FrameData* fd);
	void  netwm_set_window_type(FrameData* fd);
	long  netwm_window_state(FrameData* fd) const;
	void  netwm_set_window_state(FrameData* fd);
	void  netwm_set_active_window(Window win);
	void  netwm_strut(Window win, int* x, int* y, int* w, int* h) const;

	void  mwm_load_hints(FrameData* fd);
};
	
#endif
