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

#include "Events.h"
#include "Frame.h"
#include "Titlebar.h"
#include "Windowmanager.h"
#include "Tracers.h"
#include "Atoms.h"
#include "Hints.h"
#include "Cursor.h"
#include "debug.h"

#include <assert.h>


#define FRAME_NAME(frame) (frame->label ? frame->label : "NULL")

FrameEventHandler::FrameEventHandler(Frame* f) : curr_frame(f)
{
	assert(curr_frame != NULL);
}

FrameEventHandler::~FrameEventHandler()
{
}

/* Handle efltk events.
 * Some events like FL_FOCUS and FL_UNFOCUS are not handled, since 
 * efltk does not receive this event when when should (for example:
 * one window is focused, but other shoud be automatically unfocused).
 * X on ther hand send FocusIn and FocusOut when above events ocurr.
 */
int FrameEventHandler::handle_fltk(int event)
{
	long gggg;
	static long bbbb;
	static CursorType ctype;

	if(curr_frame->show_titlebar)
	{
		Titlebar* tbar = curr_frame->titlebar;
		assert(tbar != NULL);

		if(!curr_frame->resizing())
		{
			if(Fl::event_inside(tbar->x(), tbar->y(), tbar->w(), tbar->h()) || curr_frame->moving())
				return tbar->handle(event);
		}
	}

	switch(event)
	{
		case FL_SHOW:
		case FL_HIDE:
			return 0;

		case FL_ENTER:
			ELOG("FrameEventHandler: FL_ENTER");
			return 1;

		case FL_LEAVE:
			ELOG("FrameEventHandler: FL_LEAVE");
			return 1;

		case FL_MOVE:
		{
			// note, I am using coordinates _inside_ window
			gggg = curr_frame->resize_type(Fl::event_x(), Fl::event_y());
			switch(gggg)
			{
				case ResizeTypeUpLeft:
					ELOG("FrameEventHandler (resizing): ResizeTypeUpLeft");
					curr_frame->set_cursor(CURSOR_NW);
					ctype = CURSOR_NW;
					bbbb = ResizeTypeUpLeft;
					return 1;
				case ResizeTypeUpRight:
					ELOG("FrameEventHandler (resizing): ResizeTypeUpRight");
					curr_frame->set_cursor(CURSOR_NE);
					ctype = CURSOR_NE;
					bbbb = ResizeTypeUpRight;
					return 1;
				case ResizeTypeDownLeft:
					ELOG("FrameEventHandler (resizing): ResizeTypeDownLeft");
					curr_frame->set_cursor(CURSOR_SW);
					ctype = CURSOR_SW;
					bbbb = ResizeTypeDownLeft;
					return 1;
				case ResizeTypeDownRight:
					ELOG("FrameEventHandler (resizing): ResizeTypeDownRight");
					curr_frame->set_cursor(CURSOR_SE);
					ctype = CURSOR_SE;
					bbbb = ResizeTypeDownRight;
					return 1;
				case ResizeTypeUp:
					ELOG("FrameEventHandler (resizing): ResizeTypeUp");
					curr_frame->set_cursor(CURSOR_N);
					ctype = CURSOR_N;
					bbbb = ResizeTypeUp;
					return 1;
				case ResizeTypeLeft:
					ELOG("FrameEventHandler (resizing): ResizeTypeLeft");
					curr_frame->set_cursor(CURSOR_W);
					ctype = CURSOR_W;
					bbbb = ResizeTypeLeft;
					return 1;
				case ResizeTypeRight:
					ELOG("FrameEventHandler (resizing): ResizeTypeRight");
					curr_frame->set_cursor(CURSOR_E);
					ctype = CURSOR_E;
					bbbb = ResizeTypeRight;
					return 1;
				case ResizeTypeDown:
					ELOG("FrameEventHandler (resizing): ResizeTypeDown");
					curr_frame->set_cursor(CURSOR_S);
					ctype = CURSOR_S;
					bbbb = ResizeTypeDown;
					return 1;
				default:
					//curr_frame->set_cursor(CURSOR_DEFAULT);
					bbbb = ResizeTypeNone;
					return 1;
			}
			return 1;
		}

		case FL_PUSH:
			ELOG("FrameEventHandler: FL_PUSH");

			if(Fl::event_is_click())
			{
				curr_frame->borders_color(CLICKED);
				curr_frame->raise();
			}
			return 1;

		case FL_DRAG:
			ELOG("FrameEventHandler: FL_DRAG");
			if(bbbb != ResizeTypeNone)
			{
				if(!curr_frame->resizing())
				{
					curr_frame->grab_cursor();
					curr_frame->resize_start();
					curr_frame->show_coordinates_window();
				}

				//XGrabServer(fl_display);

				curr_frame->set_cursor(ctype);
				curr_frame->resize_window(Fl::event_x_root(), Fl::event_y_root(), bbbb);
				curr_frame->update_coordinates_window();
			}
			return 1;

		case FL_RELEASE:
			ELOG("FrameEventHandler: FL_RELEASE");
			if(curr_frame->resizing())
			{
				//XUngrabServer(fl_display);
				curr_frame->resize_end();
				curr_frame->ungrab_cursor();
				curr_frame->hide_coordinates_window();
				curr_frame->set_cursor(CURSOR_DEFAULT);
			}

			//curr_frame->color(FL_GRAY);
			curr_frame->borders_color(FOCUSED);
			return 1;
		default:
			// let other events go to sizers
			return curr_frame->handle_parent(event);
	}
	return 0;

}

int FrameEventHandler::handle_x(XEvent* event)
{
	switch(event->type)
	{
		case MapRequest:
			return map_event(event->xmaprequest);
		case UnmapNotify:
			return unmap_event(event->xunmap);
		case ReparentNotify:
			return reparent_event(event->xreparent);
		case DestroyNotify:
			return destroy_event(event->xdestroywindow);
		/*case ClientMessage:
			return client_message(event->xclient);*/
		case LeaveNotify:
		case EnterNotify:
			return enter_leave_event(event->xcrossing);

		/* FocusIn, as FocusOut is not used, but for
		 * debugging is here. When we map a large number
		 * of windows, X seems goes into (for me unknown)
		 * infinite FocusIn/FocusOut loop, which blocks
		 * everything. So, to keep us far away from pain,
		 * leave FocusIn for someone else. Here should not
		 * be used.
		 */
		case FocusIn:
		{
			ELOG("FrameEventHandler: FocusIn");
			switch(event->xfocus.mode)
			{
				case NotifyNormal:
					ELOG(" - NotifyNormal");
					break;
				case NotifyGrab:
					ELOG(" - NotifyGrab");
					break;
				case NotifyUngrab:
					ELOG(" - NotifyUngrab");
					break;
			}

			if(event->xfocus.send_event)
				ELOG(" - got from SendEvent");

			/* This will prevent menus, etc.
			 * to steal focus from parent.
			 */
			//if(event->xfocus.mode == NotifyNormal)
			//	curr_frame->focus();
		}
			return 1;

		/* FocusOut really fuck up focusing so
		 * it is not handled. Either xfocus.mode == NotifyNormal
		 * does not helps when window emits menu, since menu
		 * itself will yield FocusOut for parent and FocusIn
		 * for itself. Unfortunately I don't know any toolkit
		 * which export some kind MENU flag, where we can check.
		 * So, let we shut it up, make ourself little bit happy
		 * and take all burden of FocusOut to ourself (which made
		 * use very very unhappy).
		 */
		case FocusOut:
		{
			ELOG("FrameEventHandler: FocusOut");
			switch(event->xfocus.mode)
			{
				case NotifyNormal:
					ELOG(" - NotifyNormal");
					break;
				case NotifyGrab:
					ELOG(" - NotifyGrab");
					break;
				case NotifyUngrab:
					ELOG(" - NotifyUngrab");
					break;
			}

			if(event->xfocus.send_event)
				ELOG(" - got from SendEvent");
			//curr_frame->unfocus();
		}
			return 1;
		case PropertyNotify:
			return property_event(event->xproperty);
		case ConfigureRequest:
			return configure_event(event->xconfigurerequest);
			return 1;

		default:
#ifdef _DEBUG
			const char* name = WindowManager::instance()->xevent_map[event->type];
			if(name)
				ELOG("Got unhandled %s in frame", name);
			else
				ELOG("Got unhandled event in frame (%i)", event->type);
#endif
			return 0;
	}
	return 0;
}

int FrameEventHandler::map_event(const XMapRequestEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::map_event(const XMapRequestEvent& e)");
	assert(e.window == curr_frame->window());

	curr_frame->map();

	return 1;
}

int FrameEventHandler::unmap_event(const XUnmapEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::unmap_event(const XUnmapEvent& e)");
	//assert(e.window == curr_frame->window());
	if(e.window != curr_frame->window())
		return 1;

	ELOG("FrameEventHandler: UnmapNotify (%s)", FRAME_NAME(curr_frame->fdata));
	if(e.from_configure)
		return 1;

	if(curr_frame->option(FrameOptIgnoreUnmap))
	{
		ELOG("Frame have FrameOptIgnoreUnmap, skiping this event");
		curr_frame->clear_option(FrameOptIgnoreUnmap);
	}
	else
		curr_frame->unmap();

	return 1;
}
	
int FrameEventHandler::reparent_event(const XReparentEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::reparent_event(const XReparentEvent& e)");
	ELOG("FrameEventHandler: ReparentNotify (%s)", FRAME_NAME(curr_frame->fdata));

	// echo
	if(e.parent == fl_xid(curr_frame))
		return 1;
	// app is trying to tear-off again ???
	if(e.parent == fl_xid(WindowManager::instance()))
		return 1;

	if(e.override_redirect)
	{
		ELOG("override_redirect from reparent_event");
	}

	EWARNING("Destroy in ReparentNotify!");
	curr_frame->destroy();

	return 1;
}

int FrameEventHandler::destroy_event(const XDestroyWindowEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::destroy_event(const XDestroyWindowEvent& e)");

	curr_frame->destroy();
	return 1;
}

int FrameEventHandler::client_message(const XClientMessageEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::client_message(const XClientMessageEvent& e)");

	if(!ValidateDrawable(e.window))
		return 1;

#ifdef _DEBUG
	Atom a = e.message_type;
	const char* name = WindowManager::instance()->atom_map[a];
	if(name)
		ELOG("FrameEventHandler: unhandled atom %s", name);
	else
		ELOG("FrameEventHandler: unhandled atom %x", a);
#endif
	
	return 1;
}

/* Handle property atoms. Here Netwm atoms are handled
 * last, giving them precedence and chance to overwrite
 * Icccm stuf (list XA_WM_NAME vs. _XA_NET_WM_NAME)
 */
int FrameEventHandler::property_event(const XPropertyEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::property_event(const XPropertyEvent& e)");

	if(e.atom == XA_WM_NAME)
		ELOG("XA_WM_NAME");
	if(e.atom == XA_WM_ICON_NAME)
		ELOG("XA_WM_ICON_NAME");
	if(e.atom == XA_WM_HINTS)
		ELOG("XA_WM_HINTS");
	if(e.atom == XA_WM_NORMAL_HINTS)
		ELOG("XA_WM_NORMAL_HINTS");
	if(e.atom == XA_WM_SIZE_HINTS)
		ELOG("XA_WM_SIZE_HINTS");
	if(e.atom == XA_WM_TRANSIENT_FOR)
		ELOG("XA_WM_TRANSIENT_FOR");
	if(e.atom == _XA_WM_COLORMAP_WINDOWS)
		ELOG("_XA_WM_COLORMAP_WINDOWS");
	if(e.atom == _XA_WM_STATE)
		ELOG("_XA_WM_STATE");
	if(e.atom == _XA_WM_PROTOCOLS)
		ELOG("_XA_WM_PROTOCOLS");
	if(e.atom == _XA_NET_WM_WINDOW_TYPE)
		ELOG("_XA_NET_WM_WINDOW_TYPE");
	if(e.atom == _XA_NET_WM_STRUT)
	{
		ELOG("_XA_NET_WM_STRUT");
		int dummy;
		WindowManager::instance()->hints()->netwm_strut(curr_frame->fdata->window, 
				&dummy, &dummy, &dummy, &dummy);
	}
	if(e.atom == _XA_NET_WM_NAME)
		ELOG("_XA_NET_WM_NAME");
	if(e.atom == _XA_NET_WM_ICON)
		ELOG("_XA_NET_WM_ICON");
	if(e.atom == _XA_KWM_WIN_ICON)
		ELOG("_XA_KWM_WIN_ICON");

	return 1;
}

/* Handle EnterNotify and LeaveNotify events.
 * Here is used only LeaveNotify to change cursor, since it will be
 * trigered only on window borders (exact what is needed). On other
 * hand, entering frame's child (or plain window) is handled by
 * window itself, with ability to change cursor as like.
 * Note, this event could not be simulated with FL_LEAVE, since
 * it will be trigered only when mouse if off from whole window
 * (including childs).
 *
 * Also, checkings of frame resizings are must, since mouse moving is
 * usually faster than window resizing, so we will get flickering in
 * cursors changes if not checked.
 *
 * TODO: better will be cursor is grabbed !
 */
int FrameEventHandler::enter_leave_event(const XCrossingEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::enter_event(const XEnterWindowEvent& e)");
	if(curr_frame->state(FrameStateUnmapped))
		return 1;

	if(e.type == LeaveNotify && !curr_frame->resizing())
		curr_frame->set_cursor(CURSOR_DEFAULT);

	return 1;
}

/* This message we can get for windows we know about and we don't know
 * about (since all root messages are redirected to us). So we first check
 * is window is managed by us, and apply changes to it via our functions.
 * Otherwise, we configure window directly.
 *
 * Note: in fltk's fluid(1.1.6), setting x coords will send us pretty
 * amount of junk (change x and y in the same time !!!). This is probably
 * bug in fluid.
 */
int FrameEventHandler::configure_event(const XConfigureRequestEvent& e)
{
	TRACE_FUNCTION("int FrameEventHandler::configure_event(const XConfigureRequestEvent& e)");

	if(curr_frame->state(FrameStateUnmapped))
		return 1;

	if(curr_frame->window() == e.window)
	{
		ELOG("ConfigureRequest from frame");
		if(!ValidateDrawable(curr_frame->window()))
			return 1;

		int x_pos = curr_frame->fdata->plain.x;
		int y_pos = curr_frame->fdata->plain.y;
		int w_sz  = curr_frame->fdata->plain.w;
		int h_sz  = curr_frame->fdata->plain.h;

		if(e.value_mask & CWX)
			x_pos = e.x;
		if(e.value_mask & CWY)
			y_pos = e.y;
		if(e.value_mask & CWWidth)
			w_sz = e.width;
		if(e.value_mask & CWHeight)
			h_sz = e.height;

		if(e.value_mask & CWStackMode)
		{
			if(e.detail == Above)
				curr_frame->raise();
			if(e.detail == Below)
				curr_frame->lower();
		}

		curr_frame->set_size(x_pos, y_pos, w_sz, h_sz, true);	
	}
	else
	{
		ELOG("ConfigureRequest from unhandled window");

		if(ValidateDrawable(e.window))
		{
			XWindowChanges wc;
			wc.x = e.x;
			wc.y = e.y;
			wc.width = e.width;
			wc.height = e.height;
			wc.stack_mode = e.detail;
			XConfigureWindow(fl_display, e.window, e.value_mask, &wc);
		}
	}

	return 1;
}
