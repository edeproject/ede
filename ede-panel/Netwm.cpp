#include <string.h>
#include <X11/Xproto.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <edelib/Debug.h>
#include <edelib/List.h>

#include "Netwm.h"

EDELIB_NS_USING(list)

struct NetwmCallbackData {
	NetwmCallback  cb;
	void		   *data;
};

typedef list<NetwmCallbackData> CbList;
typedef list<NetwmCallbackData>::iterator CbListIt;

static CbList callback_list;
static bool input_selected = false;

static Atom _XA_NET_WORKAREA;
static Atom _XA_NET_WM_WINDOW_TYPE;
static Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
static Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
static Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
static Atom _XA_NET_WM_STRUT;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_WM_DESKTOP;
static Atom _XA_NET_WM_NAME;
static Atom _XA_NET_WM_VISIBLE_NAME;
static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLOSE_WINDOW;

static Atom _XA_WM_STATE;
static Atom _XA_NET_WM_STATE;
static Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
static Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;

static Atom _XA_EDE_WM_ACTION;
static Atom _XA_EDE_WM_RESTORE_SIZE;

/* this macro is set in xlib when X-es provides UTF-8 extension (since XFree86 4.0.2) */
#if X_HAVE_UTF8_STRING
static Atom _XA_UTF8_STRING;
#else
# define _XA_UTF8_STRING XA_STRING
#endif

static short atoms_inited = 0;

static void init_atoms_once(void) {
	if(atoms_inited)
		return;

	_XA_NET_WORKAREA = XInternAtom(fl_display, "_NET_WORKAREA", False);
	_XA_NET_WM_WINDOW_TYPE = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);
	_XA_NET_WM_WINDOW_TYPE_DOCK = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	_XA_NET_WM_WINDOW_TYPE_SPLASH = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
	_XA_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	_XA_NET_WM_STRUT = XInternAtom(fl_display, "_NET_WM_STRUT", False);
	_XA_NET_NUMBER_OF_DESKTOPS = XInternAtom(fl_display, "_NET_NUMBER_OF_DESKTOPS", False);
	_XA_NET_CURRENT_DESKTOP = XInternAtom(fl_display, "_NET_CURRENT_DESKTOP", False);
	_XA_NET_DESKTOP_NAMES = XInternAtom(fl_display, "_NET_DESKTOP_NAMES", False);
	_XA_NET_CLIENT_LIST = XInternAtom(fl_display, "_NET_CLIENT_LIST", False);
	_XA_NET_WM_DESKTOP = XInternAtom(fl_display, "_NET_WM_DESKTOP", False);
	_XA_NET_WM_NAME = XInternAtom(fl_display, "_NET_WM_NAME", False);
	_XA_NET_WM_VISIBLE_NAME = XInternAtom(fl_display, "_NET_WM_VISIBLE_NAME", False);
	_XA_NET_ACTIVE_WINDOW = XInternAtom(fl_display, "_NET_ACTIVE_WINDOW", False);
	_XA_NET_CLOSE_WINDOW = XInternAtom(fl_display, "_NET_CLOSE_WINDOW", False);

	_XA_WM_STATE = XInternAtom(fl_display, "WM_STATE", False);
	_XA_NET_WM_STATE = XInternAtom(fl_display, "_NET_WM_STATE", False);
	_XA_NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	_XA_NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

	_XA_EDE_WM_ACTION = XInternAtom(fl_display, "_EDE_WM_ACTION", False);
	_XA_EDE_WM_RESTORE_SIZE = XInternAtom(fl_display, "_EDE_WM_RESTORE_SIZE", False);

#ifdef X_HAVE_UTF8_STRING
	_XA_UTF8_STRING = XInternAtom(fl_display, "UTF8_STRING", False);
#endif

	atoms_inited = 1;
}

static int xevent_handler(int e) {
	if(fl_xevent->type == PropertyNotify) {
		int action = -1;

		/* E_DEBUG("==> %s\n", XGetAtomName(fl_display, fl_xevent->xproperty.atom)); */

		if(fl_xevent->xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS)
			action = NETWM_CHANGED_WORKSPACE_COUNT;
		else if(fl_xevent->xproperty.atom == _XA_NET_DESKTOP_NAMES)
			action = NETWM_CHANGED_WORKSPACE_NAMES;
		else if(fl_xevent->xproperty.atom == _XA_NET_CURRENT_DESKTOP)
			action = NETWM_CHANGED_CURRENT_WORKSPACE;
		else if(fl_xevent->xproperty.atom == _XA_NET_WORKAREA)
			action = NETWM_CHANGED_CURRENT_WORKAREA;
		else if(fl_xevent->xproperty.atom == _XA_NET_ACTIVE_WINDOW)
			action = NETWM_CHANGED_ACTIVE_WINDOW;
		else if(fl_xevent->xproperty.atom == _XA_NET_WM_NAME || fl_xevent->xproperty.atom == XA_WM_NAME)
			action = NETWM_CHANGED_WINDOW_NAME;
		else if(fl_xevent->xproperty.atom == _XA_NET_WM_VISIBLE_NAME)
			action = NETWM_CHANGED_WINDOW_VISIBLE_NAME;
		else if(fl_xevent->xproperty.atom == _XA_NET_WM_DESKTOP)
			action = NETWM_CHANGED_WINDOW_DESKTOP;
		else if(fl_xevent->xproperty.atom == _XA_NET_CLIENT_LIST)
			action = NETWM_CHANGED_WINDOW_LIST;
		else
			action = -1;

		if(action >= 0 && !callback_list.empty()) {
			Window xid = fl_xevent->xproperty.window;

			/* TODO: locking here */
			CbListIt it = callback_list.begin(), it_end = callback_list.end();
			for(; it != it_end; ++it) {
				/* execute callback */
				(*it).cb(action, xid, (*it).data);
			}
		}
	}

	return 0;
}

void netwm_callback_add(NetwmCallback cb, void *data) {
	E_RETURN_IF_FAIL(cb != NULL);

	fl_open_display();
	init_atoms_once();

	/* to catch _XA_NET_CURRENT_DESKTOP and such events */
	if(!input_selected) {
		XSelectInput(fl_display, RootWindow(fl_display, fl_screen), PropertyChangeMask | StructureNotifyMask);
		input_selected = true;
	}

	NetwmCallbackData cb_data;
	cb_data.cb = cb;
	cb_data.data = data;

	callback_list.push_back(cb_data);

	Fl::remove_handler(xevent_handler);
	Fl::add_handler(xevent_handler);
}

void netwm_callback_remove(NetwmCallback cb) {
	if(callback_list.empty())
		return;

	CbListIt it = callback_list.begin(), it_end = callback_list.end();
	while(it != it_end) {
		if(cb == (*it).cb) {
			it = callback_list.erase(it);
		} else {
			/* continue, in case the same callback is put multiple times */
			++it;
		}
	}
}

bool netwm_get_workarea(int& x, int& y, int& w, int &h) {
	init_atoms_once();

	Atom real;
	int  format;
	unsigned long n, extra;
	unsigned char* prop;
	x = y = w = h = 0;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_WORKAREA, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, (unsigned char**)&prop);

	if(status != Success)
		return false;

	CARD32* val = (CARD32*)prop;
	if(val) {
		x = val[0];
		y = val[1];
		w = val[2];
		h = val[3];

		XFree((char*)val);
		return true;
	}

	return false;
}

void netwm_make_me_dock(Fl_Window *win) {
	init_atoms_once();

	XChangeProperty(fl_display, fl_xid(win), _XA_NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, 
			(unsigned char*)&_XA_NET_WM_WINDOW_TYPE_DOCK, sizeof(Atom));
}

void netwm_set_window_strut(Fl_Window *win, int left, int right, int top, int bottom) {
	init_atoms_once();

	CARD32 strut[4] = { left, right, top, bottom };

	XChangeProperty(fl_display, fl_xid(win), _XA_NET_WM_STRUT, XA_CARDINAL, 32, 
			PropModeReplace, (unsigned char*)&strut, sizeof(CARD32) * 4);
}

int netwm_get_workspace_count(void) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_NUMBER_OF_DESKTOPS, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

/* change current workspace */
void netwm_change_workspace(int n) {
	init_atoms_once();

	XEvent xev;
	Window root_win = RootWindow(fl_display, fl_screen);

	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = root_win;
	xev.xclient.display = fl_display;
	xev.xclient.message_type = _XA_NET_CURRENT_DESKTOP;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = (long)n;
	xev.xclient.data.l[1] = CurrentTime;

	XSendEvent (fl_display, root_win, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	XSync(fl_display, True);
}

int netwm_get_current_workspace(void) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_CURRENT_DESKTOP, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

int netwm_get_workspace_names(char**& names) {
	init_atoms_once();

	/* FIXME: add _NET_SUPPORTING_WM_CHECK and _NET_SUPPORTED ??? */
	XTextProperty wnames;
	XGetTextProperty(fl_display, RootWindow(fl_display, fl_screen), &wnames, _XA_NET_DESKTOP_NAMES);

	/* if wm does not understainds _NET_DESKTOP_NAMES this is not set */
	if(!wnames.nitems || !wnames.value)
		return 0;

	int nsz;

	/*
	 * FIXME: Here should as alternative Xutf8TextPropertyToTextList since
	 * many wm's set UTF8_STRING property. Below is XA_STRING and for UTF8_STRING
	 * will fail.
	 */
	if(!XTextPropertyToStringList(&wnames, &names, &nsz)) {
		XFree(wnames.value);
		return 0;
	}

	XFree(wnames.value);
	return nsz;
}

int netwm_get_mapped_windows(Window **windows) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_CLIENT_LIST, 0L, 0x7fffffff, False, XA_WINDOW, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	*windows = (Window*)prop;
	return n;
}

int netwm_get_window_workspace(Window win) {
	E_RETURN_VAL_IF_FAIL(win >= 0, -1);

	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, win, 
			_XA_NET_WM_DESKTOP, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	//int ns = int(*(long*)prop);
	unsigned long desk = (unsigned long)(*(long*)prop);
	XFree(prop);

	/* sticky */
	if(desk == 0xFFFFFFFF || desk == 0xFFFFFFFE)
		return -1;

	return desk;
}

int netwm_manageable_window(Window win) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, win, 
			_XA_NET_WM_WINDOW_TYPE, 0L, sizeof(Atom), False, XA_ATOM, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	Atom type = *(Atom*)prop;
	XFree(prop);

	if(type == _XA_NET_WM_WINDOW_TYPE_DESKTOP || 
	   type == _XA_NET_WM_WINDOW_TYPE_SPLASH  || 
	   type == _XA_NET_WM_WINDOW_TYPE_DOCK)
	{
		return 0;
	}

	return 1;
}

char *netwm_get_window_title(Window win) {
	init_atoms_once();

	XTextProperty xtp;
	char		  *title = NULL, *ret = NULL;

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, win, 
			_XA_NET_WM_NAME, 0L, 0x7fffffff, False, _XA_UTF8_STRING , &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status == Success && prop) {
		title = (char *)prop;
		ret = strdup(title);
		XFree(prop);
	} else {
		/* try WM_NAME */
		if(!XGetWMName(fl_display, win, &xtp))
			return NULL;

		if(xtp.encoding == XA_STRING) {
			ret = strdup((const char*)xtp.value);
		} else {
#if X_HAVE_UTF8_STRING
			char **lst;
			int lst_sz;

			status = Xutf8TextPropertyToTextList(fl_display, &xtp, &lst, &lst_sz);
			if(status == Success && lst_sz > 0) {
				ret = strdup((const char*)*lst);
				XFreeStringList(lst);
			} else {
				ret = strdup((const char*)xtp.value);
			}
#else
			ret = strdup((const char*)tp.value);
#endif
		}

		XFree(xtp.value);
	}

	return ret;
}

Window netwm_get_active_window(void) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_ACTIVE_WINDOW, 0L, sizeof(Atom), False, XA_WINDOW, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return -1;

	int ret = int(*(long*)prop);
	XFree(prop);

	return (Window)ret;
}

void netwm_set_active_window(Window win) {
	init_atoms_once();	

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = win;
	xev.xclient.display = fl_display;
	xev.xclient.message_type = _XA_NET_ACTIVE_WINDOW;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = (long)win;
	xev.xclient.data.l[1] = CurrentTime;

	XSendEvent (fl_display, RootWindow(fl_display, fl_screen), False, 
			SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	XSync(fl_display, True);
}

void netwm_maximize_window(Window win) {
	init_atoms_once();	

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = win;
	xev.xclient.display = fl_display;
	xev.xclient.message_type = _XA_NET_WM_STATE;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 0;
	xev.xclient.data.l[1] = _XA_NET_WM_STATE_MAXIMIZED_HORZ;
	xev.xclient.data.l[2] = _XA_NET_WM_STATE_MAXIMIZED_VERT;

	XSendEvent (fl_display, RootWindow(fl_display, fl_screen), False, 
			SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	XSync(fl_display, True);
}

void netwm_close_window(Window win) {
	init_atoms_once();	

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = win;
	xev.xclient.display = fl_display;
	xev.xclient.message_type = _XA_NET_CLOSE_WINDOW;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = (long)win;
	xev.xclient.data.l[1] = CurrentTime;

	XSendEvent (fl_display, RootWindow(fl_display, fl_screen), False, 
			SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	XSync(fl_display, True);
}

void wm_ede_restore_window(Window win) {
	init_atoms_once();	

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.window = win;
	xev.xclient.display = fl_display;
	xev.xclient.message_type = _XA_EDE_WM_ACTION;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = _XA_EDE_WM_RESTORE_SIZE;
	xev.xclient.data.l[1] = CurrentTime;

	XSendEvent (fl_display, RootWindow(fl_display, fl_screen), False, 
			SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	XSync(fl_display, True);
}

WmStateValue wm_get_window_state(Window win) {
	init_atoms_once();

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop = 0;

	int status = XGetWindowProperty(fl_display, win,
			_XA_WM_STATE, 0L, sizeof(Atom), False, _XA_WM_STATE, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success || !prop)
		return WM_STATE_NONE;

	WmStateValue ret = WmStateValue(*(long*)prop);
	XFree(prop);

	return ret;
}

void wm_set_window_state(Window win, WmStateValue state) {
	E_RETURN_IF_FAIL((int)state > 0);

	if(state == WM_STATE_WITHDRAW) {
		XWithdrawWindow(fl_display, win, fl_screen);
		XSync(fl_display, True);
	} else if(state == WM_STATE_ICONIC) {
		XIconifyWindow(fl_display, win, fl_screen);
		XSync(fl_display, True);
	}
	
	/* nothing for WM_STATE_NORMAL */
}
