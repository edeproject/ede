/*
 * $Id: Windowmanager.cpp 1711 2006-07-25 09:51:50Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Windowmanager.h"
#include "Atoms.h"
#include "Hints.h"
#include "Frame.h"
#include "../exset/exset.h"
#include "Tracers.h"
#include "Sound.h"
#include "debug.h"

#include <efltk/Fl_Config.h>
#include <assert.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

#define WM_CONFIG_FILE  "wmanager.conf"
#define EDE_CONFIG_FILE "ede.conf"


WindowManager* WindowManager::pinstance = NULL;
int x_errors;

/* This is one of the most important part of wm and reflects
 * current design. Wm will try to send all messages to frame itself,
 * after trying to find it's ID in collected list. All further processing
 * is left to that frame. Other messages will process wm, 
 * minimizing spreading events all over the code.
 * I'am not in love with this decision; it's roots are from previous
 * edewm code (the real roots are from icewm).
 *
 * Future major versions will probably have different design.
 */
int wm_event_handler(int e)
{
	if(fl_xevent.type == KeyPress)
		e = FL_KEY;
	if(!e)
	{
		Window window = fl_xevent.xany.window;
		switch(fl_xevent.type)
		{
			case CirculateNotify:
			case CirculateRequest:
			case ConfigureNotify:
			case ConfigureRequest:
			case CreateNotify:
			case GravityNotify:
			/*case MapNotify:*/
			case MapRequest:
			case ReparentNotify:
			case UnmapNotify:
				window = fl_xevent.xmaprequest.window;
				break;
		}

		FrameList::iterator last = WindowManager::instance()->window_list.end();
		for(FrameList::iterator it = WindowManager::instance()->window_list.begin(); it != last; ++it)
		{
			Frame* c = *it;
			if(c->window() == window || fl_xid(c) == window)
			{
				//ELOG("wm_event_handler-> window found (%i), sending a message", i);
				return c->handle(&fl_xevent);
			}
		}

		return WindowManager::instance()->handle(&fl_xevent);
	}
	else
		return WindowManager::instance()->handle(e);
}

int convert_align(int a)
{
	switch(a)
	{
		default:
		case 0: break;
		case 1: return FL_ALIGN_RIGHT;
		case 2: return FL_ALIGN_CENTER;
	}
	return FL_ALIGN_LEFT;
}

int xerror_handler(Display* d, XErrorEvent* e)
{
	if(e->request_code == X_ChangeWindowAttributes && 
			e->error_code == BadAccess &&
			e->resourceid == RootWindow(fl_display, DefaultScreen(fl_display)))
	{
		// force cleaning data
		WindowManager::shutdown();
		Fl::fatal(_("Another window manager is running.  You must exit it before running edewm."));
	}

	x_errors++;

	char buff[128];

	EPRINTF("\n");
	XGetErrorDatabaseText(fl_display, "XlibMessage", "XError", "", buff, 128);
	EPRINTF("%s: ", buff);
	XGetErrorText(fl_display, e->error_code, buff, 128);
	EPRINTF("%s \n", buff);

	XGetErrorDatabaseText(fl_display, "XlibMessage", "MajorCode", "%d", buff, 128);
	EPRINTF("  ");
	EPRINTF(buff, e->request_code);

	sprintf(buff, "%d", e->request_code);
	XGetErrorDatabaseText(fl_display, "XRequest", buff, "%d", buff, 128);
	EPRINTF(" (%s)\n", buff);

	XGetErrorDatabaseText(fl_display, "XlibMessage", "MinorCode", "%d", buff, 128);
	EPRINTF("  ");
	EPRINTF(buff, e->minor_code);
	EPRINTF("  ");
	XGetErrorDatabaseText(fl_display, "XlibMessage", "ResourceID", "%d", buff, 128);
	EPRINTF(buff, e->resourceid);

	EPRINTF("\n");
	EPRINTF("\n");

	return 0;
}

bool ValidateDrawable(Drawable d)
{
	Window w;
	int dummy;
	unsigned int dummy_ui;

	XSync(fl_display, False);
	x_errors = 0;
	XGetGeometry(fl_display, d, &w, &dummy, &dummy, &dummy_ui, &dummy_ui, &dummy_ui, &dummy_ui);
	XSync(fl_display, False);

	bool ret = (x_errors == 0 ? true : false);
	x_errors = 0;

/*
	if(ret != true)
	{
		WindowManager::shutdown();
		assert(ret == true);
	}
*/

	return ret;
}

WindowManager::WindowManager() : Fl_Window(0, 0, Fl::w(), Fl::h()), is_running(false)
{
	box(FL_NO_BOX);
	ELOG("WindowManager constructor");
}

WindowManager::~WindowManager()
{
	ELOG("WindowManager destructor");

	FrameList::iterator last = window_list.end();
	for(FrameList::iterator it = window_list.begin(); it != last; ++it)
	{
		Frame* f = *it;
		delete f;
	}
	window_list.clear();

	sound_system->shutdown();
	delete sound_system;

	delete wm_conf;
	delete hint_stuff;
	delete cur;
}

WindowManager* WindowManager::instance(void)
{
	assert(WindowManager::pinstance != NULL);
	return WindowManager::pinstance;
}

void WindowManager::init(int argc, char** argv)
{
	if(WindowManager::pinstance != NULL)
		return;

	WindowManager::pinstance = new WindowManager();
	WindowManager::pinstance->init_internals();
}

void WindowManager::shutdown(void)
{
	if(WindowManager::pinstance != NULL)
	{
		delete WindowManager::pinstance;
		WindowManager::pinstance = NULL;
	}
}

void WindowManager::init_internals(void)
{
	ELOG("Starting window manager");
	wm_conf = new WindowManagerConfig;

	app_starting = false;

	// defaults, in case world goes down
	wm_conf->title_active_color = fl_rgb(0,0,128);
	wm_conf->title_active_color_text = fl_rgb(255,255,255);
	wm_conf->title_normal_color = fl_rgb(192,192,192);
	wm_conf->title_normal_color_text = fl_rgb(0,0,128);
	wm_conf->title_label_align = FL_ALIGN_LEFT;
	wm_conf->title_height = 20;
	wm_conf->title_box_type = 0;
	wm_conf->frame_do_opaque = false;
	wm_conf->frame_animate = true;
	wm_conf->frame_animate_speed = 15;

	fl_open_display();
	XSetErrorHandler(xerror_handler);
	wm_area.set(0, 0, Fl::w(), Fl::h());

	read_configuration();
	read_xset_configuration();

	//register_protocols();
#ifdef _DEBUG
	InitAtoms(fl_display, atom_map);
	register_events();
#else
	InitAtoms(fl_display);
#endif

	//cur = XCreateFontCursor(fl_display, XC_left_ptr);
	//XDefineCursor(fl_display, RootWindow(fl_display, fl_screen), cur);
	// load cursor
	cur = new CursorHandler;
	cur->load(X_CURSORS);
	cur->set_root_cursor();

	sound_system = new SoundSystem();
	sound_system->init();

	sound_system->add(SOUND_MINIMIZE, "sounds/minimize.ogg");
	sound_system->add(SOUND_MAXIMIZE, "sounds/maximize.ogg");
	sound_system->add(SOUND_CLOSE,    "sounds/close.ogg");
	sound_system->add(SOUND_RESTORE,  "sounds/restore.ogg");
	sound_system->add(SOUND_SHADE,    "sounds/shade.ogg");

	// the world is starting here
	show();
	register_protocols();

	hint_stuff = new Hints;
	hint_stuff->icccm_set_iconsizes(this);

	init_clients();
	Fl::add_handler(wm_event_handler);
	XSync(fl_display, 0);

	is_running = true;
}

// load current visible clients
void WindowManager::init_clients(void)
{
	Frame* frame = 0;
	uint win_num;
	Window w1, w2, *wins;
	XWindowAttributes attr;
	XQueryTree(fl_display, fl_xid(this), &w1, &w2, &wins, &win_num);

	// XXX: excluding root parent !!!
	//for (uint i = 0; i < win_num-1; i++)
	for (uint i = 0; i < win_num; i++)
	{
		XGetWindowAttributes(fl_display, wins[i], &attr);
		if(!attr.override_redirect && attr.map_state == IsViewable) 
		{
			if(!attr.screen)
			{
				ELOG("Screen not as window, skiping...");
				continue;
			}

			if(ValidateDrawable(wins[i]))
				frame = new Frame(wins[i], &attr);
		}
		else
			ELOG("Skipping override_redirect window");
	}
	XFree((void *)wins);
}

// register type messages wm understainds
void WindowManager::register_protocols(void)
{
	ELOG("Loading protocols");
	SetSupported(root_win);	
}

void WindowManager::read_configuration(void)
{
	ELOG("Reading config");
	Fl_Config conf(fl_find_config_file(WM_CONFIG_FILE, 0));
	conf.set_section("TitleBar");
	conf.read("Active color", wm_conf->title_active_color, fl_rgb(0, 0, 128));
	conf.read("Normal color", wm_conf->title_normal_color, fl_rgb(192, 192, 192));
	conf.read("Active color text", wm_conf->title_active_color_text, fl_rgb(255, 255, 255));
	conf.read("Normal color text", wm_conf->title_normal_color_text, fl_rgb(0, 0, 128));
	conf.read("Box type", wm_conf->title_box_type, 0);
	conf.read("Height", wm_conf->title_height, 20);

	int la;
	conf.read("Text align", la, 0);
	wm_conf->title_label_align = convert_align(la);

	conf.set_section("Resize");
	conf.read("Opaque resize", wm_conf->frame_do_opaque, false);
	conf.read("Animate", wm_conf->frame_animate, true);
	conf.read("Animate Speed", wm_conf->frame_animate_speed, 15);

	conf.set_section("Misc");
	conf.read("Use theme", wm_conf->use_theme);

	notify_clients();
}

void WindowManager::read_xset_configuration(void)
{
	Fl_Config conf(fl_find_config_file(EDE_CONFIG_FILE, 1));
	int val1, val2, val3;
	Exset xset;

	conf.set_section("Mouse");
	conf.read("Accel", val1, 4);
	conf.read("Thress",val2, 4);
	xset.set_mouse(val1, val2);

	conf.set_section("Bell");
	conf.read("Volume", val1, 50);
	conf.read("Pitch", val2, 440);
	conf.read("Duration", val3, 200);
	xset.set_bell(val1, val2, val3);

	conf.set_section("Keyboard");
	conf.read("Repeat", val1, 1);
	conf.read("ClickVolume", val2, 50);
	xset.set_keybd(val1, val2);

	conf.set_section("Screen");
	conf.read("Delay", val1, 15);
	conf.read("Pattern",val2, 2);
	xset.set_pattern(val1, val2);

	conf.read("CheckBlank", val1, 1);
	xset.set_check_blank(val1);

	conf.read("Pattern", val1, 2);
	xset.set_blank(val1);
}

void WindowManager::notify_clients(void)
{
#warning "TODO: implement WindowManager::notify_clients()"
}

void WindowManager::show(void)
{
	if(!shown())
	{
		create();
		/* Destroy efltk window, set RootWindow to our
		 * xid and redirect all messages to us, which
		 * will make us a window manager.
		 */
		XDestroyWindow(fl_display, Fl_X::i(this)->xid);
		Fl_X::i(this)->xid = RootWindow(fl_display, fl_screen);
		root_win = RootWindow(fl_display, fl_screen);
		XSelectInput(fl_display, fl_xid(this), 
			SubstructureRedirectMask | 
			SubstructureNotifyMask |
			ColormapChangeMask | 
			PropertyChangeMask |
			ButtonPressMask | 
			ButtonReleaseMask |
			EnterWindowMask | 
			LeaveWindowMask |
			KeyPressMask | 
			KeyReleaseMask | 
			KeymapStateMask);

		ELOG("RootWindow ID set to xid");
		draw();
	}
}

void WindowManager::draw(void)
{
	ELOG("RootWindow draw");
	// redraw root window
	XClearWindow(fl_display, fl_xid(this));
}

void WindowManager::idle(void)
{
	//ELOG("Idle");
	FrameList::iterator last = remove_list.end();
	for(FrameList::iterator it = remove_list.begin(); it != last; ++it)
	{
		Frame* f = *it;
		delete f;
	}
	remove_list.clear();
}

void WindowManager::exit(void)
{
	if(is_running)
		is_running = false;
}

const Cursor WindowManager::root_cursor(void)
{
	assert(cur != NULL);
	return cur->root_cursor();
}

void WindowManager::set_cursor(Frame* f, CursorType t)
{
	assert(f != NULL);
	cur->set_cursor(f, t);
/*
	if(cur->cursor_shape_type() == FLTK_CURSORS)
		cur->set_fltk_cursor(f, t);
	else
		cur->set_x_cursor(f, t);
*/
}

const CursorHandler* WindowManager::cursor_handler(void)
{
	assert(cur != NULL);
	return cur;
}

int WindowManager::handle(int event)
{
	Window window = fl_xevent.xany.window;
	switch(event)
	{
		case FL_PUSH:
		{
			FrameList::iterator last = window_list.end();
			for(FrameList::iterator it = window_list.begin(); it != last; ++it)
			{
				Frame* f = *it;
				if(f->window() == window || fl_xid(f) == window)
				{
					f->content_click();
					return 1;
				}
			}
				
			ELOG("FL_PUSH on root");
			return 0;
		}
		
		case FL_SHORTCUT:
		case FL_KEY:
	
			ELOG("FL_SHORCUT | FL_KEY");
			return 1;
		case FL_MOUSEWHEEL:
			XAllowEvents(fl_display, ReplayPointer, CurrentTime);
			return 0;
	}
	return 0;
}

int WindowManager::handle(XEvent* event)
{
	switch(event->type)
	{
		/* ClientMessage is only used for desktop handling
		 * and startup notifications.
		 */
		case ClientMessage:
		{
			ELOG("ClientMessage in wm");
			if(event->xclient.message_type == _XA_EDE_WM_STARTUP_NOTIFY)
			{
				Atom data = event->xclient.data.l[0]; 
				if(data == _XA_EDE_WM_APP_STARTING)
				{
					app_starting = true;
					cur->set_root_cursor(CURSOR_WAIT);
				}
			}
			return 1;
		}
		case MapRequest:
		{
			ELOG("MapRequest from wm");
			const XMapRequestEvent* e = &(fl_xevent.xmaprequest);

			XWindowAttributes attrs;
			XGetWindowAttributes(fl_display, e->window, &attrs);
			if(!attrs.override_redirect)
			{
				ELOG("--- map from wm ---");
				new Frame(e->window);

				if(app_starting)
				{
					cur->set_root_cursor(CURSOR_DEFAULT);
					app_starting = false;
				}
			}

			return 1;
		}
		case ConfigureRequest:
		{
			ELOG("ConfigureRequest from wm");
			const XConfigureRequestEvent *e = &(fl_xevent.xconfigurerequest);
			XConfigureWindow(fl_display, e->window, e->value_mask&~(CWSibling|CWStackMode),
				(XWindowChanges*)&(e->x));
			return 1;
		}

		default:
			return 0;
	}
	return 0;
}

/* Clear stack_list and window_list for those
 * windows schedulied for removal.
 * Expensive operation, so use it with care.
 */
void WindowManager::update_client_list(void)
{
	bool found = false;

	// first clear aot_list
	FrameList::iterator last = aot_list.end();
	for(FrameList::iterator it = aot_list.begin(); it != last; ++it)
	{
		Frame* f = *it;
		if(f->destroy_scheduled())
		{
			// erase current and let 'it' point to next element
			it = aot_list.erase(it);
			found = true;
		}
	}

	// then clear stack_list
	if(!found)
	{
		last = stack_list.end();
		for(FrameList::iterator it = stack_list.begin(); it != last; ++it)
		{
			Frame* f = *it;
			if(f->destroy_scheduled())
				it = stack_list.erase(it);
		}
	}

	// then window_list
	last = window_list.end();
	for(FrameList::iterator it = window_list.begin(); it != last; ++it)
	{
		Frame* f = *it;
		if(f->destroy_scheduled())
		{
			// TODO: do I need this ???
			remove_list.push_back(*it);
			it = window_list.erase(it);
		}
	}
}

/* Used by frames.
 * Window manager will check position of last client and
 * return possible next one
 *
 * Accepted values are: current frame width and height, and
 * returned is position where it should be placed.
 */
bool WindowManager::query_best_position(int* x, int* y, int w, int h)
{
	const int offset = 20;

	if(window_list.size() <= 0)
		return false;

	//Frame* f = window_list[window_list.size()-1];
	Frame* f = *(--window_list.end());
	if(!f)
		return false;

	*x = f->x() + offset;
	*y = f->y() + offset;

	// if w-h of frame are larger than area
	// place them to apropriate corners
	if((*x + w) > wm_area.w())
		*x = wm_area.x();
	if((*y + h) > wm_area.h())
		*y = wm_area.y();

	return true;
}

Frame* WindowManager::find_xid(Window win)
{
	FrameList::iterator last = window_list.end();
	for(FrameList::iterator it = window_list.begin(); it != last; ++it)
	{
		Frame* f = *it;
		if(f->window() == win)
			return f;
	}

	return 0;
}

void WindowManager::restack_windows(void)
{
	TRACE_FUNCTION("void WindowManager::restack_windows(void)");

	Window* stack = new Window[aot_list.size() + stack_list.size()];
	FrameList::iterator it = aot_list.begin();
	FrameList::iterator last = aot_list.end();
	unsigned int i = 0;

	for(; it != last && i < aot_list.size(); ++it, i++)
		stack[i] = fl_xid(*it);

	it = stack_list.begin();
	last = stack_list.end();


	for(; it != last && i < aot_list.size() + stack_list.size(); ++it, i++)
		stack[i] = fl_xid(*it);

	XRestackWindows(fl_display, stack, stack_list.size());
	delete [] stack;
}

void WindowManager::clear_focus_windows(void)
{
	if(aot_list.size() > 0)
	{
		FrameList::iterator it = aot_list.begin();
		FrameList::iterator last = aot_list.end();
		for(; it != last; ++it)
			(*it)->unfocus();
	}

	FrameList::iterator it = stack_list.begin();
	FrameList::iterator last = stack_list.end();
	for(; it != last; ++it)
		(*it)->unfocus();
}

void WindowManager::play_sound(short event)
{
	assert(sound_system != NULL);
	sound_system->play(event);
}


#ifdef _DEBUG
void WindowManager::register_events(void)
{
	xevent_map[CirculateNotify]  = "CirculateNotify";
	xevent_map[CirculateRequest] = "CirculateRequest";
	xevent_map[ConfigureNotify]  = "ConfigureNotify";
	xevent_map[ConfigureRequest] = "ConfigureRequest";
	xevent_map[CreateNotify]     = "CreateNotify";
	xevent_map[GravityNotify]    = "GravityNotify";
	xevent_map[MapNotify]        = "MapNotify";
	xevent_map[MapRequest]       = "MapRequest";
	xevent_map[ReparentNotify]   = "ReparentNotify";
	xevent_map[UnmapNotify]      = "UnmapNotify";
	xevent_map[DestroyNotify]    = "DestroyNotify";
	xevent_map[PropertyNotify]   = "PropertyNotify";
	xevent_map[EnterNotify]      = "EnterNotify";
	xevent_map[LeaveNotify]      = "LeaveNotify";
	xevent_map[VisibilityNotify] = "VisibilityNotify";
	xevent_map[FocusIn]          = "FocusIn";
	xevent_map[FocusOut]         = "FocusOut";
	xevent_map[ClientMessage]    = "ClientMessage";

	efltkevent_map[FL_PUSH]      = "FL_PUSH";
	efltkevent_map[FL_RELEASE]   = "FL_RELEASE";
	efltkevent_map[FL_ENTER]     = "FL_ENTER";
	efltkevent_map[FL_LEAVE]     = "FL_LEAVE";
	efltkevent_map[FL_DRAG]      = "FL_DRAG";
	efltkevent_map[FL_FOCUS]     = "FL_FOCUS";
	efltkevent_map[FL_UNFOCUS]   = "FL_UNFOCUS";
	efltkevent_map[FL_KEY]       = "FL_KEY";
	efltkevent_map[FL_KEYUP]     = "FL_KEYUP";
	efltkevent_map[FL_MOVE]      = "FL_MOVE";
	efltkevent_map[FL_SHORTCUT]  = "FL_SHORTCUT";
	efltkevent_map[FL_ACTIVATE]  = "FL_ACTIVATE";
	efltkevent_map[FL_DEACTIVATE]= "FL_DEACTIVATE";
	efltkevent_map[FL_SHOW]      = "FL_SHOW";
	efltkevent_map[FL_HIDE]      = "FL_HIDE";
	efltkevent_map[FL_MOUSEWHEEL]= "FL_MOUSEWHEEL";
	efltkevent_map[FL_PASTE]     = "FL_PASTE";
	efltkevent_map[FL_DND_ENTER] = "FL_DND_ENTER";
	efltkevent_map[FL_DND_DRAG]  = "FL_DND_DRAG";
	efltkevent_map[FL_DND_LEAVE] = "FL_DND_LEAVE";
	efltkevent_map[FL_DND_RELEASE] = "FL_DND_RELEASE";
}
#endif
