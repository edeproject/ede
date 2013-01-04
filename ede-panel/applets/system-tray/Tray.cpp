#include <stdio.h>
#define FL_LIBRARY 1
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <edelib/Debug.h>

#include "Panel.h"
#include "Tray.h"

#define SYSTEM_TRAY_REQUEST_DOCK   0
#define SYSTEM_TRAY_BEGIN_MESSAGE  1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

/* try orientation type */
#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

/* dimenstions of tray items */
#define TRAY_ICON_W 25
#define TRAY_ICON_H 25

/* space between tray icons */
#define TRAY_ICONS_SPACE 5

/* multiple tray's are not allowed anyways so this can work */
static Tray *curr_tray = 0;

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
		E_DEBUG(E_STRLOC ": Unock request for %i\n", fl_xevent->xclient.data.l[2]);
		curr_tray->unembed_window(xev.window);
		return false;
	} else if(fl_xevent->type == ConfigureNotify) {
		curr_tray->configure_notify(fl_xevent->xconfigure.window);
		return false;
	}

	return false;
}

static int validate_drawable(Display *d, Window xid) {
	Window root;
	int x, y, st;
	unsigned int w, h, b, depth;

	XSync(d, False);
	st = XGetGeometry(d, xid, &root, &x, &y, &w, &h, &b, &depth);

	XSync(d, False);
	return st;
}

Tray::Tray() : Fl_Group(0, 0, 1, 25), opcode(0) {
	box(FL_FLAT_BOX);
	register_notification_area();
}

Tray::~Tray() {
	Atom sel;
	char sel_name[20];

	snprintf(sel_name, sizeof(sel_name), "_NET_SYSTEM_TRAY_S%d", fl_screen);
	sel = XInternAtom(fl_display, sel_name, False);
	XSetSelectionOwner(fl_display, sel, None, CurrentTime);
}

void Tray::distribute_children(void) {
	int X, Y;

	X = x();
	Y = y();
	for(int i = 0; i < children(); i++) {
		child(i)->position(X, Y);
		E_DEBUG(E_STRLOC ": child %i at %i %i\n", i, child(i)->x(), child(i)->y());
		X += child(i)->w() + TRAY_ICONS_SPACE;
	}
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

	/* setup visual atom so registering icons knows how to draw images */
	Atom visual_atom = XInternAtom(fl_display, "_NET_SYSTEM_TRAY_VISUAL", False);
	XChangeProperty(fl_display, fl_message_window, visual_atom, XA_VISUALID, 32, PropModeReplace, (unsigned char*)&fl_visual->visualid, 1);

	/* 
	 * setup orientation; also required by spec
	 * panel is always in horizontal position, so value is hardcoded here
	 */
	Atom or_atom = XInternAtom(fl_display, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	int  or_val  = SYSTEM_TRAY_ORIENTATION_HORZ;
	XChangeProperty(fl_display, fl_message_window, or_atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&or_val, 1);
	
	/* notify root who is manager */
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
	/* make sure we don't get some windows that could not be displayed */
	E_RETURN_IF_FAIL(validate_drawable(fl_display, id) == 1);

	Fl_Window *win = new Fl_Window(TRAY_ICON_W, TRAY_ICON_H);
	win->end();

	add_to_tray(win);
	win->show();

	/* do real magic
	 * Some apps require we explicitly resize tray window, but some ignores it. So after window was shown, ConfigureNotify
	 * will be fired up and we will handle those windows who are not resized.
	 */
	XResizeWindow(fl_display, id, win->w(), win->h());
	XReparentWindow(fl_display, id, fl_xid(win), 0, 0); 
	XMapRaised(fl_display, id);

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

void Tray::configure_notify(Window id) {
	WinListIt it = win_list.begin(), ite = win_list.end();

	for(; it != ite; ++it) {
		if(it->id == id) {
			XWindowChanges c;
			c.width = TRAY_ICON_W;
			c.height = TRAY_ICON_H;
			c.x = 0;
			c.y = 0;

			XConfigureWindow(fl_display, id, CWWidth | CWHeight | CWX | CWY, &c);
			return;
		}
	}
}

void Tray::add_to_tray(Fl_Widget *win) {
	insert(*win, 0);
	w(w() + win->w() + TRAY_ICONS_SPACE);
	
	distribute_children();

	//redraw();
	EDE_PANEL_GET_PANEL_OBJECT(this)->relayout();
}

void Tray::remove_from_tray(Fl_Widget *win) {
	remove(win);
	w(w() - win->w() - TRAY_ICONS_SPACE);

	win->hide();
	delete win;

	distribute_children();

	EDE_PANEL_GET_PANEL_OBJECT(this)->relayout();
	EDE_PANEL_GET_PANEL_OBJECT(this)->redraw();
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
