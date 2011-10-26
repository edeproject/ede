#include <stdio.h>
#define FL_LIBRARY 1
#include <FL/Fl.H>
#include <edelib/Debug.h>
#include <edelib/Netwm.h>

#include "Applet.h"
#include "Panel.h"
#include "Tray.h"
#include "TrayWindow.h"

#define SYSTEM_TRAY_REQUEST_DOCK   0
#define SYSTEM_TRAY_BEGIN_MESSAGE  1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

EDELIB_NS_USING(netwm_window_get_icon)
EDELIB_NS_USING(netwm_window_get_title)

/* multiple tray's are not allowed anyways so this can work */
Tray *curr_tray = 0;

static int handle_xevent(int e) {
	if(fl_xevent->type == ClientMessage && fl_xevent->xclient.message_type == curr_tray->get_opcode()) {
		if(fl_xevent->xclient.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			E_DEBUG(E_STRLOC ": Dock request for %i\n", fl_xevent->xclient.data.l[2]);
			curr_tray->embed_window(fl_xevent->xclient.data.l[2]);
			return true;
		}

		if(fl_xevent->xclient.data.l[1] == SYSTEM_TRAY_BEGIN_MESSAGE) {
			E_DEBUG(E_STRLOC ": SYSTEM_TRAY_BEGIN_MESSAGE\n");
			return true;
		}

		if(fl_xevent->xclient.data.l[1] == SYSTEM_TRAY_CANCEL_MESSAGE) {
			E_DEBUG(E_STRLOC ": SYSTEM_TRAY_CANCEL_MESSAGE\n");
			return true;
		}
	} else if(fl_xevent->type == DestroyNotify) {
		XDestroyWindowEvent xev = fl_xevent->xdestroywindow;
		curr_tray->unembed_window(xev.window);
		return false;
	}

	return false;
}

Tray::Tray() : Fl_Group(0, 0, 5, 25), opcode(0) {
	box(FL_FLAT_BOX);
	color(FL_RED);
	register_notification_area();
}

Tray::~Tray() {
	Atom sel;
	char sel_name[20];

	snprintf(sel_name, sizeof(sel_name), "_NET_SYSTEM_TRAY_S%d", fl_screen);
	sel = XInternAtom(fl_display, sel_name, False);
	XSetSelectionOwner(fl_display, sel, None, CurrentTime);
}

void Tray::register_notification_area(void) {
	Atom sel;
	char sel_name[20];

	snprintf(sel_name, sizeof(sel_name), "_NET_SYSTEM_TRAY_S%d", fl_screen);
	sel = XInternAtom(fl_display, sel_name, False);
	if(XGetSelectionOwner(fl_display, sel)) {
		E_WARNING(E_STRLOC ": Notification area service is already provided by different program\n");
		return;
	}

	/* register */
	XSetSelectionOwner(fl_display, sel, fl_message_window, CurrentTime);
	if(XGetSelectionOwner(fl_display, sel) != fl_message_window) {
		E_WARNING(E_STRLOC ": Unable to register notification area service\n");
		return;
	}
	
	XClientMessageEvent xev;
	xev.type = ClientMessage;
	xev.message_type = XInternAtom(fl_display, "MANAGER", False);
	xev.format = 32;
	xev.data.l[0] = CurrentTime;
	xev.data.l[1] = sel;
	xev.data.l[2] = fl_message_window;
	xev.data.l[3] = xev.data.l[4] = 0;

	XSendEvent(fl_display, RootWindow(fl_display, fl_screen), False, StructureNotifyMask, (XEvent*)&xev);

	opcode = XInternAtom(fl_display, "_NET_SYSTEM_TRAY_OPCODE", False);
	message_data = XInternAtom(fl_display, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);

	curr_tray = this;
	Fl::add_handler(handle_xevent);
}

void Tray::embed_window(Window id) {
	TrayWindow *win = new TrayWindow(25, 25);
	win->end();
	win->set_image(netwm_window_get_icon(id));
	win->set_tooltip(netwm_window_get_title(id));

	add_to_tray(win);
	win->show();

	XReparentWindow(fl_display, id, fl_xid(win), 0, 0); 
	/* remove any pixmap from reparented window, so 'TrayWindow' can display own content */
	XSetWindowBackgroundPixmap(fl_display, id, None);

	/* need to know when child dies */
	XSelectInput(fl_display, fl_xid(win), SubstructureNotifyMask);

	WinInfo i;
	i.id = id;
	i.win = win;
	/* and to list */
	win_list.push_back(i);
}

void Tray::unembed_window(Window id) {
	WinListIt it = win_list.begin(), ite = win_list.end();

	for(; it != ite; ++it) {
		if((*it).id == id) {
			remove_from_tray((*it).win);
			win_list.erase(it);
			return;
		}
	}
}

void Tray::add_to_tray(Fl_Widget *win) {
	win->position(x(), y());

	insert(*win, 0);
	int W = w() + win->w() + 5;
	w(W);

	EDE_PANEL_GET_PANEL_OBJECT(this)->relayout();
}

void Tray::remove_from_tray(Fl_Widget *win) {
	remove(win);

	int W = w() - win->w() - 5;
	w(W);

	EDE_PANEL_GET_PANEL_OBJECT(this)->relayout();
}

int Tray::handle(int e) {
	WinListIt it = win_list.begin(), ite = win_list.end();

	for(; it != ite; ++it) {
		Fl_Window *n = (*it).win;
		if(Fl::event_x() >= n->x() && Fl::event_y() <= (n->x() + n->w()) &&
			Fl::event_y() >= n->y() && Fl::event_y() <= (n->y() + n->h()))
		{
			return n->handle(e);
		}
	}

	return Fl_Group::handle(e);
}

EDE_PANEL_APPLET_EXPORT (
 Tray, 
 EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT,
 "System Tray",
 "0.1",
 "empty",
 "Sanel Zukan, Vedran Ljubovic"
)
