/*
 * $Id: Hints.cpp 1736 2006-08-19 00:38:53Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Hints.h"
#include "Tracers.h"
#include "Frame.h"
#include "Atoms.h"
#include "Windowmanager.h"

#include <efltk/x.h>
#include <X11/Xproto.h>     // CARD32
#include <assert.h>

#define MwmHintsDecorations (1 << 1)

#define MwmDecorAll         (1 << 0)
#define MwmDecorBorder      (1 << 1)
#define MwmDecorHandle      (1 << 2)
#define MwmDecorTitle       (1 << 3)
#define MwmDecorMenu        (1 << 4)
#define MwmDecorMinimize    (1 << 5)
#define MwmDecorMaximize    (1 << 6)

#define PropMotifHintsElements 3

struct MwmHints 
{
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
};

void Hints::icccm_size(FrameData* f)
{
	TRACE_FUNCTION("void Hints::icccm_size(FrameData* f)");

	assert(f != 0);
	long supplied;
	XSizeHints* sh = XAllocSizeHints();

	if(!XGetWMNormalHints(fl_display, f->window, sh, &supplied))
		sh->flags = 0;

	if(sh->flags & PResizeInc)
	{
		if(sh->width_inc < 1)
			sh->width_inc = 1;
		if(sh->height_inc < 1)
			sh->height_inc = 1;
	}

	/* Check if PBaseSize flag is set. If not
	 * fill it with minimal values and use them below.
	 */
	if(!(sh->flags & PBaseSize))
		sh->base_width = sh->base_height = 0;
	
	if(!(sh->flags & PMinSize))
	{
		sh->min_width = sh->base_width;
		sh->min_height= sh->base_height;
	}

	if(!(sh->flags & PMaxSize))
	{
		sh->max_width = 32767;
		sh->max_height = 32767;
	}

	if(sh->max_width < sh->min_width || sh->max_width <= 0)
        sh->max_width = 32767;
	if(sh->max_height < sh->min_height || sh->max_height <= 0)
		sh->max_height = 32767;

	if(!(sh->flags & PWinGravity))
	{
		sh->win_gravity = NorthWestGravity;
		sh->flags |= PWinGravity;
	}

	f->plain.max_w = sh->max_width;
	f->plain.max_h = sh->max_height;
	//f->plain.w     = sh->base_width;
	//f->plain.h     = sh->base_height;
	f->plain.min_w = sh->min_width;
	f->plain.min_h = sh->min_height;
	f->win_gravity = sh->win_gravity;
		
	// TODO: maybe calculate aspect_min and aspect_max ?
    if(sh->flags & PAspect) 
		f->option |= FrameOptKeepAspect;

	f->autoplace = (!(sh->flags & (USPosition|PPosition)));

	XFree(sh);
}

void Hints::icccm_wm_hints(FrameData* f)
{
	TRACE_FUNCTION("void Hints::icccm_wm_hints(FrameData* f)");
	assert(f != NULL);

	XWMHints* wm_hints = XAllocWMHints();
	wm_hints = XGetWMHints(fl_display, f->window);
	if(!wm_hints)
	{
		ELOG("XGetWMHints failed!");
		return;
	}

	if((wm_hints->flags & IconPixmapHint) && wm_hints->icon_pixmap)
		f->icon_pixmap = wm_hints->icon_pixmap;
	if((wm_hints->flags & IconMaskHint) &&wm_hints->icon_mask)
		f->icon_mask   = wm_hints->icon_mask;

	switch(wm_hints->initial_state)
	{
		case WithdrawnState:
			XRemoveFromSaveSet(fl_display, f->window);
			break;
		case IconicState:
			f->state = FrameStateIconized;
			break;
		case NormalState:
		default:
			f->state = FrameStateNormal;
			break;
	}

	// check for focus
	if((wm_hints->flags & InputHint) && !wm_hints->input)
			f->option &= ~FrameOptTakeFocus; // window does not want focus
	else
			f->option |= FrameOptTakeFocus;  // window want focus;

	XFree(wm_hints);
}

char* Hints::icccm_label(Window win, bool* allocated)
{
	TRACE_FUNCTION("char* Hints::icccm_label(Window win, bool* allocated)");

	XTextProperty xtp;
	char* title = 0;
	if(XGetWMName(fl_display, win, &xtp))
    {
		if(xtp.encoding == XA_STRING)
		{
			title = strdup((const char*)xtp.value);
			*allocated = true;
		}
		else
		{
			ELOG("X11 UTF8 text property not supported, at least for now...");
			*allocated = false;
		}

       	XFree(xtp.value);
	}

	return title;
}

void Hints::icccm_set_iconsizes(WindowManager* wm)
{
	TRACE_FUNCTION("void Hints::icccm_set_iconsizes(WindowManager* wm)");
	assert(wm != 0);

	XIconSize* is = XAllocIconSize();
	if(!is)
	{
		ELOG("XAllocIconSize failed!");
		return;
	}

	is->min_width = is->min_height = 8;
	is->max_width = is->max_height = 48;
	is->width_inc = is->height_inc = 1;
	ELOG("setting icon sizes");
	XSetIconSizes(fl_display, WindowManager::instance()->root_window(), is, 1);
	XFree(is);
}

void Hints::icccm_configure(FrameData* f) const
{
	TRACE_FUNCTION("void Hints::icccm_configure(FrameData* f) const");

	assert(f != 0);

	XConfigureEvent ce;
	ce.send_event = True;
	ce.display = fl_display;
	ce.type = ConfigureNotify;
	ce.event = f->window;
	ce.window = f->window;
	ce.x = f->plain.x;
	ce.y = f->plain.y;
	ce.width = f->plain.w;
	ce.height = f->plain.h;
	ce.border_width = f->plain.border;
	ce.above = None;
	ce.override_redirect = False;
    XSendEvent(fl_display, f->window, False, StructureNotifyMask, (XEvent*)&ce);
}

char* Hints::netwm_label(Window win, bool* allocated)
{
	TRACE_FUNCTION("char* Hints::netwm_label(Window win, bool* allocated)");

	unsigned char* title = 0;
	Atom real_type;
	int  real_format;
	unsigned long items_read, items_left;

	int status = XGetWindowProperty(fl_display, win, _XA_NET_WM_NAME,
			0L, 0x7fffffff, False, _XA_UTF8_STRING, &real_type, &real_format, &items_read, &items_left,
			(unsigned char**)&title);

	if(status != Success && items_read != 1)
	{
		*allocated = false;
		if(title)
			XFree(title);

		return 0;
	}

	*allocated = true;
	return (char*)title;
}

void Hints::netwm_window_type(FrameData* fd)
{
	TRACE_FUNCTION("short Hints::netwm_window_type(FrameData* fd) const");

	Atom *data;
	Atom real_type;
	int  real_format;
	unsigned long items_read, items_left;
	short type = FrameTypeNormal;

	int status = XGetWindowProperty(fl_display, fd->window, _XA_NET_WM_WINDOW_TYPE,
			0L, 8L, false, XA_ATOM, &real_type, &real_format, &items_read, &items_left,
			(unsigned char**)&data);

	if(status != Success || !items_read)
	{
		ELOG("Netwm say: unknown window type, using FrameTypeNormal");
		type = FrameTypeNormal;
	}

	for(unsigned int i = 0; i < items_read; i++)
	{
	
		if(data[i] == _XA_NET_WM_WINDOW_TYPE_DOCK)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_DOCK");
			type = FrameTypeDock;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_TOOLBAR)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_TOOLBAR");
			type = FrameTypeToolbar;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_MENU)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_MENU");
			type = FrameTypeMenu;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_UTIL)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_UTIL");
			type = FrameTypeUtil;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_DIALOG)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_DIALOG");
			type = FrameTypeDialog;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_SPLASH)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_SPLASH");
			type = FrameTypeSplash;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_DESKTOP)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_DESKTOP");
			type = FrameTypeDesktop;
			break;
		}

		if(data[i] == _XA_NET_WM_WINDOW_TYPE_NORMAL)
		{
			ELOG("_XA_NET_WM_WINDOW_TYPE_NORMAL");
			type = FrameTypeNormal;
			break;
		}
	}

	XFree(data);
	fd->type = type;
}

void Hints::netwm_set_window_type(FrameData* fd)
{
	TRACE_FUNCTION("void Hints::netwm_set_window_type(FrameData* fd)");
	assert(fd != 0);

	Atom to_set[1];
	switch(fd->type)
	{
		case FrameTypeNormal:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_NORMAL;
			break;
		case FrameTypeDesktop:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_DESKTOP;
			break;
		case FrameTypeSplash:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_SPLASH;
			break;
		case FrameTypeUtil:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_UTIL;
			break;
		case FrameTypeMenu:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_MENU;
			break;
		case FrameTypeToolbar:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
			break;
		case FrameTypeDialog:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_DIALOG;
			break;
		case FrameTypeDock:
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_DOCK;
			break;
		default:
			EFATAL("Type unknown");
			to_set[0] = _XA_NET_WM_WINDOW_TYPE_NORMAL;
			break;
	}

	XChangeProperty(fl_display, fd->window, _XA_NET_WM_WINDOW_TYPE, XA_ATOM, 32, 
			PropModeReplace, (unsigned char*)to_set, 1);
}

long Hints::netwm_window_state(FrameData* fd) const
{
	TRACE_FUNCTION("long Hints::netwm_window_state(FrameData* fd) const");
	assert(fd != 0);

	Atom *data;
	Atom real_type;
	int  real_format;
	unsigned long items_read, items_left;

	int status = XGetWindowProperty(fl_display, fd->window, _XA_NET_WM_STATE,
			0L, 12L, false, XA_ATOM, &real_type, &real_format, &items_read, &items_left,
			(unsigned char**)&data);

	if(status != Success || !items_read)
		return FrameTypeNormal;

	for(unsigned int i = 0; i < items_read; i++)
	{
		if(data[i] == _XA_NET_WM_STATE_MODAL)
			ELOG("_XA_NET_WM_STATE_MODAL");
		else if(data[i] == _XA_NET_WM_STATE_STICKY)
			ELOG("_XA_NET_WM_STATE_STICKY");
		else if(data[i] == _XA_NET_WM_STATE_MAXIMIZED_VERT)
			ELOG("_XA_NET_WM_STATE_MAXIMIZED_VERT");
		else if(data[i] == _XA_NET_WM_STATE_MAXIMIZED_HORZ)
			ELOG("_XA_NET_WM_STATE_MAXIMIZED_HORZ");
		else if(data[i] == _XA_NET_WM_STATE_SHADED)
			ELOG("_XA_NET_WM_STATE_SHADED");
		else if(data[i] == _XA_NET_WM_STATE_SKIP_TASKBAR)
			ELOG("_XA_NET_WM_STATE_SKIP_TASKBAR");
		else if(data[i] == _XA_NET_WM_STATE_SKIP_PAGER)
			ELOG("_XA_NET_WM_STATE_SKIP_PAGER");
		else if(data[i] == _XA_NET_WM_STATE_HIDDEN)
			ELOG("_XA_NET_WM_STATE_HIDDEN");
		else if(data[i] == _XA_NET_WM_STATE_FULLSCREEN)
			ELOG("_XA_NET_WM_STATE_FULLSCREEN");
		else if(data[i] == _XA_NET_WM_STATE_ABOVE)
			ELOG("_XA_NET_WM_STATE_ABOVE");
		else if(data[i] == _XA_NET_WM_STATE_BELOW)
			ELOG("_XA_NET_WM_STATE_BELOW");
		else if(data[i] == _XA_NET_WM_STATE_DEMANDS_ATTENTION)
			ELOG("_XA_NET_WM_STATE_DEMANDS_ATTENTION");
	}

	XFree(data);
	return FrameStateNormal;
}

void Hints::netwm_set_window_state(FrameData* fd)
{
	TRACE_FUNCTION("void Hints::set_window_state(FrameData* fd)");

	assert(fd != 0);
	Atom data[10];
	int i = 0;

	if(fd->state & FrameStateSticky)
		data[i++] = _XA_NET_WM_STATE_STICKY;
	if(fd->state & FrameStateShaded)
		data[i++] = _XA_NET_WM_STATE_SHADED;
	if(fd->state & FrameStateAlwaysAbove)
		data[i++] = _XA_NET_WM_STATE_ABOVE;
	if(fd->state & FrameStateAlwaysBelow)
		data[i++] = _XA_NET_WM_STATE_BELOW;
	if((fd->state & FrameStateMaximizedHorz) || (fd->state & FrameStateMaximized))
		data[i++] = _XA_NET_WM_STATE_MAXIMIZED_HORZ;
	if((fd->state & FrameStateMaximizedVert) || (fd->state & FrameStateMaximized))
		data[i++] = _XA_NET_WM_STATE_MAXIMIZED_VERT;

	XChangeProperty(fl_display, fd->window, _XA_NET_WM_STATE, XA_ATOM, 32, 
			PropModeReplace, (unsigned char*)data, i);
}

void Hints::netwm_set_active_window(Window win)
{
	TRACE_FUNCTION("void Hints::netwm_set_active_window(Window win)");

    XChangeProperty(fl_display, WindowManager::instance()->root_window(), _XA_NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&win, 1);
}

void Hints::netwm_strut(Window win, int* x, int* y, int* w, int* h) const
{
	TRACE_FUNCTION("void Hints::netwm_strut(Window win, int* x, int* y, int* w, int* h) const");

	CARD32 *data;
	Atom real_type;
	int  real_format;
	unsigned long items_read, items_left;

	int status = XGetWindowProperty(fl_display, win, _XA_NET_WM_STRUT,
			0L, 0x7fffffff, False, XA_CARDINAL, &real_type, &real_format, &items_read, &items_left,
			(unsigned char**)&data);

	ELOG("STRUT: items_read %i", items_read);
	if(status != Success)
		return;
	
	if((items_read / sizeof(CARD32)) != 4)
	{
		ELOG("STRUT: window have wrong strut %i", items_read/sizeof(CARD32));
		return;
	}

	ELOG("STRUT: %i %i %i %i", data[0], data[1], data[2], data[3]);
	XFree((char*)data);
}

void Hints::mwm_load_hints(FrameData* fd)
{
	TRACE_FUNCTION("void Hints::mwm_load_hints(FrameData* fd)");
	assert(fd != 0);

	Atom real_type;
	int  real_format;
	unsigned long items_read, items_left;
	MwmHints* mwm;

	int status = XGetWindowProperty(fl_display, fd->window, _XA_MOTIF_HINTS,
			0L, 20L, false, _XA_MOTIF_HINTS, &real_type, &real_format, &items_read, &items_left,
			(unsigned char**)&mwm);

	if(status == Success && items_read /*>= PropMotifHintsElements*/)
	{
		ELOG("MWM: got hints !!!");
		if((mwm->flags & MwmHintsDecorations))
		{
			if(mwm->decorations & MwmDecorAll)
				ELOG("MwmDecorAll");
			else
			{
				if(mwm->decorations & MwmDecorTitle)
					ELOG("MwmHintsDecorTitle");
				if(mwm->decorations & MwmDecorBorder)
					ELOG("MwmHintsDecorBorder");
				if(mwm->decorations & MwmDecorHandle)
					ELOG("MwmHintsDecorHandle");
			}
		}
		else
			ELOG("mwm: no decor at all");
	}

	XFree(mwm);
}
