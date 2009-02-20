// Frame.cpp

//#define TITLE_H 20
#define TITLE_H title->h()

#include "config.h"
#include "Frame.h"
#include "Windowmanager.h"
#include "Desktop.h"
#include "Winhints.h"
#include "Netwm.h"
#include "Icon.h"
#include "Theme.h"

#include "debug.h"

#include <string.h>
#include <stdio.h>

#include <efltk/fl_draw.h>
#include <efltk/Fl_Tooltip.h>

static const int XEventMask =
	ExposureMask
	|StructureNotifyMask|VisibilityChangeMask
	|KeyPressMask|KeyReleaseMask|KeymapStateMask|FocusChangeMask
	|ButtonPressMask|ButtonReleaseMask
	|EnterWindowMask|LeaveWindowMask
	|PointerMotionMask|SubstructureRedirectMask|SubstructureNotifyMask;

extern bool grab();
extern void grab_cursor(bool grab);

extern Fl_Window* Root;

extern int title_active_color, title_active_color_text;
extern int title_normal_color, title_normal_color_text;

Frame* Frame::active_ = 0;

// Resizes opaque (draws everything), if this set to true
bool Frame::do_opaque = false;
bool Frame::focus_follows_mouse = false;

bool Frame::animate = true;
int Frame::animate_speed = 15;

static int px,py,pw,ph;
static GC invertGc=0;
static void draw_current_rect() { XDrawRectangle(fl_display, RootWindow(fl_display, fl_screen), invertGc, px, py, pw, ph); }
void clear_overlay() { if (pw > 0) { draw_current_rect(); pw = 0; } }
void draw_overlay(int x, int y, int w, int h) {
	if(!invertGc) {
		XGCValues v;
		v.subwindow_mode = IncludeInferiors;
		v.foreground = 0xffffffff;
		v.function = GXxor;
		v.line_width = 2;
		v.graphics_exposures = False;
		int mask = GCForeground|GCSubwindowMode|GCFunction|GCLineWidth|GCGraphicsExposures;
		invertGc = XCreateGC(fl_display, RootWindow(fl_display, fl_screen), mask, &v);
	}
	if (w < 0) {x += w; w = -w;} else if (!w) w = 1;
	if (h < 0) {y += h; h = -h;} else if (!h) h = 1;
	if (pw > 0) {
		if (x==px && y==py && w==pw && h==ph) return;
		draw_current_rect();
	}
	px = x; py = y; pw = w; ph = h;
	draw_current_rect();
}

static void animate(int fx, int fy, int fw, int fh, int tx, int ty, int tw, int th)
{
# undef max
# define max(a,b) (a) > (b) ? (a) : (b)
	double max_steps = max( (tw-fw), (th-fh) );
	double min_steps = max( (fw-tw), (fh-th) );
	double steps = max(max_steps, min_steps);
	steps/=Frame::animate_speed;

	double sx = max( ((double)(fx-tx)/steps), ((double)(tx-fx)/steps) );
	double sy = max( ((double)(fy-ty)/steps), ((double)(ty-fy)/steps) );
	double sw = max( ((double)(fw-tw)/steps), ((double)(tw-fw)/steps) );
	double sh = max( ((double)(fh-th)/steps), ((double)(th-fh)/steps) );

	int xinc = fx < tx ? 1 : -1;
	int yinc = fy < ty ? 1 : -1;
	int winc = fw < tw ? 1 : -1;
	int hinc = fh < th ? 1 : -1;
	double rx=fx,ry=fy,rw=fw,rh=fh;

	XGrabServer(fl_display);

	while(steps-- > 0) {

		rx+=(sx*xinc);
		ry+=(sy*yinc);
		rw+=(sw*winc);
		rh+=(sh*hinc);

		draw_overlay((int)rx, (int)ry, (int)rw, (int)rh);

		Fl::sleep(10);
		XFlush(fl_display);
	}

	clear_overlay();

	XUngrabServer(fl_display);
}

// The constructor is by far the most complex part, as it collects
// all the scattered pieces of information about the window that
// X has and uses them to initialize the structure, position the
// window, and then finally create it.

int dont_set_event_mask = 0; // used by FrameWindow

// "existing" is a pointer to an XWindowAttributes structure that is
// passed for an already-existing window when the window manager is
// starting up.  If so we don't want to alter the state, size, or
// position.  If null than this is a MapRequest of a new window.
Frame::Frame(Window window, XWindowAttributes* existing)
: Fl_Window(0,0),
window_(window),
frame_flags_(0), state_flags_(0), decor_flags_(0), func_flags_(0),
transient_for_xid(None),
transient_for_(0),
revert_to(active_),
colormapWinCount(0)
{
	wintype = TYPE_NORMAL;

	maximized = false;
	offset_x = offset_y = offset_w = offset_h = 0;
	strut_ = 0;

	icon_ = 0;
	mwm_hints = 0;
	wm_hints = 0;
	size_hints = XAllocSizeHints();

	begin();

	title = new Titlebar(0, 0, 100, Titlebar::default_height, _("Untitled"));
	title->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	title->label_color((Fl_Color)title_normal_color_text);
	title->color((Fl_Color)title_normal_color);
	title->label_font(label_font()->bold());

	end();

	bool autoplace = ICCCM::size_hints(this);

	// do this asap so we don't miss any events...
	if(!dont_set_event_mask) {
		XSelectInput(fl_display, window_, VisibilityChangeMask | ColormapChangeMask | PropertyChangeMask | FocusChangeMask );
	}

	XGetTransientForHint(fl_display, window_, &transient_for_xid);

	getColormaps();

	get_protocols();

	MWM::get_hints(this);
	MWM::update_hints(this);

	get_wm_hints();
	update_wm_hints();

	NETWM::get_strut(this);

	if(NETWM::get_window_type(this)) {
		// This call overwrites MWM hints!
		get_funcs_from_type();
	} else if(transient_for_xid) {// && wintype==TYPE_NORMAL && decor_flag(BORDER)) {
		//title->h(15); title->label(0);
		if(decor_flag(BORDER) || decor_flag(THIN_BORDER)) {
			clear_decor_flag(BORDER);
			set_decor_flag(THIN_BORDER);
			wintype = TYPE_DIALOG;
			get_funcs_from_type();
		}
	}

	getLabel();

	{
		XWindowAttributes attr;
		if(existing)
			attr = *existing;
		else {
			// put in some legal values in case XGetWindowAttributes fails:
			attr.x = attr.y = 0;
			attr.width = attr.height = 100;
			attr.colormap = fl_colormap;
			attr.border_width = 0;
			XGetWindowAttributes(fl_display, window, &attr);
		}
		int W=attr.width, H=attr.height;
		ICCCM::get_size(this, W, H);
		resize(attr.x, attr.y, W, H);

		restore_x = attr.x;
		restore_y = attr.y;
		restore_w = attr.width;
		restore_h = attr.height;

		colormap = attr.colormap;
	}

	
	// Get icon & mask:
	icon_ = new Icon(wm_hints);

	switch(getIntProperty(_XA_WM_STATE, _XA_WM_STATE, 0))
	{
	case NormalState:
		state_ = NORMAL; break;
	case IconicState:
		state_ = ICONIC; break;
		// X also defines obsolete values ZoomState and InactiveState
	default:
		if(wm_hints && (wm_hints->flags&StateHint) && wm_hints->initial_state==IconicState)
			state_ = ICONIC;
		else
			state_ = NORMAL;
	}

	fix_transient_for();

	if(transient_for()) {
		DBG("WIN %lx is transient for %lx", window_, transient_for()->window());
		// Get state from parent
		if(state_ == NORMAL)
			state_ = transient_for()->state_;
		// And put it on same desktop
		desktop_ = transient_for()->desktop();

	} else {

		// get the desktop from either NET or GNOME (NET takes preference):
		int p = getDesktop();
		if (p > 0 && p <= 8)
			desktop_ = Desktop::desktop(p);
		DBG("WIN %lx belongs to %d", window_, desktop_?desktop_->number():-2);
	}
	send_desktop();

	// some Motif programs assumme this will force the size to conform :-(
	if(w() < size_hints->min_width || h() < size_hints->min_height) {
		if(w() < size_hints->min_width) w(size_hints->min_width);
		if(h() < size_hints->min_height) h(size_hints->min_height);
		XResizeWindow(fl_display, window_, w(), h());
	}

	updateBorder();

	if (autoplace && !existing && !(transient_for() && (x() || y()))) {
		// autoplacement (stupid version for now)
		x(root->x()+(root->w()-w())/2);
		y(root->y()+(root->h()-h())/2);
		// move it until it does not hide any existing windows:
		const int delta = 20;//TITLE_WIDTH+LEFT;
		for(uint n=0; n<map_order.size(); n++) {
			Frame *f = map_order[n];
			if(f==this) continue;
			if (f->x()+delta > x() && f->y()+delta > y() &&
				f->x()+f->w()-delta < x()+w() && f->y()+f->h()-delta < y()+h()) {
				x(max(x(),f->x()+delta));
				y(max(y(),f->y()+delta));
				f = this;
			}
		}
	}

	// move window so contents and border are visible:
	x(force_x_onscreen(x(), w()));
	y(force_y_onscreen(y(), h()));

	// FLTK thinks its override window, so it doesnt send any X junk to wm
	set_override();
	// Create Fl_X class and xid
	create();

	const int mask =
		CWBitGravity|
		CWBorderPixel|
		CWColormap|
		CWEventMask|
		CWBackPixel|
		CWOverrideRedirect;

	XSetWindowAttributes sattr;
	sattr.bit_gravity = NorthWestGravity;
	sattr.event_mask = XEventMask;
	sattr.colormap = fl_colormap;
	sattr.border_pixel = fl_xpixel(FL_BLACK);
	sattr.override_redirect = 0;
	sattr.background_pixel = fl_xpixel(FL_GRAY);

	XChangeWindowAttributes(fl_display, fl_xid(this), mask, &sattr);

	send_state_property();

	if(!dont_set_event_mask)
		XAddToSaveSet(fl_display, window_);

	if(existing)
		set_state_flag(IGNORE_UNMAP);

	// Reparent window to frames
	XReparentWindow(fl_display, window_, fl_xid(this), offset_x, offset_y);

	XSetWindowBorderWidth(fl_display, window_, 0);
	
	XGrabButton(fl_display, AnyButton, AnyModifier, window_, False,
				ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

	if(state_ == NORMAL) {
		XMapWindow(fl_display, window_);

		if(decor_flag(TITLEBAR)) title->show();
		show();

		// many apps expect this even if window size unchanged
		send_configurenotify();

		if(!existing)
			activate_if_transient();
	}

#ifdef SHAPE
	XShapeSelectInput(fl_display, window_, ShapeNotifyMask);
	if(get_shape()) {
		clear_decor_flag(BORDER|THIN_BORDER);
		XShapeCombineShape(fl_display, fl_xid(this), ShapeBounding,
						   0, 0, window_,
						   ShapeBounding, ShapeSet);
	}
#endif

	map_order.append(this);
	stack_order.append(this);

	WindowManager::update_client_list();
	if(strut()) root->update_workarea(true);
}

extern bool wm_shutdown;

// destructor
// The destructor is called on DestroyNotify, so I don't have to do anything
// to the contained window, which is already been destroyed.
Frame::~Frame()
{

}


void Frame::destroy_frame()
{
	if(shown()) {
		XUnmapWindow(fl_display, fl_xid(this));
		XUnmapWindow(fl_display, window_);
		if(title->shown()) XUnmapWindow(fl_display, fl_xid(title));
	}

	// It is possible for the frame to be destroyed while the menu is
	// popped-up, and the menu will still contain a pointer to it.	To
	// fix this the menu checks the state_ location for a legal and
	// non-withdrawn state value before doing anything.  This should
	// be reliable unless something reallocates the memory and writes
	// a legal state value to this location:
	state_ = UNMAPPED;

	// fix fltk bug:
	Fl::focus_ = 0;

	for(uint n=0; n<map_order.size(); n++) {
		Frame *f = map_order[n];
		if(f->transient_for_ == this) f->transient_for_ = transient_for_;
		if(f->revert_to == this) f->revert_to = revert_to;
	}

	throw_focus(1);

	if(colormapWinCount) {
		XFree((char *)colormapWindows);
		delete[] window_Colormaps;
	}

	if(mwm_hints)  XFree((char *)mwm_hints);
	if(wm_hints)   XFree((char *)wm_hints);
	if(size_hints) XFree((char *)size_hints);

	if(icon_) {
		delete icon_;
		icon_ = 0;
	}

	// remove any pointers to this:
	if(!wm_shutdown) {
		stack_order.remove(this);
		map_order.remove(this);
		WindowManager::update_client_list();
	}

	if(strut_) 
	{
		delete strut_;
		strut_=0;
		root->update_workarea(true);
	}
	if(grab()) grab_cursor(false);

	remove_list.append(this);
	Fl::awake();
}

bool Frame::get_shape()
{
#ifdef SHAPE
	shaped = 0;
	int xws, yws, xbs, ybs;
	unsigned wws, hws, wbs, hbs;
	Bool boundingShaped, clipShaped;
	XShapeQueryExtents(fl_display, window_,
					   &boundingShaped, &xws, &yws, &wws, &hws,
					   &clipShaped, &xbs, &ybs, &wbs, &hbs);
	return (shaped = boundingShaped);
#else
	return false;
#endif
}


void Frame::get_funcs_from_type()
{
	DBG("get_funcs_from_type() = %d", wintype);
	switch(wintype) {
	case TYPE_DESKTOP:
	case TYPE_DOCK:
	case TYPE_SPLASH:
		desktop_ = 0;
		clear_func_flags();
		clear_decor_flags();

		if(wintype == TYPE_DOCK)
			set_frame_flag(SKIP_LIST|SKIP_FOCUS);
		else
			set_frame_flag(SKIP_LIST);
		break;
	case TYPE_TOOLBAR:
	case TYPE_MENU:
		clear_decor_flags();
		set_frame_flag(SKIP_LIST|SKIP_FOCUS|NO_FOCUS);
		break;
	case TYPE_DIALOG:
	case TYPE_UTIL:
		title->h(Titlebar::default_height);
		clear_decor_flag(BORDER);
		set_decor_flag(THIN_BORDER);
		break;

	case TYPE_NORMAL:
	default:
		// Dont do anything for NORMAL and UNKNOWN types...
		break;
	};

	
}

void Frame::settings_changed_all()
{
	for(uint n=0; n<map_order.size(); n++) {
		Frame *f = map_order[n];
		f->settings_changed();
	}
	XFlush(fl_display);
}

void Frame::settings_changed()
{
	if(title && title->shown())
		title->setting_changed();

	if(!desktop() || desktop()==Desktop::current()) {
		if(title && title->shown()) {
			XClearWindow(fl_display, fl_xid(title)); //expose title
			redraw(); //Set damage bits
			draw(); //FORCE DRAW!
		}
	}
}

// modify the passed X & W to a legal horizontal window position

// Note (by V.): Previous code preserved the window dimensions so that the
// window middle was centered on screen
// I believe that window top left corner should remain visible, so that
// user can resize window
int Frame::force_x_onscreen(int X, int W)
{
	if(X<root->x()) X=root->x();
//	if(X+W > root->x()+root->w()) X=root->x()+root->w()-W;

	return X;
}

// modify the passed Y & H to a legal vertical window position:
int Frame::force_y_onscreen(int Y, int H)
{
	if(Y<root->y()) Y=root->y();
//	if(Y+H > root->y()+root->h()) Y=root->y()+root->h()-H;

	return Y;
}



int Frame::getDesktop()
{
	if(wintype == TYPE_DESKTOP ||
	   wintype == TYPE_DOCK ||
	   wintype == TYPE_SPLASH ) {
		set_frame_flag(STICKY);
		return -2;
	}

	int ret=0;
	unsigned long desk;

	desk = getIntProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, 100000, &ret);

	if(ret!=Success || desk==100000) {
		clear_frame_flag(STICKY);
		return Desktop::current_num();
	}

	if(desk==0xFFFFFFFF || desk==0xFFFFFFFE) {
		desktop_ = 0;
		set_frame_flag(STICKY);
		DBG("getDesktop() returns 0xFFFFFFFF");
		return -2;
	}

	if(desk>=0) {
		clear_frame_flag(STICKY);
		desk++;
	}

	DBG("getDesktop() returns %ld", desk);
	return desk;
}

void Frame::getLabel(bool del, Atom from_atom)
{
	char *newname=0;
	if(!del)
	{
		if(from_atom==_XA_NET_WM_NAME || from_atom==0)
			newname = NETWM::get_title(this);

		if((from_atom==XA_WM_NAME||from_atom==0) && !newname)
			newname = ICCCM::get_title(this);

	} else {
		label(0);
	}

	if(newname) {
		// since many window managers print a default label when none is
		// given, many programs send spaces to make a blank label.	Detect
		// this and make it really be blank:
		char* c = newname;
		while(*c == ' ') c++;
		if(!*c) {
			XFree(newname);
			newname = 0;
		}
	}

	if(newname) {
		label(newname);
		XFree(newname);
	}

	title->layout();
	title->redraw();
}


void Frame::getColormaps(void)
{
	if (colormapWinCount) {
		XFree((char *)colormapWindows);
		delete[] window_Colormaps;
	}
	unsigned long n;
	Window* cw = (Window*)getProperty(_XA_WM_COLORMAP_WINDOWS, XA_WINDOW, &n);
	if (cw) {
		colormapWinCount = n;
		colormapWindows = cw;
		window_Colormaps = new Colormap[n];
		for(unsigned int i = 0; i < n; ++i) {
			if (cw[i] == window_) {
				window_Colormaps[i] = colormap;
			} else {
				XWindowAttributes attr;
				XSelectInput(fl_display, cw[i], ColormapChangeMask);
				XGetWindowAttributes(fl_display, cw[i], &attr);
				window_Colormaps[i] = attr.colormap;
			}
		}
	} else {
		colormapWinCount = 0;
	}
}

void Frame::installColormap() const
{
	for (int i = colormapWinCount; i--;)
		if (colormapWindows[i] != window_ && window_Colormaps[i])
			XInstallColormap(fl_display, window_Colormaps[i]);
	if (colormap)
		XInstallColormap(fl_display, colormap);
}



// figure out transient_for(), based on the windows that exist, the
// transient_for and group attributes, etc:
void Frame::fix_transient_for()
{
	Frame* p = 0;
	if(transient_for_xid)
	{
		p = root->find_by_wid(transient_for_xid);

		for(Frame* q = p; q; q = q->transient_for_) {
			if(q == this) {
				p = 0;
				break;
			}
		}
	}
	transient_for_ = p;
}

int Frame::is_transient_for(const Frame* f) const
{
	if(f) {
		for (Frame* p = transient_for(); p!=0; p = p->transient_for())
			if(p == f)
				return 1;
	}
	return 0;
}

// When a program maps or raises a window, this is called.	It guesses
// if this window is in fact a modal window for the currently active
// window and if so transfers the active state to this:
// This also activates new main windows automatically
bool Frame::activate_if_transient()
{
	if(!transient_for() || is_transient_for(active_)) {
		return activate();
	}
	return false;
}



bool Frame::activate()
{
	DBG("Frame::activate()");

	// see if a modal & newer window is up:
	//for(uint n=map_order.size(); n--;)
	for(uint n=map_order.size(); n--;)
	{
		Frame *c = map_order[n];
		if(c->state()!=NORMAL) continue;
		if(c->frame_flag(MODAL) && c->transient_for()==this)
			if(c->activate())
				return true;
	}

	// ignore invisible windows:
	if(state() != NORMAL) return 0;

	// always put in the colormap:
	installColormap();

	// skip windows that don't want focus:
	if(frame_flag(NO_FOCUS)) return false;

	// set this even if we think it already has it, this seems to fix
	// bugs with Motif popups:
	XSetInputFocus(fl_display, window_, RevertToPointerRoot, fl_event_time);

	// Change active window
	if(active_ != this) {
		if(active_) active_->deactivate();
		active_ = this;

		//Update titlebar
		title->color((Fl_Color)title_active_color);
		title->label_color((Fl_Color)title_active_color_text);
		title->redraw();

		XSetInputFocus(fl_display, window_, RevertToPointerRoot, fl_event_time);
		NETWM::set_active_window(window_);

		//And last thing is focus
		if(frame_flag(TAKE_FOCUS_PRT))
			sendMessage(_XA_WM_PROTOCOLS, _XA_WM_TAKE_FOCUS);
	}
	return true;
}

// this private function should only be called by constructor and if
// the window is active():
void Frame::deactivate()
{
	title->color((Fl_Color)title_normal_color);
	title->label_color((Fl_Color)title_normal_color_text);
	title->redraw();
}

// After the XGrabButton, the main loop will get the mouse clicks, and
// it will call here when it gets them:
void Frame::content_click()
{
	activate();
	if(wintype==TYPE_DOCK || wintype==TYPE_DESKTOP) {
		XAllowEvents(fl_display, ReplayPointer, CurrentTime);
		return;
	}
	if(fl_xevent.xbutton.button == 1) raise();

	if(fl_xevent.xbutton.button == 1 && Fl::get_key_state(FL_Alt_L)) {

		int mx = Fl::event_x_root()-x();
		int my = Fl::event_y_root()-y();
		int ny,nx;

		if(!Frame::do_opaque) {
			XGrabServer(fl_display);
		}
		grab_cursor(true);

		bool grabbing=true;
		while(grabbing)
		{
			Fl::wait();

			nx = Fl::event_x_root()-mx;
			ny = Fl::event_y_root()-my;
			handle_move(nx, ny);

			if( !Fl::get_key_state(FL_Alt_L) || !Fl::event_state(FL_BUTTON1) )
				grabbing=false;
		}

		if(!Frame::do_opaque) {
			XUngrabServer(fl_display);
			clear_overlay();
			set_size(nx, ny, w(), h());
		}
		grab_cursor(false);
	}

	XAllowEvents(fl_display, ReplayPointer, CurrentTime);
}

// get rid of the focus by giving it to somebody, if possible,
// but not to workpanel
void Frame::throw_focus(int destructor)
{
	
	if(!active()) return;

	DBG("Frame::throw_focus(%d)", destructor);

	if(!destructor) deactivate();
	active_ = 0;

	bool is_active;
	for(uint i = stack_order.size(); --i;)
	{
		Frame *f = stack_order[i];
		if(f != 0 && f != this && f->window_type() != TYPE_DOCK)
		{
			is_active = f->activate();
			if(is_active) 
				f->raise();
			return;
		}
	}
}


// change the state of the window (this is a private function and
// it ignores the transient-for or desktop information):
void Frame::state(short newstate)
{
	short oldstate = state();
	if (newstate == oldstate) return;
	state_ = newstate;

	DBG("Frame(%s)::state(%d -> %d)", label().c_str(), oldstate, newstate);

	switch (newstate) {

	case UNMAPPED:
		XRemoveFromSaveSet(fl_display, window_);

	case OTHER_DESKTOP:
		set_state_flag(IGNORE_UNMAP);
		XUnmapWindow(fl_display, fl_xid(this));
		XUnmapWindow(fl_display, window_);
		title->hide();
		break;

	case ICONIC:
		set_state_flag(ICONIC);
		XUnmapWindow(fl_display, fl_xid(this));
		XUnmapWindow(fl_display, window_);
		title->hide();
		break;

	case NORMAL: {
		int desktop = getDesktop();
		if(desktop<0 || (desktop==Desktop::current_num()) ) {

			if(oldstate == UNMAPPED) XAddToSaveSet(fl_display, window_);
			XMapWindow(fl_display, window_);
			if(decor_flag(TITLEBAR)) title->show();
			show();
			clear_state_flag(IGNORE_UNMAP);

		} else {

			state_ = OTHER_DESKTOP;

		}
		break;
	}

	default:
		DBG("state() default:");
		if(oldstate == UNMAPPED) {
			XAddToSaveSet(fl_display, window_);
		} else if(oldstate == NORMAL) {
			throw_focus();
			set_state_flag(IGNORE_UNMAP);
			XUnmapWindow(fl_display, fl_xid(this));
			XUnmapWindow(fl_display, window_);
			title->hide();
		} else {  // don't setStateProperty IconicState multiple times
			return;
		}
		break;
	}

	send_state_property();
}

void Frame::send_state_property()
{
	switch(state())
	{
	case UNMAPPED:
		ICCCM::state_withdrawn(this);
		break;
	case NORMAL:
	case OTHER_DESKTOP:
		ICCCM::state_normal(this);
		break;
	default :
		ICCCM::state_iconic(this);
		break;
	}
}

void Frame::send_configurenotify()
{
	ICCCM::configure(this);
}

void Frame::send_desktop()
{
	// sends desktop where window belongs to server
	if(desktop_) {
		setProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, desktop_->number()-1);
		if(desktop_!=Desktop::current() && state_==NORMAL)
			state_=OTHER_DESKTOP;
	} else {
		// means sticky...
		setProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF);
		//hmmm.. GNOME sticky should send also?!?
	}
}

void Frame::raise(Window wid)
{
	if(wid==None) {

		Frame::active_=0;
		for(uint n=0; n<map_order.size(); n++) {
			Frame *f = map_order[n];
			f->deactivate();
		}

	} else {

		for(uint n=0; n<map_order.size(); n++) {
			Frame *f = map_order[n];
			if(wid==f->window()) {
				f->raise();
				f->activate_if_transient();
				return;
			}
		}

	}
}


// Public state modifiers that move all transient_for(this) children
// with the frame and do the desktops right:
void Frame::raise()
{
	DBG("Frame(%s)::raise()", title->label().c_str());

	Frame* newtop = 0;
	int previous_state = state_;

	Frame_List raise_list;
	Frame *f;
	uint n;

	stack_order.remove(this);
	raise_list.append(this);
	if(desktop() && desktop()!=Desktop::current()) state(OTHER_DESKTOP);
	else state(NORMAL);

	for(n=0; n<stack_order.size(); n++) {
		f = stack_order[n];

		if(f==this || f->state()==UNMAPPED) continue;

		if(f->is_transient_for(this)) {
			stack_order.remove(f);
			raise_list.append(f);
		} else
			continue;

		if(f->desktop() && f->desktop()!=Desktop::current())
			f->state(OTHER_DESKTOP);
		else
			f->state(NORMAL);
	}

	for(n=0; n<raise_list.size(); n++) {
		f = raise_list[n];
		stack_order.append(f);
		newtop = f;
	}

	if(stack_order.count()!=map_order.count())
		Fl::fatal(_("EDEWM: Internal bug, when restacking (%d != %d)! Exiting... "), 
				stack_order.count(), map_order.count());

	if(!transient_for() && desktop_ && desktop_ != Desktop::current()) {
		// for main windows we also must move to the current desktop
		desktop(Desktop::current());
	}

	if(previous_state!=NORMAL && newtop->state_==NORMAL)
		newtop->activate_if_transient();

	DBG("Calling restack_windows()");
	root->restack_windows();
	DBG("Calling update_task_list()");
}

void Frame::lower()
{
	DBG("Frame::lower()");

	if(map_order.count()<2) return; // if we have just only one window

	Frame* t = transient_for();
	if(t) t->lower();

	stack_order.remove(this);
	stack_order.prepend(this);

	root->restack_windows();
}


void Frame::iconize()
{
	DBG("Frame::iconize()");
	if(window_type()==TYPE_NORMAL || is_transient_for(this) && state() != UNMAPPED)
	{
		state(ICONIC);
		throw_focus(1);
	}
}

// maximize window
void Frame::maximize()
{
	if(maximized)
		return;

	bool m = true;

	int W=root->w(); 
	int H=root->h();
	W-=offset_w;
	H-=offset_h;

	if(ICCCM::get_size(this, W, H)) 
		m=false;

	restore_x = x();
	restore_y = y();
	restore_w = w();
	restore_h = h();
	
	W+=offset_w;
	H+=offset_h;

	if(Frame::animate) 
		::animate(x(), y(), w(), h(), root->x(), root->y(), root->w(), root->h());

	set_size(root->x(), root->y(), W, H);

	maximized = m;
	redraw();
}

// restore window size
void Frame::restore()
{
	if(!maximized)
		return;

	if(Frame::animate) 
		::animate(x(), y(), w(), h(), restore_x, restore_y, restore_w, restore_h);

	set_size(restore_x, restore_y, restore_w, restore_h);
	maximized = false;
}

void Frame::desktop(Window wid, int desktop)
{
	for(uint n=0; n<map_order.size(); n++) {
		Frame *f = map_order[n];
		if(f->window()==wid) {
			if(desktop==-2) f->desktop(0);
			else f->desktop(Desktop::desktop(desktop));
			return;
		}
	}
}

void Frame::desktop(Desktop* d)
{
	DBG("Frame(%s)::desktop(%d)", label().c_str(), d->number());

	if(d == desktop_) return;

	if(d) clear_frame_flag(STICKY);
	else  set_frame_flag(STICKY);

	// Put all the relatives onto the desktop as well:
	for(uint n=0; n<map_order.size(); n++) {
		Frame *c = map_order[n];

		if(c==this || c->is_transient_for(this)) {
			c->desktop_ = d;
			if(d) {
				c->setProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, d->number()-1);
			} else {
				// means sticky...
				c->setProperty(_XA_NET_WM_DESKTOP, XA_CARDINAL, 0xFFFFFFFF);
			}
			if(!d || d == Desktop::current()) {
				if(c->state() == OTHER_DESKTOP)
					c->state(NORMAL);
			} else {
				if(c->state() == NORMAL)
					c->state(OTHER_DESKTOP);
			}
		}
	}
}



void Frame::handle_move(int &nx, int &ny)
{
	int X = root->x();
	if(nx < X && nx > X-SCREEN_SNAP) {
		int t = X;
		if(t <= x() && t > nx) nx = t;
	}

	int Y = root->y();
	if(ny < Y && ny > Y-SCREEN_SNAP) {
		int t = Y;
		if(t <= y() && t > ny) ny = t;
	}

	int W = root->x()+root->w();
	if (nx+w() > W && nx+w() < W+SCREEN_SNAP) {
		int t = W-w();
		if (t >= x() && t < nx) nx = t;
	}

	int H = root->y()+root->h();
	if (ny+h() > H && ny+h() < H+SCREEN_SNAP) {
		int t = H-h();
		if(t >= y() && t < ny) ny = t;
	}

	if(!Frame::do_opaque) {
		draw_overlay(nx-1, ny-1, w()+2, h()+2);
	} else
		set_size(nx, ny, w(), h());
}

// Resize and/or move the window.  The size is given for the frame, not
// the contents.  This also sets the buttons on/off as needed:
void Frame::set_size(int nx, int ny, int nw, int nh)
{
	maximized = false;

	int dx = nx-x();
	int dy = ny-y();

	if(!dx && !dy && nw == w() && nh == h())
		return;

	x(nx);
	y(ny);

	if(nw != w()) w(nw);
	if(nh != h()) h(nh);

	XMoveWindow(fl_display, fl_xid(this), nx, ny);
	XResizeWindow(fl_display, fl_xid(this), nw, nh);
	XResizeWindow(fl_display, window_, nw-offset_w, nh-offset_h);

	send_configurenotify();
	redraw();
	//XSync(fl_display, False);
}

// Resize the frame to match the current border type:
void Frame::updateBorder()
{

	DBG("updateBorder(): %s", label().c_str());

	int nx = x()+offset_x;
	int ny = y()+offset_y;
	int nw = w()-offset_w;
	int nh = h()-offset_h;

	// Set boxes, according decorations flags
	if(decor_flag(THIN_BORDER) && box()!=FL_UP_BOX)
		box(FL_UP_BOX);
	else if(decor_flag(BORDER) && box()!=FL_THICK_UP_BOX)
		box(FL_THICK_UP_BOX);
	else if(!decor_flag(THIN_BORDER) && !decor_flag(BORDER) && box()!=FL_NO_BOX)
		box(FL_NO_BOX);

	if(!decor_flag(BORDER) && !decor_flag(THIN_BORDER)) {
		offset_x=offset_y=offset_w=offset_h=0;
		if(decor_flag(TITLEBAR)) offset_y += TITLE_H;
	} else {
		offset_x = box()->dx();
		offset_y = box()->dy();
		offset_w = box()->dw();
		offset_h = box()->dh();

		offset_y+=title->h();
		offset_h+=title->h();
	}

	nx -= offset_x;
	ny -= offset_y;
	nw += offset_w;
	nh += offset_h;

	if(x()==nx && y()==ny && w()==nw && h()==nh)
		return;

	x(nx); y(ny);
	w(nw); h(nh);

	redraw();

	if(!shown())
		return; // this is so constructor can call this

	if(!decor_flag(TITLEBAR)) title->hide();
	else title->show();

	// try to make the contents not move while the border changes around it:
	XSetWindowAttributes a;
	a.win_gravity = StaticGravity;
	XChangeWindowAttributes(fl_display, window_, CWWinGravity, &a);

	XMoveResizeWindow(fl_display, fl_xid(this), nx, ny, nw, nh);

	a.win_gravity = NorthWestGravity;
	XChangeWindowAttributes(fl_display, window_, CWWinGravity, &a);

	// fix the window position if the X server didn't do the gravity:
	XMoveWindow(fl_display, window_, offset_x, offset_y);
}



void Frame::close(Window wid)
{
	for(uint n=0; n<map_order.size(); n++) {
		Frame *f = map_order[n];
		if(wid==f->window()) {
			f->close();
			return;
		}
	}
}

void Frame::close()
{
	DBG("Frame::close()");
	if(frame_flag(DELETE_WIN_PRT))
		sendMessage(_XA_WM_PROTOCOLS, _XA_WM_DELETE_WINDOW);
	else
		kill();

}

void Frame::kill()
{
	DBG("Frame::kill()");
	XUnmapWindow(fl_display, fl_xid(this));
	XUnmapWindow(fl_display, window_);
	XUnmapWindow(fl_display, fl_xid(title));

	XKillClient(fl_display, window_);
	destroy_frame();
}


// Drawing code:

void Frame::draw()
{
	if((!decor_flag(BORDER) || !decor_flag(THIN_BORDER)) && !decor_flag(TITLEBAR)) 
		return;
	
	if(!(damage()&FL_DAMAGE_ALL|FL_DAMAGE_EXPOSE) )
		return;
	

	DBG("Frame::draw(%x)", damage());

	if(!Theme::use_theme() || Theme::frame_color()==FL_NO_COLOR)
	{
		if(!decor_flag(BORDER) || !decor_flag(THIN_BORDER))
			draw_frame();
	}
	else
	{
		Fl_Color color = Theme::frame_color();
		fl_color(fl_lighter(color));
		fl_line(0,0,0,h());
		fl_line(0,0,w(),0);

		fl_color(fl_darker(color));
		fl_line(w()-1,0,w()-1,h());
		fl_line(0,h()-1,w(),h()-1);

		fl_color(color);
		fl_rect(1,1,w()-2,h()-2);
		fl_rect(2,2,w()-4,h()-4);
	}
	
	if(decor_flag(TITLEBAR))
	{
		if(active()) { // this here to update colors of the titlebar when are changed
			title->color((Fl_Color)title_active_color);
			title->label_color((Fl_Color)title_active_color_text);
		} else {
			title->color((Fl_Color)title_normal_color);
			title->label_color((Fl_Color)title_normal_color_text);
		}

		// Resize and draw titlebar
		if(decor_flag(BORDER) || decor_flag(THIN_BORDER)) {
			XMoveResizeWindow(fl_display, fl_xid(title), box()->dx(), box()->dy(), w()-box()->dw(), TITLE_H);
			title->resize(box()->dx(), box()->dy(), w()-box()->dw(), TITLE_H);
		} else {
			//XMoveResizeWindow(fl_display, fl_xid(title), 0, 0, w(), TITLE_H);
			title->resize(0, 0, w(), TITLE_H);
		}
		title->layout();
		draw_child(*title);
	}
}


// User interface code:

// This method figures out what way the mouse will resize the window.
// It is used to set the cursor and to actually control what you grab.
// If the window cannot be resized in some direction this should not
// return that direction.
int Frame::mouse_location()
{
	int x = Fl::event_x();
	int y = Fl::event_y();
	int r = 0;
	if(!decor_flag(RESIZE)) return 0;

	if(size_hints->min_height != size_hints->max_height) {
		if(y < RESIZE_EDGE)
			r |= FL_ALIGN_TOP;
		else if(y >= h()-RESIZE_EDGE)
			r |= FL_ALIGN_BOTTOM;
	}
	if(size_hints->min_width != size_hints->max_width) {
		if(x < RESIZE_EDGE)
			r |= FL_ALIGN_LEFT;
		else if (x >= w()-RESIZE_EDGE)
			r |= FL_ALIGN_RIGHT;
	}
	return r;
}

// set the cursor correctly for a return value from mouse_location():
static Frame* previous_frame=0;
void Frame::set_cursor(int r) {
	Fl_Cursor c = FL_CURSOR_ARROW;
	switch (r) {
	case FL_ALIGN_TOP:
	case FL_ALIGN_BOTTOM:
		c = FL_CURSOR_NS;
		break;
	case FL_ALIGN_LEFT:
	case FL_ALIGN_RIGHT:
		c = FL_CURSOR_WE;
		break;
	case FL_ALIGN_LEFT|FL_ALIGN_TOP:
	case FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM:
		c = FL_CURSOR_NWSE;
		break;
	case FL_ALIGN_LEFT|FL_ALIGN_BOTTOM:
	case FL_ALIGN_RIGHT|FL_ALIGN_TOP:
		c = FL_CURSOR_NESW;
		break;
	}
	if(this != previous_frame || c != root->get_cursor()) {
		previous_frame = this;
		if(r<=0)
			root->set_default_cursor();
		else
			root->set_cursor(c, FL_WHITE ,FL_BLACK);
	}
}

#ifdef AUTO_RAISE
// timeout callback to cause autoraise:
void auto_raise(void*) {
  if (Frame::activeFrame() && !Fl::grab() && !Fl::pushed())
	Frame::activeFrame()->raise();
}
#endif

// If cursor is in the contents of a window this is set to that window.
// This is only used to force the cursor to an arrow even though X keeps
// sending mysterious erroneous move events:
static Frame* cursor_inside = 0;

//If true, we send all push, move, drag events to titlebar
bool handle_title = false;

// Handle an fltk event.
int Frame::handle(int e)
{
	static bool grabbed=false;
	static int what, dx, dy, ix, iy, iw, ih;

	switch (e) {

	case FL_SHOW:
	case FL_HIDE:
		return 0; // prevent fltk from messing things up

	case FL_ENTER:
		set_cursor(mouse_location());
		if (Frame::focus_follows_mouse == true) activate(); // AEW
		return 1;

	case FL_LEAVE:
		set_cursor(-1);
		return 1;

	case FL_MOVE:

		// Tooltips...
		if(!handle_title && Fl::event_inside(title->x(), title->y(), title->w(), title->h())) {
			if(!title->tooltip().empty()) {
				tooltip(title->tooltip());
				Fl_Tooltip::enter(this);
			} else
				tooltip("");
			handle_title = true;
		} else if(!Fl::event_inside(title->x(), title->y(), title->w(), title->h()) ) { //Fl::belowmouse()==title) {
			tooltip(0);
			Fl_Tooltip::exit();
			handle_title = false;
		}

		if(Fl::belowmouse() != this || cursor_inside == this) {
			set_cursor(-1);
		} else {
			set_cursor(mouse_location());
		}
		return 1;

	case FL_PUSH:
		if(grabbed) return 1;

		activate();
		raise();

		ix = x(); iy = y(); iw = w(); ih = h();

		what = mouse_location();
		if(Fl::event_button() > 1) what = 0; // middle button does drag

		// FL_MOVE sets this!
		if(handle_title) {
			return title->handle(e);
		}

		dx = Fl::event_x_root()-ix;
		if (what & FL_ALIGN_RIGHT) dx -= iw;
		dy = Fl::event_y_root()-iy;
		if (what & FL_ALIGN_BOTTOM) dy -= ih;

		set_cursor(what);
		if(!decor_flag(RESIZE)) return 0;

		if(!Frame::do_opaque && !grabbed) {
			XGrabServer(fl_display);
			grabbed = true;
		}

		//We need to grab cursor, otherwise it keeps changing while resizing
		grab_cursor(true);

		return 1;

	case FL_DRAG:

		if(handle_title) {
			return title->handle(e);
		}
		if(Fl::event_is_click()) return 1; // don't drag yet

		if(!Fl::event_state(FL_BUTTON1)) return 0;
	case FL_RELEASE:
		if(e==FL_RELEASE) {
			set_cursor(mouse_location());
			if(Fl::event_state(FL_BUTTON1)) return 1;
		}


		// Check if titlebar handles event
		if(handle_title) {
			handle_title = false;
			return title->handle(e);
		}

		//Otherwise it's resize event
		int nx = ix;
		int ny = iy;
		int nw = iw;
		int nh = ih;
		if (what & FL_ALIGN_RIGHT)
			nw = Fl::event_x_root()-dx-nx;
		else if (what & FL_ALIGN_LEFT)
			nw = ix+iw-(Fl::event_x_root()-dx);
		else {
			nx = x();
			nw = w();
		}
		if (what & FL_ALIGN_BOTTOM)
			nh = Fl::event_y_root()-dy-ny;
		else if (what & FL_ALIGN_TOP)
			nh = iy+ih-(Fl::event_y_root()-dy);
		else {
			ny = y();
			nh = h();
		}

		int W=nw, H=nh;
		ICCCM::get_size(this, W, H);
		if(nw!=w()) {
			nw = W;
			nw -= (nw-offset_w)%size_hints->width_inc;
		}
		if(nh!=h()) 
			nh = H;
	

		if(what & FL_ALIGN_LEFT) nx = ix+iw-nw;
		if(what & FL_ALIGN_TOP) ny = iy+ih-nh;

		if(Frame::do_opaque || e==FL_RELEASE) {
			set_size(nx,ny,nw,nh);
			redraw();
		} else {
			draw_overlay(nx,ny,nw,nh);
		}

		if(e==FL_RELEASE) {
			if(grabbed && !Frame::do_opaque) {
				XUngrabServer(fl_display);
				clear_overlay();
				grabbed = false;
			}
			grab_cursor(false);
		}

		return 1;
	}
	
	return 0;
}

// Handle events that fltk did not recognize (mostly ones directed
// at the desktop):

int Frame::handle(const XEvent* ei)
{
	switch (ei->type)
	{
		case VisibilityNotify: return visibility_event(&(ei->xvisibility));
		case ConfigureRequest: return configure_event(&(ei->xconfigurerequest));
		case MapRequest:	   return map_event(&(ei->xmaprequest));
		case UnmapNotify:	   return unmap_event(&(ei->xunmap));
		case DestroyNotify:    return destroy_event(&(ei->xdestroywindow));
		case ReparentNotify:   return reparent_event(&(ei->xreparent));
		case ClientMessage:    return clientmsg_event(&(ei->xclient));
		case ColormapNotify:   return colormap_event(&(ei->xcolormap));
		case PropertyNotify:   return property_event(&(ei->xproperty));
		case EnterNotify: 
		{
			// see if cursor skipped over frame and directly to interior:
			if (ei->xcrossing.detail == NotifyVirtual || ei->xcrossing.detail == NotifyNonlinearVirtual) 
				cursor_inside = this;
			else 
			{
				// cursor is now pointing at frame:
				cursor_inside = 0;
				set_cursor(0);
			}
			return 1;
		}

		case LeaveNotify: 
		{
			if (ei->xcrossing.detail == NotifyInferior) 
			{
				// cursor moved from frame to interior
				cursor_inside = this;
				set_cursor(0);
			}
			return 1;
		}

		default:
#ifdef SHAPE
			if(ei->type == (root->XShapeEventBase + ShapeNotify))
				return shape_event((XShapeEvent *)&ei);
#endif
		break;
	}

	return 0;
}

// X utility routines:

void* getProperty(Window w, Atom a, Atom type, unsigned long *np, int *ret)
{
	Atom realType;
	int format;
	unsigned long n, extra;
	int status;
	uchar *prop=0;
	status = XGetWindowProperty(fl_display, w, a, 0L, 0x7fffffff,
								False, type, &realType,
								&format, &n, &extra, (uchar**)&prop);
	if(ret) *ret = status;
	if (status != Success) return 0;
	if (!prop) { return 0; }
	if (!n) { XFree(prop); return 0; }
	if (np) *np = n;
	return prop;
}

int getIntProperty(Window w, Atom a, Atom type, int deflt, int *ret)
{
	void* prop = getProperty(w, a, type, 0, ret);
	if(!prop) return deflt;
	int r = int(*(long*)prop);
	XFree(prop);
	return r;
}

void setProperty(Window w, Atom a, Atom type, int v)
{
	long prop = v;
	XChangeProperty(fl_display, w, a, type, 32, PropModeReplace, (uchar*)&prop,1);
}

void Frame::setProperty(Atom a, Atom type, int v) const
{
	::setProperty(window_, a, type, v);
}

int Frame::getIntProperty(Atom a, Atom type, int deflt, int *ret) const
{
	return ::getIntProperty(window_, a, type, deflt, ret);
}

void* Frame::getProperty(Atom a, Atom type, unsigned long *np, int *ret) const
{
	return ::getProperty(window_, a, type, np, ret);
}

void Frame::sendMessage(Atom a, Atom l) const
{
	XEvent ev;
	long mask;
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = window_;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = long(l);
	ev.xclient.data.l[1] = long(fl_event_time);
	mask = 0L;
	XSendEvent(fl_display, window_, False, mask, &ev);
}
