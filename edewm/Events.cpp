#include <stdio.h>

#include "Frame.h"
#include "Desktop.h"
#include "Netwm.h"
#include "Windowmanager.h"
#include "Icon.h"
#include "debug.h"

#include "Winhints.h"

bool Frame::configure_event(const XConfigureRequestEvent *e)
{
	DBG("configure_event 0x%lx", e->window);
	// Always update flags and border...
	get_funcs_from_type();
	updateBorder();

	unsigned long mask = e->value_mask;
	//if (mask & CWBorderWidth) app_border_width = e->border_width;

	// Try to detect if the application is really trying to move the
	// window, or is simply echoing it's postion, possibly with some
	// variation (such as echoing the parent window position), and
	// dont' move it in that case:
	int X = (mask & CWX && e->x != x()) ? e->x-offset_x : x();
	int Y = (mask & CWY && e->y != y()) ? e->y-offset_y : y();

	int W = (mask & CWWidth) ? e->width+offset_w : w();
	int H = (mask & CWHeight) ? e->height+offset_h : h();

	// Fix Rick Sayre's program that resizes it's windows bigger than the maximum size:
	// Just turn off sizehints
	if(W > size_hints->max_width + offset_w) size_hints->max_width = 0;
	if(H > size_hints->max_height + offset_h) size_hints->max_height = 0;

	set_size(X, Y, W, H);

	if (e->value_mask & CWStackMode && e->detail == Above && state()==NORMAL)
		raise();

	return true;
}

bool Frame::map_event(const XMapRequestEvent *e)
{
	DBG("map_event 0x%lx", e->window);

	if(state()!=NORMAL) state(NORMAL);
	activate();
	raise();

	return true;
}

bool Frame::unmap_event(const XUnmapEvent *e)
{
	DBG("unmap_event 0x%lx", e->window);

	if(state()==OTHER_DESKTOP) return true;
	if(state()==ICONIC) return true;

	if(e->from_configure)
	{
		DBG("Unmap notify from configure\n");
		state(UNMAPPED);
	} else if(state_flags_&IGNORE_UNMAP)
	clear_state_flag(IGNORE_UNMAP);
	else
		state(UNMAPPED);

	WindowManager::update_client_list();

	return true;
}

bool Frame::destroy_event(const XDestroyWindowEvent *e)
{
	DBG("destroy_event 0x%lx", e->window);
	destroy_frame();
	return true;
}

bool Frame::reparent_event(const XReparentEvent *e)
{
	DBG("reparent_event 0x%lx", e->window);

	if(e->parent==fl_xid(this)) return true;	  // echo
	if(e->parent==fl_xid(root)) return true;	  // app is trying to tear-off again?

	DBG("Destroy in reparent_event");
	destroy_frame();							  // guess they are trying to paste tear-off thing back?

	return true;
}

#include <efltk/fl_ask.h>
bool Frame::clientmsg_event(const XClientMessageEvent *e)
{
	DBG("clientmsg_event 0x%lx", e->window);

	if(e->message_type == _XA_WM_CHANGE_STATE && e->format == 32)
	{
		DBG("WIN change state");
		if (e->data.l[0] == NormalState) raise();
		else if (e->data.l[0] == IconicState) iconize();
		return true;

	}
	else if(e->message_type == _XA_NET_WM_STATE)
	{
		/* int action = e->data.l[0]; */
		Atom atom1 = e->data.l[1];
		Atom atom2 = e->data.l[2];

		// We accepts only HORZ - VERT combination (sent by taskbar).
		// Horizontal or vertical maximization only, is not supported for now.
		if((atom1 == _XA_NET_WM_STATE_MAXIMIZED_HORZ || atom1 == _XA_NET_WM_STATE_MAXIMIZED_VERT) &&
			atom2 == _XA_NET_WM_STATE_MAXIMIZED_HORZ || atom2 == _XA_NET_WM_STATE_MAXIMIZED_VERT)
		{
			maximize();
			return true;
		}

		DBG("NET state");
	}
	else if(e->message_type == _XA_NET_ACTIVE_WINDOW)
	{

		DBG("Net active window: %ld", e->window);
		raise();
		activate();
		return true;

	}
	else if(e->message_type == _XA_NET_CLOSE_WINDOW)
	{

		DBG("Net close window: %ld", e->window);
		close();
		return true;

	}
	else if(e->message_type == _XA_NET_WM_DESKTOP)
	{

		DBG("Net wm desktop: %ld", e->window);
		long d = e->data.l[0];
		if(d<0) d=1;
		else if(d>8) d=8;
		else if((unsigned)d == 0xFFFFFFFE || (unsigned)d == 0xFFFFFFFF) d=-2;
		else d++;

		if(d>0) desktop(Desktop::desktop(d));
		else desktop(0);
		return true;

	}
	else if(e->message_type ==_XA_EDE_WM_ACTION)
	{
		Atom atom = e->data.l[0];
		if(atom == _XA_EDE_WM_RESTORE_SIZE)
		{
			restore();
			return true;
		}
		if(atom == _XA_EDE_WM_LOGOUT)
		{
			root->shutdown();
			return true;
		}
	}

	//        Fl::warning("flwm: unexpected XClientMessageEvent, type 0x%lx, "
	//                    "window 0x%lx\n", e->message_type, e->window);
	return true;
}

bool Frame::colormap_event(const XColormapEvent *e)
{
	DBG("colormap_event 0x%lx", e->window);

	// this field is called "new" in the old C++-unaware Xlib
	if(e->c_new)
	{
		colormap = e->colormap;
		if(active())
			installColormap();
	}

	return true;
}

bool Frame::property_event(const XPropertyEvent *e)
{
	DBG("property_event 0x%lx", e->window);

	Atom a = e->atom;

	switch(a)
	{

		case XA_WM_NAME:
			getLabel((e->state==PropertyDelete), XA_WM_NAME);
			break;

		case XA_WM_ICON_NAME:
			break;

		case XA_WM_HINTS:
		{
			get_wm_hints();
			Icon *old_i = icon_;
			icon_ = new Icon(wm_hints);
			if(old_i) delete old_i;
			old_i=0;
			title->redraw();
			redraw();
		}
		break;

		case XA_WM_NORMAL_HINTS:
		case XA_WM_SIZE_HINTS:
			ICCCM::size_hints(this);
			break;

		case XA_WM_TRANSIENT_FOR:
			XGetTransientForHint(fl_display, window_, &transient_for_xid);
			fix_transient_for();
			break;

		default:
			if(a==_XA_WM_COLORMAP_WINDOWS)
			{

				getColormaps();
				if (active()) installColormap();

			}
			else if(a==_XA_WM_STATE)
			{

				// it's not clear if I really need to look at this.  Need to make
				// sure it is not seeing the state echoed by the application by
				// checking for it being different...
				switch (getIntProperty(_XA_WM_STATE, _XA_WM_STATE, state()))
				{
					case IconicState:
						if (state() == NORMAL || state() == OTHER_DESKTOP)
						{
							iconize();
						}
						break;
					case NormalState:
						if (state() != NORMAL && state() != OTHER_DESKTOP)
						{
							raise();
						}
						break;
				}

			}
			else if(a==_XATOM_MWM_HINTS)
			{

				// some SGI Motif programs change this after mapping the window!!!
				MWM::get_hints(this);
				if(MWM::update_hints(this))		  // returns true if any flags changed
				{
					fix_transient_for();
					updateBorder();
				}

			}
			else if(a==_XA_WM_PROTOCOLS)
			{

				get_protocols();
				//getMotifHints(); // get Motif hints since they may do something with QUIT

			}
			else if(a==_XA_NET_WM_WINDOW_TYPE)
			{

				DBG("New NET WM window type!");

				if(NETWM::get_window_type(this))
				{
					// This call overwrites MWM hints!
					get_funcs_from_type();
				}								  // && wintype==TYPE_NORMAL && decor_flag(BORDER)) {
				else if(transient_for_xid)
				{
					//title->h(15); title->label(0);
					if(decor_flag(BORDER) || decor_flag(THIN_BORDER))
					{
						clear_decor_flag(BORDER);
						set_decor_flag(THIN_BORDER);
						wintype = TYPE_DIALOG;
						get_funcs_from_type();
					}
				}

			}
			else if(a==_XA_NET_WM_STRUT)
			{

				DBG("New NET WM STRUT");
				NETWM::get_strut(this);
				root->update_workarea(true);

			}
			else if(a== _XA_NET_WM_NAME)
			{

				DBG("New NET WM NAME!");
				getLabel((e->state==PropertyDelete), _XA_NET_WM_NAME);

			}
			else if(a== _XA_NET_WM_ICON)
			{

				DBG("New NET icon!");

			}
			else if(a== _XA_KWM_WIN_ICON)
			{

				DBG("New KWM icon!");

			}
			else
			{

				DBG("Unhandled atom %lx\n", a);
				return false;
			}
			break;
	}

	return true;
}

bool Frame::visibility_event(const XVisibilityEvent *e)
{
	DBG("visibility_event 0x%lx", e->window);

	// POSSIBLE SPEED UP!
	// if e->state==FullyObscured we dont need to
	// map/draw window, when we change desktop!

	/*if(e->state == VisibilityUnobscured)
	   DBG("Fully visible");
	else
	   DBG("Partial visible");
	*/

	return true;
}

#ifdef SHAPE
bool Frame::shape_event(const XShapeEvent *e)
{
	DBG("shape event 0x%lx", e->window);
	XShapeSelectInput(fl_display, window_, ShapeNotifyMask);
	if(get_shape())
	{
		clear_decor_flag(BORDER|THIN_BORDER);
		XShapeCombineShape(fl_display, fl_xid(this), ShapeBounding,
			0, 0, window_,
			ShapeBounding, ShapeSet);
	}
	return true;
}
#endif
