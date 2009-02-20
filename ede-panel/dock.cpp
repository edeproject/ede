#include "dock.h"
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl.h>
#include <efltk/x.h>
#include <X11/Xatom.h>
//#include <vector>
#include <efltk/Fl_WM.h>

#include <unistd.h>
#include <X11/Xproto.h> //For CARD32

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

Dock *currentDock; //Can't pass a pointer to handle_x11_event; must be a better way though
int panelheight=0;

struct straydata {
	Window id;
	Fl_Window *win;
	straydata *next;
} *traydata;


Dock::Dock()
    : Fl_Group(0,0,0,0)
{
	register_notification_area();
//	box(FL_THIN_DOWN_BOX);
	color(FL_INVALID_COLOR); //Draw with parent color

	layout_align(FL_ALIGN_RIGHT);
	layout_spacing(1);

	traydata=0;
	end();
	winstate=0;
}

Dock::~Dock() {
	Atom selection_atom;
	char selection_atom_name[20];

	sprintf(selection_atom_name, "_NET_SYSTEM_TRAY_S%d", fl_screen);	
	selection_atom = XInternAtom (fl_display, selection_atom_name, false);
	XSetSelectionOwner(fl_display, selection_atom, None, CurrentTime);	

}


void Dock::register_notification_area() 
{
	Atom selection_atom;
	char selection_atom_name[20];

	sprintf(selection_atom_name, "_NET_SYSTEM_TRAY_S%d", fl_screen);	
	selection_atom = XInternAtom (fl_display, selection_atom_name, false);
	if (XGetSelectionOwner(fl_display, selection_atom)) {
		printf("The notification area service is being provided by a different program. I'll just handle EDE components. \n");
		return;
	}

	//Register
	XSetSelectionOwner(fl_display, selection_atom, fl_message_window, CurrentTime);	

	//Check registration was successful
	if (XGetSelectionOwner(fl_display, selection_atom) != fl_message_window) {
		printf("Could not register as notification area service.\n");
		return;
	}

	XClientMessageEvent xev;
	
	xev.type = ClientMessage;
	xev.message_type = XInternAtom (fl_display, "MANAGER", false);
	
	xev.format = 32;
	xev.data.l[0] = CurrentTime;
	xev.data.l[1] = selection_atom;
	xev.data.l[2] = fl_message_window;
	xev.data.l[3] = 0;
	xev.data.l[4] = 0;
	
	XSendEvent (fl_display, RootWindow(fl_display, fl_screen), false, StructureNotifyMask, (XEvent *)&xev);

	opcode = XInternAtom(fl_display, "_NET_SYSTEM_TRAY_OPCODE", false);
	message_data = XInternAtom(fl_display,"_NET_SYSTEM_TRAY_MESSAGE_DATA", false);

	currentDock = this;
	Fl::add_handler(handle_x11_event);
}

int Dock::handle_x11_event(int e) 
{
	if (fl_xevent.type == ClientMessage && fl_xevent.xclient.message_type == currentDock->opcode ) {
		if (fl_xevent.xclient.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			currentDock->embed_window(fl_xevent.xclient.data.l[2]);	
			return true;
		}

		if (fl_xevent.xclient.data.l[1] == SYSTEM_TRAY_BEGIN_MESSAGE) {
			return true;
		}

		if (fl_xevent.xclient.data.l[1] == SYSTEM_TRAY_CANCEL_MESSAGE) {
			return true;
		}
	} else if (fl_xevent.type == DestroyNotify) {
		XDestroyWindowEvent xev = fl_xevent.xdestroywindow;

		// Loop trough linked list to find destroyed window
		struct straydata *p=traydata;
		struct straydata *p1=0;
		while (p!=0) {
			if (p->id == xev.window) {
				currentDock->remove_from_tray(p->win);
				delete p->win;
				if (p1 == 0)
					traydata = p->next;
				else
					p1->next = p->next;
				delete p;
				break;
			}
			p1=p;
			p=p->next;
		}
	}
	return false;
}


// Helper for Dock::embed_window()
// Kopied from kdelibs/kdeui/qxembed.cpp
static int get_parent(Window winid, Window *out_parent)
{
    Window root, *children=0;
    unsigned int nchildren;
    int st = XQueryTree(fl_display, winid, &root, out_parent, &children, &nchildren);
    if (st && children) 
        XFree(children);
    return st;
}


void Dock::embed_window(Window id) 
{
	if (id==0) return;

	// Store window id in a linked list structure
	struct straydata *p=traydata;
	struct straydata *p1;
	while (p!=0) {
		// Sometimes the app will reuse the same id on next start?
//		if (p->id == id) 
//			return;
		p1=p;
		p=p->next;
	}
	p = new straydata;
	if (traydata == 0) { traydata=p; } else { p1->next = p; }
	p->id = id;
	p->next = 0;

	Fl_Window *win = new Fl_Window(24, 24);
	win->end();
	this->add_to_tray(win);
	win->show();
	p->win = win;

//	printf("id: %ld -> %ld\n", id, fl_xid(win));
	XReparentWindow(fl_display, id, fl_xid(win), 0, 0);

	// Hack to get KDE icons working...
	Window parent = 0;
	get_parent(id, &parent);
//	printf(" ++ parent: %ld\n", parent);
	XMapWindow(fl_display, id);

	//Need to know when child dies
	XSelectInput(fl_display, fl_xid(win), SubstructureNotifyMask);



/*	Some ideas from KDE code... consider using (sometime)

	----------------

	Does this have any effect?

	static Atom hack_atom = XInternAtom( fl_display, "_KDE_SYSTEM_TRAY_EMBEDDING", False );
	XChangeProperty( fl_display, id, hack_atom, hack_atom, 32, PropModeReplace, NULL, 0 );
		.....
	XDeleteProperty( fl_display, id, hack_atom );


	----------------

	Force window to withdraw (?)

// Helper for Dock::embed_window()
// Kopied from kdelibs/kdeui/qxembed.cpp
static bool wstate_withdrawn( Window winid )
{
	static Atom _XA_WM_STATE = 0;
	if(!_XA_WM_STATE) _XA_WM_STATE = XInternAtom(fl_display, "WM_STATE", False);

    Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data;
    int r = XGetWindowProperty( fl_display, winid, _XA_WM_STATE, 0, 2,
                                false, AnyPropertyType, &type, &format,
                                &length, &after, &data );
    bool withdrawn = true;
    // L1610: Non managed windows have no WM_STATE property.
    //        Returning true ensures that the loop L1711 stops.
    if ( r == Success && data && format == 32 ) {
        uint32 *wstate = (uint32*)data;
        withdrawn  = (*wstate == WithdrawnState );
        XFree( (char *)data );
    }
    return withdrawn;
}


	if ( !wstate_withdrawn(id) ) {
		XWithdrawWindow(fl_display, id, fl_screen);
		XFlush(fl_display);
		// L1711: See L1610
		while (!wstate_withdrawn(id))
		sleep(1);
	}

	----------------

	XEMBED support:

#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
#define XEMBED_MODALITY_ON              10

// L0500: Helper to send XEmbed messages.
static void sendXEmbedMessage( Window window, long message, long detail = 0,
                               long data1 = 0, long data2 = 0)
{
	static Atom xembed=0;
	if (!xembed) xembed = XInternAtom( fl_display, "_XEMBED", False );
	if (!window) return;
	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = window;
	ev.xclient.message_type = xembed;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = message;
	ev.xclient.data.l[2] = detail;
	ev.xclient.data.l[3] = data1;
	ev.xclient.data.l[4] = data2;
	XSendEvent(fl_display, window, false, NoEventMask, &ev);
	XSync(fl_display, false);
}


	static Atom _XA_XEMBED_INFO = 0;
	if(!_XA_XEMBED_INFO) _XA_XEMBED_INFO = XInternAtom(fl_display, "_XEMBED_INFO", False);
	int status=1;
	CARD32* prop = (CARD32*)getProperty(id, _XA_XEMBED_INFO, XA_CARDINAL, 0, &status);
	if (status == 0) {
		// Not XEMBED
		.......
	} else {
		sendXEmbedMessage( id, XEMBED_EMBEDDED_NOTIFY, 0, fl_xid(win) );
	}
*/
}



void Dock::add_to_tray(Fl_Widget *w)
{
	insert(*w, 0);
	w->layout_align(FL_ALIGN_LEFT);
	w->show();
	
	int new_width = this->w() + w->width() + layout_spacing();
	this->w(new_width);

	parent()->relayout();
	Fl::redraw();
}

void Dock::remove_from_tray(Fl_Widget *w)
{
	remove(w);
	
	int new_width = this->w() - w->width() - layout_spacing();
	this->w(new_width);
	
	parent()->relayout();
	Fl::redraw();
}

int Dock::handle(int event) {
	struct straydata *p=traydata;
	while (p!=0) {
		Fl_Window *w = p->win;
		if (Fl::event_x()>=w->x() && Fl::event_x()<=(w->x() + w->w()) 
			&& Fl::event_y()>=w->y() && Fl::event_y()<=(w->y() + w->h())) {
			int ret = w->handle(event); 
			w->throw_focus();
			return ret; 
		}
		p=p->next;
	}
	return Fl_Group::handle(event);
}
