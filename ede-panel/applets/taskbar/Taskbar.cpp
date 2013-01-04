/*
 * $Id$
 *
 * Copyright (C) 2012 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <edelib/Debug.h>
#include <edelib/Netwm.h>

#include "TaskButton.h"
#include "Taskbar.h"

#define DEFAULT_CHILD_W 175
#define DEFAULT_SPACING 5

EDELIB_NS_USING(netwm_callback_add)
EDELIB_NS_USING(netwm_callback_remove)
EDELIB_NS_USING(netwm_window_get_all_mapped)
EDELIB_NS_USING(netwm_window_is_manageable)
EDELIB_NS_USING(netwm_window_get_workspace)
EDELIB_NS_USING(netwm_window_get_active)
EDELIB_NS_USING(netwm_window_set_active)
EDELIB_NS_USING(netwm_window_set_state)
EDELIB_NS_USING(netwm_workspace_get_current)
EDELIB_NS_USING(wm_window_set_state)
EDELIB_NS_USING(wm_window_get_state)
EDELIB_NS_USING(NETWM_CHANGED_ACTIVE_WINDOW)
EDELIB_NS_USING(NETWM_CHANGED_CURRENT_WORKSPACE)
EDELIB_NS_USING(NETWM_CHANGED_WINDOW_LIST)
EDELIB_NS_USING(NETWM_CHANGED_WINDOW_NAME)
EDELIB_NS_USING(NETWM_CHANGED_WINDOW_ICON)
EDELIB_NS_USING(WM_WINDOW_STATE_ICONIC)
EDELIB_NS_USING(NETWM_STATE_ACTION_TOGGLE)
EDELIB_NS_USING(NETWM_STATE_HIDDEN)

static void button_cb(TaskButton *b, void *t) {
	Taskbar *tt = (Taskbar*)t;

	if(Fl::event_button() == FL_RIGHT_MOUSE)
		b->display_menu();
	else 
		tt->activate_window(b);
}

static void net_event_cb(int action, Window xid, void *data) {
	E_RETURN_IF_FAIL(data != NULL);

	/* this is a message, so property is not changed and netwm_window_get_active() must be called */
	if(action == NETWM_CHANGED_ACTIVE_WINDOW) {
		Taskbar *tt = (Taskbar*)data;
		tt->update_active_button();
		return;
	}
	
	if(action == NETWM_CHANGED_CURRENT_WORKSPACE) {
		Taskbar *tt = (Taskbar*)data;
		tt->update_workspace_change();
		return;
	}

	if(action == NETWM_CHANGED_WINDOW_LIST) {
		Taskbar *tt = (Taskbar*)data;
		tt->update_task_buttons();
		return;
	}

	if(action == NETWM_CHANGED_WINDOW_NAME) {
		Taskbar *tt = (Taskbar*)data;
		tt->update_child_title(xid);
		return;
	}

	if(action == NETWM_CHANGED_WINDOW_ICON) {
		Taskbar *tt = (Taskbar*)data;
		tt->update_child_icon(xid);
		return;
	}
}

Taskbar::Taskbar() : Fl_Group(0, 0, 40, 25), curr_active(NULL), prev_active(NULL) {
	end();
	fixed_layout = false;

	update_task_buttons();
	netwm_callback_add(net_event_cb, this);
}

Taskbar::~Taskbar() {
	netwm_callback_remove(net_event_cb);
}

void Taskbar::update_task_buttons(void) {
	Window *wins;
	int nwins = netwm_window_get_all_mapped(&wins);

	if(nwins < 1) {
		if(children() > 0) clear();
		return;
	}

	TaskButton *b;
	int curr_workspace = netwm_workspace_get_current();
	bool need_full_redraw = false;

	for(int i = 0, found; i < children(); i++) {
		found = 0;
		b = (TaskButton*)child(i);

		for(int j = 0; j < nwins; j++) {
			if(b->get_window_xid() == wins[j]) {
				/* assure this window is on current workspace */
				found = 1;
				break;
			} 
		}

		if(!found) {
			/* FLTK since 1.3 optimized Fl_Group::remove() */
#if (FL_MAJOR_VERSION >= 1) && (FL_MINOR_VERSION >= 3)
			remove(i);
#else
			remove(b);
#endif
			/* Fl_Group does not call delete on remove() */
			delete b;
			need_full_redraw = true;
		} 
	}

	/* now see which one needs to be created */
	for(int i = 0, found; i < nwins; i++) {
		found = 0;

		for(int j = 0; j < children(); j++) {
			b = (TaskButton*)child(j);

			if(b->get_window_xid() == wins[i]) {
				found = 1;
				break;
			}
		}

		if(found || !netwm_window_is_manageable(wins[i]))
			continue;

		Window transient_prop_win = None;

		/*
		 * see if it has WM_TRANSIENT_FOR hint set; transient_prop_win would point to parent window, but
		 * parent should not be root window for this screen
		 */
		if(XGetTransientForHint(fl_display, wins[i], &transient_prop_win) 
		   && (transient_prop_win != None)
		   && (transient_prop_win != RootWindow(fl_display, fl_screen)))
		{
			continue;
		}
		
		/* create button */
		if(curr_workspace == netwm_window_get_workspace(wins[i])) {
			b = new TaskButton(0, 0, DEFAULT_CHILD_W, 25);
			b->set_window_xid(wins[i]);
			b->update_title_from_xid();
			b->update_image_from_xid();

			/* catch the name changes */
			XSelectInput(fl_display, wins[i], PropertyChangeMask | StructureNotifyMask);
			b->callback((Fl_Callback*)button_cb, this);
			add(b);

			need_full_redraw = true;
		}
	}

	XFree(wins);
	layout_children();
	update_active_button(!need_full_redraw);

	if(need_full_redraw) panel_redraw();
}

void Taskbar::update_workspace_change(void) {
	if(children() < 1) return;

	TaskButton *b;
	int p, curr_workspace = netwm_workspace_get_current();

	for(int i = 0; i < children(); i++) {
		b = (TaskButton*)child(i);
		/*
		 * get fresh value, as wm can change it without notification
		 * (e.g. pekwm window drag to different workspace)
		 */
		p = netwm_window_get_workspace(b->get_window_xid());
		(p == curr_workspace) ? b->show() : b->hide();
	}	

	layout_children();
	redraw();
}

void Taskbar::resize(int X, int Y, int W, int H) {
	Fl_Widget::resize(X, Y, W, H);
	layout_children();
}

void Taskbar::layout_children(void) {
	if(!children())
		return;

	Fl_Widget *o;
	int X, Y, W, reduce = 0, sz = 0, all_buttons_w = 0;

	X = x() + Fl::box_dx(box());
	Y = y() + Fl::box_dy(box());
	W = w() - Fl::box_dw(box());

	for(int i = 0; i < children(); i++) {
		o = child(i);
		if(!o->visible()) continue;

		sz++;

		/* resize every child to default size so we can calculate total length */
		o->resize(o->x(), o->y(), (fixed_layout ? DEFAULT_CHILD_W : W), o->h());
		all_buttons_w += o->w();

		/* do not include ending spaces */
		if(i != children() - 1)
			all_buttons_w += DEFAULT_SPACING;
	}

	/*
	 * find reduction size
	 * TODO: due large number of childs, 'reduce' could be bigger than child
	 * width, yielding drawing issues on borders
	 */
	if(all_buttons_w > W)
		reduce = (all_buttons_w - W) / sz;

	/* now, position each child and resize it if needed */
	for(int i = 0; i < children(); i++) {
		o = child(i);
		if(!o->visible()) continue;

		o->resize(X, Y, o->w() - reduce, o->h());
		X += o->w();

		if(i != children() - 1)
			X += DEFAULT_SPACING;
	}
}

void Taskbar::update_active_button(bool do_redraw, int xid) {
	if(!children())
		return;

	if(xid == -1) {
		xid = netwm_window_get_active();
		/* TODO: make sure panel does not get 'active', or all buttons will be FL_UP_BOX */
	}

	TaskButton *o;
	for(int i = 0; i < children(); i++) {
		o = (TaskButton*)child(i);
		if(!o->visible()) continue;

		if(o->get_window_xid() == (Window)xid) {
			o->box(FL_DOWN_BOX);
			curr_active = o;
		} else {
			o->box(FL_UP_BOX);
		}
	}

	if(do_redraw) redraw();
}

void Taskbar::activate_window(TaskButton *b) {
	E_RETURN_IF_FAIL(b != NULL);

	Window xid = b->get_window_xid();

	/* if clicked on activated button, it will be minimized, then next one will be activated */
	if(b == curr_active) {
		if(wm_window_get_state(xid) != WM_WINDOW_STATE_ICONIC) {
			/* minimize if not so */
			wm_window_set_state(xid, WM_WINDOW_STATE_ICONIC);

			if(prev_active && 
			   prev_active != b && 
			   wm_window_get_state(prev_active->get_window_xid()) != WM_WINDOW_STATE_ICONIC) 
			{
				xid = prev_active->get_window_xid();
				b   = prev_active;
			} else {
				return;
			}
		}
	} 

	/* active or restore minimized */
	netwm_window_set_active(xid, 1);
	update_active_button(false, xid);

	prev_active = curr_active;
	curr_active = b;
}

void Taskbar::update_child_title(Window xid) {
	E_RETURN_IF_FAIL(xid >= 0);

	TaskButton *o;
	for(int i = 0; i < children(); i++) {
		o = (TaskButton*)child(i);

		if(o->visible() && o->get_window_xid() == xid) {
			o->update_title_from_xid();
			break;
		}
	}
}

void Taskbar::update_child_icon(Window xid) {
	E_RETURN_IF_FAIL(xid >= 0);

	TaskButton *o;
	for(int i = 0; i < children(); i++) {
		o = (TaskButton*)child(i);

		if(o->visible() && o->get_window_xid() == xid) {
			o->update_image_from_xid();
			o->redraw();
			break;
		}
	}
}

void Taskbar::panel_redraw(void) {
	parent()->redraw();
}

EDE_PANEL_APPLET_EXPORT (
 Taskbar, 
 EDE_PANEL_APPLET_OPTION_ALIGN_LEFT | EDE_PANEL_APPLET_OPTION_RESIZABLE_H,
 "Taskbar",
 "0.1",
 "empty",
 "Sanel Zukan"
)
