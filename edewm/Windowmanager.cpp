#include "Windowmanager.h"
#include "Icccm.h"
#include "Frame.h"
#include "Desktop.h"
#include "Winhints.h"
#include "Theme.h"

#include <X11/Xproto.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../exset/exset.h"
#include "config.h"
#include "debug.h"

WindowManager *root;
Window root_win;

Frame_List remove_list;

Frame_List stack_order;
Frame_List map_order;

////////////////////////////////////////////////////////////////
static int initializing;
Exset xset;

static const char* program_name;

// in Hotkeys.cpp
extern int Handle_Hotkey();
extern void Grab_Hotkeys();
extern void read_hotkeys_configuration();
void read_disp_configuration();

#if DESKTOPS
extern void init_desktops();
#endif

static const char* cfg, *cbg;

Fl_Color title_active_color, title_active_color_text;
Fl_Color title_normal_color, title_normal_color_text;

////////////////////////////////////////////////////////////////

// fltk calls this for any events it does not understand:
static int wm_event_handler(int e)
{
    if(fl_xevent.type == KeyPress) e=FL_KEY;

    // XEvent that fltk did not understand.
    if(!e) {
        Window window = fl_xevent.xany.window;

        // unfortunately most of the redirect events put the interesting
        // window id in a different place:
        switch (fl_xevent.type) {
        case CirculateNotify:
        case CirculateRequest:
        case ConfigureNotify:
        case ConfigureRequest:
        case CreateNotify:
        case GravityNotify:
        case MapNotify:
        case MapRequest:
        case ReparentNotify:
        case UnmapNotify:
            window = fl_xevent.xmaprequest.window;
            break;
        }

        for(uint n=stack_order.size(); n--;) {
            Frame *c = stack_order[n];
            if (c->window() == window || fl_xid(c) == window) {
                return c->handle(&fl_xevent);
            }
        }

        return root->handle(&fl_xevent);
    } else
        return root->handle(e);

    return 0;
}

static int xerror_handler(Display* d, XErrorEvent* e) {
    if(initializing && (e->request_code == X_ChangeWindowAttributes) && e->error_code == BadAccess)
        Fl::fatal(_("Another window manager is running.  You must exit it before running %s."), program_name);

#ifndef DEBUG
    if (e->error_code == BadWindow) return 0;
    if (e->error_code == BadColor) return 0;
#endif

    char buf1[128], buf2[128];
    sprintf(buf1, "XRequest.%d", e->request_code);
    XGetErrorDatabaseText(d,"",buf1,buf1,buf2,128);
    XGetErrorText(d, e->error_code, buf1, 128);
    Fl::warning("%s: %s: %s 0x%lx", program_name, buf2, buf1, e->resourceid);
    return 0;
}

WindowManager::WindowManager(int argc, char *argv[])
: Fl_Window(0, 0, Fl::w(), Fl::h())
{
    root = this;
    xset = new Exset();
    init_wm(argc, argv);

    box(FL_NO_BOX);
}

// consume a switch from argv.  Returns number of words eaten, 0 on error:
int arg(int argc, char **argv, int &i) {
    const char *s = argv[i];
    if (s[0] != '-') return 0;
    s++;

    // do single-word switches:
    if (!strcmp(s,"x")) {
        //exit_flag = 1;
        i++;
        return 1;
    }

    // do switches with a value:
    const char *v = argv[i+1];
    if (i >= argc-1 || !v)
        return 0;	// all the rest need an argument, so if missing it is an error

    if (!strcmp(s, "cfg")) {
        cfg = v;
    } else if (!strcmp(s, "cbg")) {
        cbg = v;
    } else if (*s == 'v') {
        int visid = atoi(v);
        fl_open_display();
        XVisualInfo templt; int num;
        templt.visualid = visid;
        fl_visual = XGetVisualInfo(fl_display, VisualIDMask, &templt, &num);
        if (!fl_visual) Fl::fatal("No visual with id %d",visid);
        fl_colormap = XCreateColormap(fl_display, RootWindow(fl_display,fl_screen),
                                      fl_visual->visual, AllocNone);
    } else
        return 0; // unrecognized
    // return the fact that we consumed 2 switches:
    i += 2;
    return 2;
}

int real_align(int i) {
    switch(i) {
    default:
    case 0: break;
    case 1: return FL_ALIGN_RIGHT;
    case 2: return FL_ALIGN_CENTER;
    }
    return FL_ALIGN_LEFT;
}

void read_configuration()
{
    Fl_String buf;
    
    Fl_Config wmconf(fl_find_config_file("wmanager.conf", 0));

    wmconf.set_section("TitleBar");

    wmconf.read("Active color", title_active_color, fl_rgb(0,0,128));
    wmconf.read("Normal color", title_normal_color, fl_rgb(192,192,192));

    wmconf.read("Active color text", title_active_color_text, fl_rgb(255,255,255));
    wmconf.read("Normal color text", title_normal_color_text, fl_rgb(0,0,128));

    wmconf.read("Box type", Titlebar::box_type, 0);
    wmconf.read("Height", Titlebar::default_height, 20);
    wmconf.read("Text align", Titlebar::label_align, 0);
    Titlebar::label_align = real_align(Titlebar::label_align);

    wmconf.set_section("Resize");
    wmconf.read("Opaque resize", Frame::do_opaque, false);
    wmconf.read("Animate", Frame::animate, true);
    wmconf.read("Animate Speed", Frame::animate_speed, 15);
    
    wmconf.set_section("Misc");
	wmconf.read("FocusFollowsMouse", Frame::focus_follows_mouse, false);

    bool theme = false;
    wmconf.read("Use theme", theme, false);
    if(theme) {
        wmconf.read("Theme path", buf, 0);
        Theme::load_theme(buf);
        Theme::use_theme(true);
    } else {
        Theme::unload_theme();
        Theme::use_theme(false);
    }
    Frame::settings_changed_all();
    
    read_hotkeys_configuration();
}

void do_xset_from_conf()
{
    Fl_Config config(fl_find_config_file("ede.conf",1));

    int val1, val2, val3;

    config.set_section("Mouse");
    config.read("Accel", val1, 4);
    config.read("Thress",val2, 4);
    xset.set_mouse(val1, val2);

    config.set_section("Bell");
    config.read("Volume", val1, 50);
    config.read("Pitch", val2, 440);
    config.read("Duration", val3, 200);
    xset.set_bell(val1, val2, val3);

    config.set_section("Keyboard");
    config.read("Repeat", val1, 1);
    config.read("ClickVolume", val2, 50);
    xset.set_keybd(val1, val2);

    config.set_section("Screen");
    config.read("Delay", val1, 15);
    config.read("Pattern",val2, 2);
    xset.set_pattern(val1, val2);

    config.read("CheckBlank", val1, 1);
    xset.set_check_blank(val1);

    config.read("Pattern", val1, 2);
    xset.set_blank(val1);
}

void WindowManager::init_wm(int argc, char *argv[])
{
    static bool wm_inited = false;
    if(wm_inited) return;

    DBG("init windowmanager");

    fl_open_display();

    XShapeQueryExtension(fl_display, &XShapeEventBase, &XShapeErrorBase);

    wm_area.set(0, 0/*22*/, Fl::w(), Fl::h()/*-22*/);

    program_name = fl_file_filename(argv[0]);
    int i;
    if(Fl::args(argc, argv, i, arg) < argc)
        Fl::error("options are:\n"
                  " -d[isplay] host:#.#\tX display & screen to use\n"
                  " -v[isual] #\t\tvisual to use\n"
                  " -g[eometry] WxH+X+Y\tlimits windows to this area\n"
                  " -x\t\t\tmenu says Exit instead of logout\n"
                  " -bg color\t\tFrame color\n"
                  " -fg color\t\tLabel color\n"
                  " -bg2 color\t\tText field color\n"
                  " -cfg color\t\tCursor color\n"
                  " -cbg color\t\tCursor outline color" );

    // Init started
    initializing = 1;

    XSetErrorHandler(xerror_handler);
    Fl::add_handler(wm_event_handler);

    init_atoms(); // intern atoms

    read_configuration();
    do_xset_from_conf();

    show(); // Set XID now

    set_default_cursor();

    ICCCM::set_iconsizes();
    MWM::set_motif_info();

    register_protocols(fl_xid(this));

    Grab_Hotkeys();

    XSync(fl_display, 0);

    init_desktops();

    //Init done
    initializing = 0;
    wm_inited = true;

    // find all the windows and create a Frame for each:
    Frame *f=0;
    unsigned int n;
    Window w1, w2, *wins;
    XWindowAttributes attr;
    XQueryTree(fl_display, fl_xid(this), &w1, &w2, &wins, &n);
    for (i = 0; i < (int)n; ++i) {
        XGetWindowAttributes(fl_display, wins[i], &attr);
        if(attr.override_redirect) continue;
        if(!attr.map_state) {
            if(getIntProperty(wins[i], _XA_WM_STATE, _XA_WM_STATE, 0) != IconicState)
                continue;
        }
        f = new Frame(wins[i], &attr);
    }
    XFree((void *)wins);

    // Activate last one
    for(uint n=0; n<map_order.size(); n++) {
        Frame *f = map_order[n];
        if(f->desktop()==Desktop::current()) {
            f->activate();
            f->raise();
            break;
        }
    }

    update_workarea(true);
}


void WindowManager::show()
{
    if(!shown()) {
        create();

        // Destroy FLTK window
        XDestroyWindow(fl_display, Fl_X::i(this)->xid);
        // Set RootWindow to our xid
        Fl_X::i(this)->xid = RootWindow(fl_display, fl_screen);
        root_win = RootWindow(fl_display, fl_screen);

        // setting attributes on root window makes it the window manager:
        XSelectInput(fl_display, fl_xid(this),
                     SubstructureRedirectMask | SubstructureNotifyMask |
                     ColormapChangeMask | PropertyChangeMask |
                     ButtonPressMask | ButtonReleaseMask |
                     EnterWindowMask | LeaveWindowMask |
                     KeyPressMask | KeyReleaseMask | KeymapStateMask);

        DBG("RootWindow ID set to xid");

        draw();
    }
}

void WindowManager::draw()
{
    DBG("ROOT DRAW");
    //Redraw root window
    XClearWindow(fl_display, fl_xid(this));
}

extern void set_frame_cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg, Window wid);
void WindowManager::set_default_cursor()
{
    cursor = FL_CURSOR_ARROW;
    set_frame_cursor(FL_CURSOR_ARROW, FL_WHITE, FL_BLACK, root_win);
}

void WindowManager::set_cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg)
{
    cursor = c;
    set_frame_cursor(c, bg, fg, root_win);
}

Frame *WindowManager::find_by_wid(Window wid)
{
    for(uint n=0; n<map_order.size(); n++) {
        Frame *f = map_order[n];
        if(f->window()==wid) return f;
    }
    return 0;
}

void WindowManager::restack_windows()
{
    Window *windows = new Window[1];
    int total=0;

    DBG("Restack: DOCK, SPLASH");
    for(uint n=0; n<stack_order.size(); n++) {
        Frame *f = stack_order[n];
        if(f->window_type()==TYPE_DOCK || f->window_type()==TYPE_SPLASH) {
            windows = (Window*)realloc(windows, (total+1)*sizeof(Window));
            windows[total++] = fl_xid(f);
        }
    }
    DBG("Restack: TOOLBAR, MENU");
    for(uint n=0; n<stack_order.size(); n++) {
        Frame *f = stack_order[n];
        if(f->window_type()==TYPE_TOOLBAR || f->window_type()==TYPE_MENU) {
            windows = (Window*)realloc(windows, (total+1)*sizeof(Window));
            windows[total++] = fl_xid(f);
        }
    }
    DBG("Restack: NORMAL, UTIL, DIALOG");
    for(uint n=stack_order.size(); n--;) {
        Frame *f = stack_order[n];
        if( (f->window_type()==TYPE_NORMAL ||
             f->window_type()==TYPE_UTIL ||
             f->window_type()==TYPE_DIALOG) &&
           f->state()==NORMAL ) {
            windows = (Window*)realloc(windows, (total+1)*sizeof(Window));
            windows[total++] = fl_xid(f);
        }
    }
    DBG("Restack: DESKTOP");
    for(uint n=0; n<stack_order.size(); n++) {
        Frame *f = stack_order[n];
        if(f->window_type()==TYPE_DESKTOP) {
            windows = (Window*)realloc(windows, (total+1)*sizeof(Window));
            windows[total++] = fl_xid(f);
        }
    }

    DBG("Restack: Call XRestackWindows!");
    if(total) XRestackWindows(fl_display, windows, total);
    delete []windows;
}

void WindowManager::update_workarea(bool send)
{
    int left = 0;
    int right = 0;
    int top = 0;
    int bottom = 0;

    for(uint n=0; n<map_order.size(); n++) {
        Frame *f = map_order[n];
        if( f->strut() && (f->desktop()==Desktop::current() || f->frame_flag(STICKY)) ) {
            left = max(left,  f->strut()->left());
            right= max(right, f->strut()->right());
            top  = max(top,   f->strut()->top());
            bottom = max(bottom,f->strut()->bottom());
        }
    }
    wm_area.set(left, top, Fl::w()-(left+right), Fl::h()-(top+bottom));

    for(uint n=stack_order.size(); n--;) {
        Frame *f = stack_order[n];
        if(f->maximized && f->state()==NORMAL) {
            int W=wm_area.w(), H=wm_area.h();
            W-=f->offset_w;
            H-=f->offset_h;
            ICCCM::get_size(f, W, H);
            W+=f->offset_w;
            H+=f->offset_h;

            f->set_size(wm_area.x(), wm_area.y(), W, H);
            f->maximized = true;
        }
    }

    if(send) {
        Desktop::update_desktop_workarea();
        Desktop::update_desktop_geometry();
    }
}

//Updates NET client list atoms
void WindowManager::update_client_list()
{
    int i=0, client_count=0;
    Frame *f;

    Window *net_map_order   = 0;
    Window *net_stack_order = 0;

    for(uint n=0; n<map_order.size(); n++) {
        f = map_order[n];
        if(!f->frame_flag(SKIP_LIST))
            client_count++;
    }
    if(!client_count) return;

    net_map_order   = new Window[client_count];
    net_stack_order = new Window[client_count];

    i=0;
    for(uint n=0; n<stack_order.size(); n++) {
        f = stack_order[n];
        // We don't want to include transients in our client list
        if(!f->frame_flag(SKIP_LIST)) {
            net_stack_order[i++] = f->window();
        }
    }

    i=0;
    for(uint n=0; n<map_order.size(); n++) {
        f = map_order[n];
        // We don't want to include transients in our client list
        if(!f->frame_flag(SKIP_LIST)) {
            net_map_order[i++] = f->window();
        }
    }

    XChangeProperty(fl_display, fl_xid(root), _XA_NET_CLIENT_LIST, XA_WINDOW, 32, PropModeReplace, (unsigned char*)net_map_order, client_count);
    XChangeProperty(fl_display, fl_xid(root), _XA_NET_CLIENT_LIST_STACKING, XA_WINDOW, 32, PropModeReplace, (unsigned char*)net_stack_order, client_count);

    delete []net_stack_order;
    delete []net_map_order;
}

void WindowManager::idle()
{
    for(uint n=remove_list.size(); n--;) {
        Frame *c = remove_list[n];
        delete c;
    }
    remove_list.clear();
}

// Really really really quick fix, since this
// solution sucks. Btw wm_shutdown is in main.cpp.
extern bool wm_shutdown;
void WindowManager::shutdown()
{
	for(uint n = 0; n < map_order.size(); n++)
	{
		Frame* f = map_order[n];
		f->kill();
	}
	wm_shutdown = true;
}

int WindowManager::handle(int e)
{
    Window window = fl_xevent.xany.window;

    switch(e) {
    case FL_PUSH:
        {
            for(uint n=stack_order.size(); n--;) {
                Frame *c = map_order[n];
                if (c->window() == window || fl_xid(c) == window) {
                    c->content_click();
                    return 1;
                }
            }
            DBG("Button press in root?!?!");
            return 0;
        }

    case FL_SHORTCUT:
    case FL_KEY:
        //case FL_KEYUP:
        return Handle_Hotkey();

    case FL_MOUSEWHEEL:
        {
            XAllowEvents(fl_display, ReplayPointer, CurrentTime);
        }
    }

    return 0;
}

int WindowManager::handle(XEvent *e)
{
    switch(e->type)
    {
    case ClientMessage: {
        DBG("WindowManager ClientMessage 0x%lx", e->xclient.window);

        if(handle_desktop_msgs(&(e->xclient))) return 1;

        return 0;
    }
    case ConfigureRequest: {
        DBG("WindowManager ConfigureRequest: 0x%lx", e->xconfigurerequest.window);
        const XConfigureRequestEvent *e = &(fl_xevent.xconfigurerequest);
        XConfigureWindow(fl_display, e->window,
                         e->value_mask&~(CWSibling|CWStackMode),
                         (XWindowChanges*)&(e->x));
        return 1;
    }

    case MapRequest: {
        DBG("WindowManager MapRequest: 0x%lx", e->xmaprequest.window);
        const XMapRequestEvent* e = &(fl_xevent.xmaprequest);
        new Frame(e->window);
        return 1;
    }
    }
    return 0;
}

bool WindowManager::handle_desktop_msgs(const XClientMessageEvent *e)
{
    if(e->format!=32) return false;

    if(e->message_type==_XA_NET_CURRENT_DESKTOP) {
        Desktop::current((int)e->data.l[0]+1);
        return true;
    } else
    if(e->message_type==_XA_NET_NUM_DESKTOPS) {
        DBG("New desk count: %ld", e->data.l[0]);
        Desktop::update_desktop_count(e->data.l[0]);
        // Set also new names...
        Desktop::set_names();
        return true;
    } else
    if(e->message_type==FLTKChangeSettings) {
        DBG("FLTK change settings");
        read_configuration();
        return true;
    }

    return false;
}


