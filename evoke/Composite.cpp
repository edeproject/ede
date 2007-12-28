/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Based on xcompmgr (c) 2003 Keith Packard.
 * Copyright (c) 2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Composite.h"
#include "ClassHack.h"

#include <edelib/Debug.h>

#include <FL/x.h>
#include <FL/Fl.h>

#include <string.h> // memcpy
#include <stdlib.h> // malloc, realloc

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#define TRANSLUCENT 0xe0000000
#define OPAQUE      0xffffffff

#define REFRESH_TIMEOUT 0.05

#define WIN_MODE_SOLID 0
#define WIN_MODE_TRANS 1
#define WIN_MODE_ARGB  2

static int render_error;
static int damage_error;
static int xfixes_error;
static bool have_name_pixmap = false;
static int composite_opcode;
static int damage_event;
static bool another_is_running;


Atom _XA_NET_WM_WINDOW_OPACITY;
Atom _XA_NET_WM_WINDOW_TYPE;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
Atom _XA_NET_WM_WINDOW_TYPE_MENU;
Atom _XA_NET_WM_WINDOW_TYPE_UTILITY;
Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;
Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;

static char* backgroundProps[] = {
	"_XROOTPMAP_ID",
	"_XSETROOT_ID",
	0
};

//#define HAVE_NAME_WINDOW_PIXMAP 1

struct CWindow {
	Window id;
#if HAVE_NAME_WINDOW_PIXMAP
	Pixmap pixmap;
#endif
	XWindowAttributes attr;
	int mode;
	int damaged;
	Damage damage;

	Picture picture;
	Picture picture_alpha;
	Picture picture_shadow;

	XserverRegion border_size;
	XserverRegion extents;

	Picture shadow;
	int shadow_dx;
	int shadow_dy;
	int shadow_w;
	int shadow_h;
	unsigned int opacity;

	Atom window_type;

	unsigned long damage_sequence; // sequence when damage was created

	// for drawing translucent windows
	XserverRegion border_clip;
};

XserverRegion allDamage;
bool fadeWindows = true;
bool clipChanged = false;
bool excludeDockShadows = true;
Picture rootBuffer = 0;
Picture rootPicture = 0;
Picture rootTile = 0;
Picture blackPicture = 0;

int shadowRadius = 12;
int shadowOffsetX = -15;
int shadowOffsetY = -15;
double shadowOpacity = .75;

bool hasNamePixmap = true;

Composite* global_composite;

void idle_cb(void* c) {
	Composite* comp = (Composite*)c;

	comp->paint_all(allDamage);

	allDamage = None;
	clipChanged = false;

	Fl::repeat_timeout(REFRESH_TIMEOUT, idle_cb, c);
}

int xerror_handler(Display* display, XErrorEvent* xev) {
	if(xev->request_code == composite_opcode && xev->minor_code == X_CompositeRedirectSubwindows) {
		EWARNING(ESTRLOC ": Another composite manager is running\n");
		another_is_running = true;
		return 1;
	}

	char* name = NULL;
	int o = xev->error_code - xfixes_error;
	switch(o) {
		case BadRegion:
			name = "BadRegion";
			break;
		default:
			break;
	}

	o = xev->error_code - damage_error;
	switch(o) {
		case BadDamage:
			name = "BadDamage";
			break;
		default:
			break;
	}

	o = xev->error_code - render_error;
	switch(o) {
		case BadPictFormat:
			name = "BadPictFormat";
			break;
		case BadPicture:
			name = "BadPicture";
			break;
		case BadPictOp:
			name = "BadPictOp";
			break;
		case BadGlyphSet:
			name = "BadGlyphSet";
			break;
		case BadGlyph:
			name = "BadGlyph";
			break;
		default:
			break;
	}
	
	EDEBUG(ESTRLOC ": (%s) : error %i request %i minor %i serial %i\n", 
			(name ? name : "unknown"), xev->error_code, xev->request_code, xev->minor_code, xev->serial);

#if 0
	char buff[128];
	XGetErrorDatabaseText(fl_display, "XlibMessage", "XError", "", buff, 128);
	EDEBUG("%s", buff);

	XGetErrorText(fl_display, xev->error_code, buff, 128);
	EDEBUG(" :%s\n", buff);

	XGetErrorDatabaseText(fl_display, "XlibMessage", "MajorCode", "%d", buff, 128);
	EDEBUG(" ");
	EDEBUG(buff, xev->request_code);
	EDEBUG("\n");

	XGetErrorDatabaseText(fl_display, "XlibMessage", "MinorCode", "%d", buff, 128);
	EDEBUG(" ");
	EDEBUG(buff, xev->minor_code);
	EDEBUG("\n");

	XGetErrorDatabaseText(fl_display, "XlibMessage", "ResourceID", "%d", buff, 128);
	EDEBUG(" ");
	EDEBUG(buff, xev->resourceid);
	EDEBUG("\n");
#endif

	return 0;
}

void set_ignore(unsigned long sequence) {
	// TODO
}

unsigned int get_opacity_property(CWindow* win, unsigned int dflt) {
	Atom actual;
	int format;
	unsigned long n, left;
	unsigned char* data = NULL;

	int ret = XGetWindowProperty(fl_display, win->id, _XA_NET_WM_WINDOW_OPACITY, 0L, 1L, False,
			XA_CARDINAL, &actual, &format, &n, &left, &data);

	if(ret == Success && data != NULL) {
		unsigned int p;
		// TODO: replace memcpy call
		memcpy(&p, data, sizeof(unsigned int));
		XFree(data);
		EDEBUG(":) Opacity for %i = %i\n", win->id, p);
		return p;
	}

	EDEBUG("Opacity for %i = %i\n", win->id, dflt);

	return dflt;
}

double get_opacity_percent(CWindow* win, double dflt) {
	unsigned int opacity = get_opacity_property(win, (unsigned int)(OPAQUE * dflt));
	return opacity * 1.0 / OPAQUE;
}

const char* get_window_label(Window win) {
	XTextProperty tp;
	if(XGetWMName(fl_display, win, &tp) != 0)
		return (const char*)tp.value;
	else
		return "(none)";
}

Atom get_window_type_property(Window win) {
	Atom actual;
	int format;
	unsigned long n, left;
	unsigned char* data = NULL;

	int ret = XGetWindowProperty(fl_display, win, _XA_NET_WM_WINDOW_TYPE, 0L, 1L, False,
			XA_ATOM, &actual, &format, &n, &left, &data);

	if(ret == Success && data != NULL) {
		Atom a;
		// TODO: replace memcpy call
		memcpy(&a, data, sizeof(Atom));
		XFree(data);
		return a;
	}

	return _XA_NET_WM_WINDOW_TYPE_NORMAL;
}

Atom determine_window_type(Window win) {
	Atom type = get_window_type_property(win);
	if(type != _XA_NET_WM_WINDOW_TYPE_NORMAL)
		return type;

	// scan children
	Window root_return, parent_return;
	Window* children = NULL;
	unsigned int nchildren;
	
	if(!XQueryTree(fl_display, win, &root_return, &parent_return, &children, &nchildren)) {
		if(children)
			XFree(children);
		return _XA_NET_WM_WINDOW_TYPE_NORMAL;
	}

	for(unsigned int i = 0; i < nchildren; i++) {
		type = determine_window_type(children[i]);
		if(type != _XA_NET_WM_WINDOW_TYPE_NORMAL)
			return type;
	}

	if(children)
		XFree(children);
	return _XA_NET_WM_WINDOW_TYPE_NORMAL;
}

void add_damage(XserverRegion damage) {
	if(allDamage) {
		XFixesUnionRegion(fl_display, allDamage, allDamage, damage);
		XFixesDestroyRegion(fl_display, damage);
	} else 
		allDamage = damage;
}

void determine_mode(CWindow* win) {
	if(win->picture_alpha) {
		XRenderFreePicture(fl_display, win->picture_alpha);
		win->picture_alpha = None;
	}

	if(win->picture_shadow) {
		XRenderFreePicture(fl_display, win->picture_shadow);
		win->picture_shadow = None;
	}

	XRenderPictFormat* format;
	if(get_attributes_class_hack(&win->attr) == InputOnly)
		format = 0;
	else
		format = XRenderFindVisualFormat(fl_display, win->attr.visual);

	if(format && format->type == PictTypeDirect && format->direct.alphaMask)
		win->mode = WIN_MODE_ARGB;
	else if(win->opacity != OPAQUE)
		win->mode = WIN_MODE_TRANS;
	else
		win->mode = WIN_MODE_SOLID;

	if(win->extents) {
		XserverRegion damage = XFixesCreateRegion(fl_display, 0, 0);
		XFixesCopyRegion(fl_display, damage, win->extents);
		add_damage(damage);
	}
}

// TODO: make this a member
XserverRegion set_border_size(CWindow* win) {
    /*
     * if window doesn't exist anymore,  this will generate an error
     * as well as not generate a region.  Perhaps a better XFixes
     * architecture would be to have a request that copies instead
     * of creates, that way you'd just end up with an empty region
     * instead of an invalid XID.
     */
    set_ignore(NextRequest(fl_display));
    XserverRegion border = XFixesCreateRegionFromWindow(fl_display, win->id, WindowRegionBounding);

    // translate this
    set_ignore(NextRequest(fl_display));
    XFixesTranslateRegion(fl_display, border, 
			win->attr.x + win->attr.border_width, 
			win->attr.y + win->attr.border_width);

    return border;
}

// TODO: make this a part of paint_root()
Picture root_tile(void) {
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char* prop;
	Pixmap pixmap = None;
	Atom pixmap_atom = XInternAtom(fl_display, "PIXMAP", False);
	Window root = RootWindow(fl_display, fl_screen);
	bool fill;

	for(int i = 0; backgroundProps[i]; i++) {
		if(XGetWindowProperty(fl_display, root, XInternAtom(fl_display, backgroundProps[i], False),
					0, 4, False, AnyPropertyType,
					&actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success &&
				actual_type == pixmap_atom &&
				actual_format == 32 &&
				nitems == 1) {
			memcpy(&pixmap, prop, 4);
			XFree(prop);
			fill = false;
			break;
		}
	}

	if(!pixmap) {
		pixmap = XCreatePixmap(fl_display, root, 1, 1, DefaultDepth(fl_display, fl_screen));
		fill = true;
	}

	XRenderPictureAttributes pa;
	pa.repeat = True;

	Picture picture = XRenderCreatePicture(fl_display, pixmap, 
			XRenderFindVisualFormat(fl_display, DefaultVisual(fl_display, fl_screen)), 
			CPRepeat, &pa);

	if(fill) {
		XRenderColor c;
		c.red = c.green = c.blue = 0x8080;
		c.alpha = 0xffff;
		XRenderFillRectangle(fl_display, PictOpSrc, picture, &c, 0, 0, 1, 1);
	}

	return picture;
}

Picture solid_picture(bool argb, double a, double r, double g, double b) {
	Pixmap pixmap = XCreatePixmap(fl_display, RootWindow(fl_display, fl_screen), 1, 1, argb ? 32 : 8);
	if(!pixmap)
		return None;

	XRenderPictureAttributes pa;
	pa.repeat = True;
	
	Picture picture = XRenderCreatePicture(fl_display, pixmap, 
			XRenderFindStandardFormat(fl_display, argb ? PictStandardARGB32 : PictStandardA8),
			CPRepeat, &pa);

	if(!picture) {
		XFreePixmap(fl_display, pixmap);
		return None;
	}

	XRenderColor color;
	color.alpha = a * 0xffff;
	color.red = r * 0xffff;
	color.green = g * 0xffff;
	color.blue = b * 0xffff;

	XRenderFillRectangle(fl_display, PictOpSrc, picture, &color, 0, 0, 1, 1);
	XFreePixmap(fl_display, pixmap);
	return picture;
}


Composite::Composite() : manual_redirect(true) {
}

Composite::~Composite() {
	EDEBUG("Composite::~Composite()\n");
}

bool Composite::init(void) {
	another_is_running = false;

	global_composite = this;

	// set error handler first
	XSetErrorHandler(xerror_handler);

	int render_event;
	if(!XRenderQueryExtension(fl_display, &render_event, &render_error)) {
		EWARNING(ESTRLOC ": No render extension\n");
		return false;
	}

	// check Composite extension
	int composite_event, composite_error;
	if(!XQueryExtension(fl_display, COMPOSITE_NAME, &composite_opcode, &composite_event, &composite_error)) {
		EWARNING(ESTRLOC ": No composite extension\n");
		return false;
	}

	int composite_major, composite_minor;
	XCompositeQueryVersion(fl_display, &composite_major, &composite_minor);
	EDEBUG(ESTRLOC ": Setting up XComposite version %i.%i\n", composite_major, composite_minor);

	if(composite_major > 0 || composite_minor >= 2)
		have_name_pixmap = true;

	// check XDamage extension
	if(!XDamageQueryExtension(fl_display, &damage_event, &damage_error)) {
		EWARNING(ESTRLOC ": No damage extension\n");
		return false;
	}

	// check XFixes extension
	int xfixes_event;
	if(!XFixesQueryExtension(fl_display, &xfixes_event, &xfixes_error)) {
		EWARNING(ESTRLOC ": No XFixes extension\n");
		return false;
	}

	_XA_NET_WM_WINDOW_OPACITY = XInternAtom(fl_display, "_NET_WM_WINDOW_OPACITY", False);
	_XA_NET_WM_WINDOW_TYPE = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);
	_XA_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	_XA_NET_WM_WINDOW_TYPE_DOCK = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	_XA_NET_WM_WINDOW_TYPE_TOOLBAR = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
	_XA_NET_WM_WINDOW_TYPE_MENU = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_MENU", False);
	_XA_NET_WM_WINDOW_TYPE_UTILITY = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_UTILITY", False);
	_XA_NET_WM_WINDOW_TYPE_SPLASH = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
	_XA_NET_WM_WINDOW_TYPE_DIALOG = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	_XA_NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_NORMAL", False);

	allDamage = None;
	clipChanged = true;

	XRenderPictureAttributes pa;
	pa.subwindow_mode = IncludeInferiors;
	rootPicture = XRenderCreatePicture(fl_display, RootWindow(fl_display, fl_screen),
			XRenderFindVisualFormat(fl_display, DefaultVisual(fl_display, fl_screen)), CPSubwindowMode, &pa);

	blackPicture = solid_picture(true, 1, 0, 0, 0);

	// now redirect all windows to the Composite
	XGrabServer(fl_display);

	/*
	 * If another manager is running, XCompositeRedirectSubwindows() will report it in xerror_handler().
	 * This only works if another-manager/evoke are both set in CompositeRedirectManual mode
	 */
	if(!manual_redirect)
		XCompositeRedirectSubwindows(fl_display, RootWindow(fl_display, fl_screen), CompositeRedirectAutomatic);
	else {
		XCompositeRedirectSubwindows(fl_display, RootWindow(fl_display, fl_screen), CompositeRedirectManual);
#if 0
		// Commented, since already selected in main evoke code (evoke.cpp).
		XSelectInput(fl_display, RootWindow(fl_display, fl_screen), 
				SubstructureNotifyMask | ExposureMask | StructureNotifyMask | PropertyChangeMask);
#endif

		Window root_ret, parent_ret;
		Window* children;
		unsigned int nchildren;

		XQueryTree(fl_display, RootWindow(fl_display, fl_screen), &root_ret, &parent_ret, &children, &nchildren);
		for(unsigned int i = 0; i < nchildren; i++)
			add_window(children[i], i ? children[i-1] : None);

		XFree(children);
	}

	XUngrabServer(fl_display);

	/* 
	 * draw all windows now, but only if we manualy manage them
	 * since window_list will be empty
	 */
	if(manual_redirect) {
		paint_all(None); // XXX: probably not needed since will be called in idle_cb()
		Fl::add_timeout(REFRESH_TIMEOUT, idle_cb, this);
	}

	return true;
}

void Composite::add_window(Window id, Window previous) {
	CWindow* new_win = new CWindow;
	new_win->id = id;

	set_ignore(NextRequest(fl_display));

	if(!XGetWindowAttributes(fl_display, id, &new_win->attr)) {
		delete new_win;
		return;
	}

	new_win->damaged = 0;
#if HAVE_NAME_WINDOW_PIXMAP
	new_win->pixmap = None;
#endif
	new_win->picture = None;

	if(get_attributes_class_hack(&(new_win->attr)) == InputOnly) {
		new_win->damage_sequence = 0;
		new_win->damage = None;
	} else {
		new_win->damage_sequence = NextRequest(fl_display);
		new_win->damage = XDamageCreate(fl_display, id, XDamageReportNonEmpty);
	}

	new_win->picture_alpha = None;
	new_win->picture_shadow = None;
	new_win->border_size = None;
	new_win->extents = None;
	new_win->shadow = None;
	new_win->shadow_dx = None;
	new_win->shadow_dy = None;
	new_win->shadow_w = None;
	new_win->shadow_h = None;
	new_win->opacity = get_opacity_property(new_win, OPAQUE);
	new_win->border_clip = None;

	new_win->window_type = determine_window_type(new_win->id);
	determine_mode(new_win);

	// put window at the top
	window_list.push_front(new_win);

	// map it if was mapped before
	if(new_win->attr.map_state == IsViewable)
		map_window(new_win->id, new_win->damage_sequence - 1, true);
}

void Composite::map_window(Window id, unsigned long sequence, bool fade) {
	CWindow* win = find_window(id);
	if(!win)
		return;

	win->attr.map_state = IsViewable;

	// this needs to be here or we loose transparency messages
	XSelectInput(fl_display, win->id, PropertyChangeMask);
	win->damaged = 0;

	/* 
	 * XXX: a hack, not present in xcompmgr due main event
	 * loop changes
	 */
	repair_window(win);

	// TODO: fading support
}

void Composite::unmap_window(Window id, bool fade) {
	CWindow* win = find_window(id);
	if(!win)
		return;
	win->attr.map_state = IsUnmapped;
#if HAVE_NAME_WINDOW_PIXMAP
	if(win->pixmap && fade && fadeWindows) {
		// TODO: fading support
	} else
#endif
		finish_unmap_window(win);
}

void Composite::finish_unmap_window(CWindow* win) {
	win->damaged = 0;
	if(win->extents != None) {
		add_damage(win->extents);  // destroy region
		win->extents = None;
	}

#if HAVE_NAME_WINDOW_PIXMAP
	if(win->pixmap) {
		XFreePixmap(fl_display, win->pixmap);
		win->pixmap = None;
	}
#endif

	if(win->picture) {
		set_ignore(NextRequest(fl_display));
		XRenderFreePicture(fl_display, win->picture);
		win->picture = None;
	}

	// don't care about properties anymore
	set_ignore(NextRequest(fl_display));
	XSelectInput(fl_display, win->id, 0);

	if(win->border_size) {
		set_ignore(NextRequest(fl_display));
		XFixesDestroyRegion(fl_display, win->border_size);
		win->border_size = None;
	}

	if(win->shadow) {
		XRenderFreePicture(fl_display, win->shadow);
		win->shadow = None;
	}

	if(win->border_clip) {
		XFixesDestroyRegion(fl_display, win->border_clip);
		win->border_clip = None;
	}

	clipChanged = true;
}

void Composite::configure_window(const XConfigureEvent* ce) {
	CWindow* win = find_window(ce->window);
	if(!win) {
		if(ce->window == RootWindow(fl_display, fl_screen)) {
			if(rootBuffer) {
				XRenderFreePicture(fl_display, rootBuffer);
				rootBuffer = None;
			}

			// TODO: here should be updated
			// root_width and root_height
		}

		return;
	}

	XserverRegion damage = None;

	damage = XFixesCreateRegion(fl_display, 0, 0);
	if(win->extents != None)
		XFixesCopyRegion(fl_display, damage, win->extents);

	win->attr.x = ce->x;
	win->attr.y = ce->y;
	if(win->attr.width != ce->width || win->attr.height != ce->height) {
#if HAVE_NAME_WINDOW_PIXMAP
		if(win->pixmap) {
			XFreePixmap(fl_display, win->pixmap);
			win->pixmap = None;
			if(win->picture) {
				XRenderFreePicture(fl_display, win->picture);
				win->picture = None;
			}
		}
#endif
		if(win->shadow) {
			XRenderFreePicture(fl_display, win->shadow);
			win->shadow = None;
		}
	}

	win->attr.width = ce->width;
	win->attr.height = ce->height;
	win->attr.border_width = ce->border_width;
	win->attr.override_redirect = ce->override_redirect;

#if 0
	EDEBUG("--- before restack() ---\n");
	CWindowListIter first = window_list.begin();
	for(; first != window_list.end(); ++first)
		EDEBUG("0x%x ", (*first)->id);
	EDEBUG("\n");

	EDEBUG("ME: 0x%x ABOVE: 0x%x\n", win->id, ce->above);
#endif
	restack_window(win, ce->above);

#if 0
	EDEBUG("--- after restack() ---\n");
	first = window_list.begin();
	for(; first != window_list.end(); ++first)
		EDEBUG("0x%x ", (*first)->id);
	EDEBUG("\n");
#endif

	if(damage) {
		XserverRegion extents = window_extents(win);
		XFixesUnionRegion(fl_display, damage, damage, extents);
		XFixesDestroyRegion(fl_display, extents);
		add_damage(damage);
	}

	clipChanged = true;
}

void Composite::restack_window(CWindow* win, Window new_above) {
	CWindowListIter it = window_list.begin(), it_end = window_list.end();
	while(it != it_end) {
		if(*it == win)
			break;
		++it;
	}

	Window old_above;
	if(it == it_end) // not found
		old_above = None;
	else {
		++it; // get the next one
		old_above = (*it)->id;
	}

	if(old_above != new_above) {
		--it; // return to our window
		EASSERT(*it == win && "Wrong code");
		window_list.erase(it);
		/*
		it = window_list.begin();
		it_end = window_list.end();

		// find our window and remove it from current position
		while(it != it_end) {
			if(*it == win) {
				window_list.erase(it);
				break;
			}
			++it;
		}*/

		it = window_list.begin();

		// now found new_above and insert our window
		while(it != it_end) {
			if((*it)->id == new_above) {
				break;
			}
			++it;
		}
		// now insert where iterator was stopped (if not found 'new_above') it will insert at the end
		window_list.insert(it, win);
	}
}

void Composite::property_notify(const XPropertyEvent* pe) {
	for(int i = 0; backgroundProps[i]; i++) {
		if(pe->atom == XInternAtom(fl_display, backgroundProps[i], False)) {
			if(rootTile) {
				XClearArea(fl_display, RootWindow(fl_display, fl_screen), 0, 0, 0, 0, True);
				XRenderFreePicture(fl_display, rootTile);
				rootTile = None;
				break;
			}
		}
	}

	// check if Trans property was changed
	if(pe->atom == _XA_NET_WM_WINDOW_OPACITY) {
		// reset mode and redraw window
		CWindow* win = find_window(pe->window);
		if(win) {
			// TODO: fading support
			win->opacity = get_opacity_property(win, OPAQUE);
			determine_mode(win);
			if(win->shadow) {
				XRenderFreePicture(fl_display, win->shadow);
				win->shadow = None;
				win->extents = window_extents(win);
			}
		}
	}
}

#if 0
static XRectangle* expose_rects = 0;
static int size_expose = 0;
static int n_expose = 0;
#endif

void Composite::expose_event(const XExposeEvent* ee) {
#if 0
	if(ee->window != RootWindow(fl_display, fl_screen))
		return;

	int more = ee->count + 1;

	if(n_expose == size_expose) {
		if(expose_rects) {
			expose_rects = (XRectangle*)realloc(expose_rects, (size_expose + more) * sizeof(XRectangle));
			size_expose += more;
		} else {
			expose_rects = (XRectangle*)malloc(more * sizeof(XRectangle));
			size_expose = more;
		}
	}

	expose_rects[n_expose].x = ee->x;
	expose_rects[n_expose].y = ee->y;
	expose_rects[n_expose].width = ee->width;
	expose_rects[n_expose].height = ee->height;
	n_expose++;

	if(ee->count == 0) {
		// expose root
		XserverRegion region = XFixesCreateRegion(fl_display, expose_rects, n_expose);
		add_damage(region);
		n_expose = 0;
	}
#endif

	XRectangle rect[1];
	rect[0].x = ee->x;
	rect[0].y = ee->y;
	rect[0].width = ee->width;
	rect[0].height = ee->height;

	XserverRegion region = XFixesCreateRegion(fl_display, rect, 1);
	add_damage(region);
}

void Composite::reparent_notify(const XReparentEvent* re) {
	if(re->parent == RootWindow(fl_display, fl_screen))
		add_window(re->window, 0);
	else
		destroy_window(re->window, false, true);
}

void Composite::circulate_window(const XCirculateEvent* ce) {
	CWindow* win = find_window(ce->window);
	if(!win)
		return;

	Window new_above;
	if(ce->place == PlaceOnTop)
		new_above = (*window_list.begin())->id;
	else
		new_above = None;
	restack_window(win, new_above);
	clipChanged = true;
}

CWindow* Composite::find_window(Window id) {
	if(window_list.size() == 0)
		return NULL;

	CWindowListIter it = window_list.begin(), it_end = window_list.end();

	while(it != it_end) {
		if((*it)->id == id)
			return *it;
		++it;
	}

	return NULL;
}

XserverRegion Composite::window_extents(CWindow* win) {
	XRectangle r;
	r.x = win->attr.x;
	r.y = win->attr.y;
	r.width = win->attr.width + win->attr.border_width * 2;
	r.height = win->attr.height + win->attr.border_width * 2;

	if(!(win->window_type == _XA_NET_WM_WINDOW_TYPE_DOCK && excludeDockShadows)) {
		// TODO: check this in xcompmgr since server shadows are not used, only client one
		if(win->mode != WIN_MODE_ARGB) {
			win->shadow_dx = shadowOffsetX;
			win->shadow_dx = shadowOffsetX;

			// make shadow if not already made
			if(!win->shadow) {
				double opacity = shadowOpacity;

				if(win->mode == WIN_MODE_TRANS)
					opacity = opacity * ((double)win->opacity)/((double)OPAQUE);
/*
				win->shadow = shadow_picture(opacity, win->picture_alpha, 
						win->attr.width + win->attr.border_width * 2,
						win->attr.height + win->attr.border_width * 2,
						&win->shadow_w, &win->shadow_h);
*/
			}
		}

		XRectangle sr;
		sr.x = win->attr.x + win->shadow_dx;
		sr.y = win->attr.y + win->shadow_dy;
		sr.width = win->shadow_w;
		sr.height = win->shadow_h;

		if(sr.x < r.x) {
			r.width = (r.x + r.width) - sr.x;
			r.x = sr.x;
		}

		if(sr.y < r.y) {
			r.height = (r.y + r.height) - sr.y;
			r.y = sr.y;
		}

		if(sr.x + sr.width > r.x + r.width)
			r.width = sr.x + sr.width - r.x;
		if(sr.y + sr.height > r.y + r.height)
			r.height = sr.y + sr.height - r.y;
	}

	return XFixesCreateRegion(fl_display, &r, 1);
}

void Composite::paint_root(void) {
	if(!rootTile)
		rootTile = root_tile();

	XRenderComposite(fl_display, PictOpSrc, rootTile, None, rootBuffer, 0, 0, 0, 0, 0, 0,
			DisplayWidth(fl_display, fl_screen),
			DisplayHeight(fl_display, fl_screen));
}

void Composite::paint_all(XserverRegion region) {
	//EDEBUG(ESTRLOC ": PAINT %i windows\n", window_list.size());

	// if region is None repaint all screen
	if(!region) {
		XRectangle r;
		r.x = 0;
		r.y = 0;
		// TODO: DisplayWidth/DisplayHeight should be calculated in init()
		r.width = DisplayWidth(fl_display, fl_screen);
		r.height = DisplayHeight(fl_display, fl_screen);
		
		region = XFixesCreateRegion(fl_display, &r, 1);
	}

	if(!rootBuffer) {
		Pixmap root_pix = XCreatePixmap(fl_display, RootWindow(fl_display, fl_screen),
				DisplayWidth(fl_display, fl_screen),
				DisplayHeight(fl_display, fl_screen),
				DefaultDepth(fl_display, fl_screen));

		rootBuffer = XRenderCreatePicture(fl_display, root_pix, 
				XRenderFindVisualFormat(fl_display, DefaultVisual(fl_display, fl_screen)), 0, 0);

		XFreePixmap(fl_display, root_pix);
	}

	XFixesSetPictureClipRegion(fl_display, rootPicture, 0, 0, region);

	// paint from top to bottom
	CWindowListIter it = window_list.begin(), it_end = window_list.end();
	for(; it != it_end; ++it) {
		CWindow* win = *it;

		// never painted, ignore it
		if(!win->damaged)
			continue;

		// invisible, ignore it
		if(win->attr.x + win->attr.width < 1 || win->attr.y + win->attr.height < 1
				|| win->attr.x >= DisplayWidth(fl_display, fl_screen)
				|| win->attr.y >= DisplayHeight(fl_display, fl_screen)) {
			continue;
		}

		if(!win->picture) {
			Drawable draw = win->id;
#ifdef HAVE_NAME_WINDOW_PIXMAP
			if(hasNamePixmap && !win->pixmap)
				win->pixmap = XCompositeNameWindowPixmap(fl_display, win->id);
			if(win->pixmap)
				draw = win->pixmap;
#endif
			XRenderPictureAttributes pa;
			XRenderPictFormat* format = XRenderFindVisualFormat(fl_display, win->attr.visual);

			pa.subwindow_mode = IncludeInferiors;
			win->picture = XRenderCreatePicture(fl_display, draw, format, CPSubwindowMode, &pa);
		}

		if(clipChanged) {
			if(win->border_size) {
				set_ignore(NextRequest(fl_display));
				XFixesDestroyRegion(fl_display, win->border_size);
				win->border_size = None;
			}

			if(win->extents) {
				XFixesDestroyRegion(fl_display, win->extents);
				win->extents = None;
			}

			if(win->border_clip) {
				XFixesDestroyRegion(fl_display, win->border_clip);
				win->border_clip = None;
			}
		}

		if(!win->border_size)
			win->border_size = set_border_size(win);

		if(!win->extents)
			win->extents = window_extents(win);

		// EDEBUG("mode :%i\n", win->mode);

		if(win->mode == WIN_MODE_SOLID) {
			int x, y, wid, hei;
#if HAVE_NAME_WINDOW_PIXMAP
			x = win->attr.x;
			y = win->attr.y;
			wid = win->attr.width + win->attr.border_width * 2;
			hei = win->attr.height + win->attr.border_width * 2;
#else
			x = win->attr.x + win->attr.border_width;
			y = win->attr.y + win->attr.border_width;
			wid = win->attr.width;
			hei = win->attr.height;
#endif
			XFixesSetPictureClipRegion(fl_display, rootBuffer, 0, 0, region);
			set_ignore(NextRequest(fl_display));
			XFixesSubtractRegion(fl_display, region, region, win->border_size);
			set_ignore(NextRequest(fl_display));

			// EDEBUG(ESTRLOC ": Composite on %i x:%i y:%i w:%i h:%i\n", win->id, x, y, wid, hei);

			XRenderComposite(fl_display, PictOpSrc, win->picture, None, rootBuffer,
					0, 0, 0, 0, x, y, wid, hei);
		}

		if(!win->border_clip) {
			win->border_clip = XFixesCreateRegion(fl_display, 0, 0);
			XFixesCopyRegion(fl_display, win->border_clip, region);
		}
	}

	XFixesSetPictureClipRegion(fl_display, rootBuffer, 0, 0, region);
	paint_root();

	/*
	 * paint from bottom to top
	 * use counter since checking only iterator will not pick up first element
	 */
	it = window_list.end();
	it_end = window_list.begin();
	--it; // decrease iterator so it pointing to the last element

	for(unsigned int i = 0; i < window_list.size(); i++) {
		CWindow* win = *it;
		
		XFixesSetPictureClipRegion(fl_display, rootBuffer, 0, 0, win->border_clip);

		// TODO: server shadows
		// go and directly try to draw client shadows
		// don't draw them on desktop
#if 0
		if(win->shadow && win->window_type != _XA_NET_WM_WINDOW_TYPE_DESKTOP) {
			XRenderComposite(fl_display, PictOpOver, blackPicture, win->shadow, rootBuffer,
					0, 0, 0, 0,
					win->attr.x + win->shadow_dx,
					win->attr.y + win->shadow_dy,
					win->shadow_w, win->shadow_h);
		}
#endif

		if(win->opacity != OPAQUE && !win->picture_alpha)
			win->picture_alpha = solid_picture(false, (double)win->opacity / OPAQUE, 0, 0, 0);

		// TODO: check this, xcompmgr have the same code for WIN_MODE_TRANS and WIN_MODE_ARGB
		if(win->mode == WIN_MODE_TRANS || win->mode == WIN_MODE_ARGB) {
			EDEBUG("!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			int x, y, wid, hei;
#if HAVE_NAME_WINDOW_PIXMAP
			x = win->attr.x;
			y = win->attr.y;
			wid = win->attr.width + win->attr.border_width * 2;
			hei = win->attr.height + win->attr.border_width * 2;
#else
			x = win->attr.x + win->attr.border_width;
			y = win->attr.y + win->attr.border_width;
			wid = win->attr.width;
			hei = win->attr.height;
#endif
			set_ignore(NextRequest(fl_display));
			XRenderComposite(fl_display, PictOpOver, win->picture, win->picture_alpha, rootBuffer,
					0, 0, 0, 0,
					x, y, wid, hei);
		}

		// XXX: a lot of errors here ?
		if(win->border_clip != None) {
			XFixesDestroyRegion(fl_display, win->border_clip);
			win->border_clip = None;
		}

		// this will assure we catch first element too
		if(it != it_end)
			--it;
	}

	XFixesDestroyRegion(fl_display, region);

	if(rootBuffer != rootPicture) {
		XFixesSetPictureClipRegion(fl_display, rootBuffer, 0, 0, None);
		XRenderComposite(fl_display, PictOpSrc, rootBuffer, None, rootPicture,
				0, 0, 0, 0, 0, 0,
				DisplayWidth(fl_display, fl_screen),
				DisplayHeight(fl_display, fl_screen));
	}
}

void Composite::damage_window(XDamageNotifyEvent* de) {
	CWindow* win = find_window(de->drawable);
	if(!win)
		return;

	repair_window(win);
}

void Composite::repair_window(CWindow* win) {
	XserverRegion parts;

	EDEBUG("Repairing %i (%i) (area: x:%i y:%i w:%i h:%i)\n", win->id, win->damaged, 
			win->attr.x,
			win->attr.y,
			win->attr.width,
			win->attr.height);

	if(!win->damaged) {
		parts = window_extents(win);
		set_ignore(NextRequest(fl_display));
		XDamageSubtract(fl_display, win->damage, None, None);
	} else {
		parts = XFixesCreateRegion(fl_display, 0, 0);
		set_ignore(NextRequest(fl_display));
		XDamageSubtract(fl_display, win->damage, None, parts);

		XFixesTranslateRegion(fl_display, parts, 
				win->attr.x + win->attr.border_width,
				win->attr.y + win->attr.border_width);

		// TODO: server shadows
	}

	if(parts == None)
		EDEBUG("parts == None\n");

	add_damage(parts);
	win->damaged = 1;
}

void Composite::destroy_window(Window id, bool gone, bool fade) {
#if HAVE_NAME_WINDOW_PIXMAP
	CWindow* win = find_window(id);

	if(win && win->pixmap && fade && fadeWindows) {
		// TODO: fading support
	} else
#endif
		finish_destroy_window(id, gone);
}

void Composite::finish_destroy_window(Window id, bool gone) {
	CWindowListIter it = window_list.begin(), it_end = window_list.end();

	while(it != it_end) {
		if((*it)->id == id) {
			CWindow* win = *it;

			if(!gone)
				finish_unmap_window(win);
			if(win->picture) {
				set_ignore(NextRequest(fl_display));
				XRenderFreePicture(fl_display, win->picture);
				win->picture = None;
			}

			if(win->picture_alpha) {
				XRenderFreePicture(fl_display, win->picture_alpha);
				win->picture_alpha = None;
			}

			if(win->picture_shadow) {
				XRenderFreePicture(fl_display, win->picture_shadow);
				win->picture_shadow = None;
			}

			if(win->damage != None) {
				set_ignore(NextRequest(fl_display));
				XDamageDestroy(fl_display, win->damage);
				win->damage = None;
			}

			// TODO: fading support
			window_list.erase(it);
			delete win;
			break;
		}

		++it;
	}
}

void Composite::handle_xevents(const XEvent* xev) {
	if(another_is_running || !manual_redirect)
		return;

	switch(xev->type) {
		case CreateNotify:
			EDEBUG(ESTRLOC ": CreateNotify from composite\n");
			add_window(xev->xcreatewindow.window, 0);
			break;
		case ConfigureNotify:
			EDEBUG(ESTRLOC ": ConfigureNotify from composite\n");
			configure_window(&xev->xconfigure);
			break;
		case DestroyNotify:
			EDEBUG(ESTRLOC ": DestroyNotify from composite\n");
			destroy_window(xev->xdestroywindow.window, true, true);
			break;
		case MapNotify:
			EDEBUG(ESTRLOC ": MapNotify from composite\n");
			map_window(xev->xmap.window, xev->xmap.serial, true);
			break;
		case UnmapNotify:
			EDEBUG(ESTRLOC ": UnmapNotify from composite\n");
			unmap_window(xev->xunmap.window, true);
			break;
		case ReparentNotify:
			EDEBUG(ESTRLOC ": ReparentNotify from composite\n");
			reparent_notify(&xev->xreparent);
			break;
		case CirculateNotify:
			EDEBUG(ESTRLOC ": CirculateNotify from composite\n");
			circulate_window(&xev->xcirculate);
			break;
		case Expose:
			EDEBUG(ESTRLOC ": Expose from composite\n");
			expose_event(&xev->xexpose);
			break;
		case PropertyNotify:
			EDEBUG(ESTRLOC ": PropertyNotify from composite\n");
			property_notify(&xev->xproperty);
			break;
		default:
			if(xev->type == damage_event + XDamageNotify) {
				EDEBUG("---------> %i <---------\n", damage_event);
				damage_window((XDamageNotifyEvent*)xev);
			}
			break;
	}
}
