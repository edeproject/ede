// Frame.H

// Each X window being managed by fltk has one of these

#ifndef Frame_H
#define Frame_H

#include "config.h"

#include "Mwm.h"
#include "Icccm.h"

#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Bitmap.h>
#include <efltk/x.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <efltk/Fl_Image.h>

// The state is an enumeration of reasons why the window may be invisible.
// Only if it is NORMAL is the window visible.
enum {
    UNMAPPED	  = 0,    // unmap command from app (X calls this WithdrawnState)
    NORMAL	  = 1,	  // window is visible
    SHADED	  = 2,	  // acts like NORMAL
    ICONIC	  = 3,	  // hidden/iconized
    OTHER_DESKTOP = 4	  // normal but on another desktop
};

// values for frame flags:
// The flags are constant and are turned on by information learned
// from the Gnome, NETWM, and/or Motif window manager hints.  edewm will
// ignore attempts to change these hints after the window is mapped.
enum {
    NO_FOCUS	   = 0x00000001,   // does not take focus
    CLICK_TO_FOCUS = 0x00000002,   // must click on window to give it focus
    KEEP_ASPECT	   = 0x00000004,   // aspect ratio from sizeHints

    MODAL	   = 0x00000008,   // grabs focus from transient_for window

    TAKE_FOCUS_PRT = 0x00000010,   // send junk when giving window focus
    DELETE_WIN_PRT = 0x00000020,   // close box sends a message

    MAX_VERT       = 0x00000100,
    MAX_HORZ       = 0x00000200,
    STICKY         = 0x00000400,   // Everyone knows sticky
    OWNER_HIDDEN   = 0x00000800,   // Owner of transient is hidden
    FIXED_POSITION = 0x00001000,   // Window is fixed in position even

    SKIP_LIST      = 0x00002000,   // Not in window list
    SKIP_FOCUS     = 0x00004000    // Skip "Alt+Tab"
};

// values for func and decor flags:
enum {
    // DECOR only:
    BORDER    = 1,   // Draw border
    THIN_BORDER = 2, // Draw thin border
    TITLEBAR  = 4,   // Show titlebar
    SYSMENU   = 8,   // Show system menu (left on titlebar)

    //FUNC only:
    MOVE      = 16,  // Add move action to system menu
    CLOSE     = 32,  // Add close action to system menu

    //Common flags:
    MINIMIZE  = 64,  // Minimize button and minimize action
    MAXIMIZE  = 128, // Maximize button and maximize action
    RESIZE    = 256  // Resize allowed and resize action
};

// NETWM types
enum {
    TYPE_DESKTOP = 1,
    TYPE_DOCK,
    TYPE_TOOLBAR,
    TYPE_MENU,
    TYPE_UTIL,
    TYPE_SPLASH,
    TYPE_DIALOG,
    TYPE_NORMAL
};

// values for state_flags:
// These change over time
enum {
    IGNORE_UNMAP	= 0x01,	// we did something that echos an UnmapNotify
};

class Icon;
class Desktop;
#include "Titlebar.h"

class Frame : public Fl_Window {
    friend class Titlebar;
    friend class ICCCM;
    friend class MWM;
    friend class NETWM;

    Titlebar *title;
    Window window_;




    int wintype; //Type of window (see NETWM types)

    int frame_flags_;   // above constant flags
    short state_;	// X server state: iconic, withdrawn, normal
    short state_flags_;	// above state flags
    short decor_flags_; // Decoration flags from MWM
    short func_flags_;  // Functions flags from MWM

    int restore_x, restore_w; // saved size when min/max width is set
    int restore_y, restore_h; // saved size when max height is set

    double aspect_min, aspect_max;

    Fl_Rect *strut_;

    MwmHints *mwm_hints;
    XWMHints *wm_hints;
    XSizeHints *size_hints;
    long win_hints;
    
    Window transient_for_xid; // value from X
    Frame* transient_for_; // the frame for that xid, if found

    Frame* revert_to;	// probably the xterm this was run from

    Colormap colormap;	// this window's colormap
    int colormapWinCount; // list of other windows to install colormaps for
    Window *colormapWindows;
    Colormap *window_Colormaps; // their colormaps

    // These are filled if window has icon
    Icon *icon_;

    Desktop* desktop_;

    int maximize_width();
    int maximize_height();
    int force_x_onscreen(int X, int W);
    int force_y_onscreen(int Y, int H);

    void sendMessage(Atom, Atom) const;

    void send_desktop();
    void send_configurenotify();
    void send_state_property();

    void* getProperty(Atom, Atom = AnyPropertyType, unsigned long *length = 0, int *ret=0) const;
    int  getIntProperty(Atom, Atom = AnyPropertyType, int deflt = 0, int *ret = 0) const;
    void setProperty(Atom, Atom, int) const;
    void getLabel(bool del = false, Atom from_atom=0);
    void getColormaps();
    int getDesktop();

    void updateBorder();
    void fix_transient_for(); // called when transient_for_xid changes

    void get_protocols();

    void get_wm_hints();
    void update_wm_hints();

    void get_funcs_from_type();

    void installColormap() const;

    void set_cursor(int);
    int mouse_location();

    void layout() { Fl_Widget::layout(); }
    void draw();
    
    static Frame* active_;
    void _desktop(Desktop*);

    bool configure_event(const XConfigureRequestEvent *e);
    bool map_event(const XMapRequestEvent *e);
    bool unmap_event(const XUnmapEvent *e);
    bool destroy_event(const XDestroyWindowEvent *e);
    bool reparent_event(const XReparentEvent *e);
    bool clientmsg_event(const XClientMessageEvent *e);
    bool colormap_event(const XColormapEvent *e);
    bool property_event(const XPropertyEvent *e);
    bool visibility_event(const XVisibilityEvent *e);
    bool shape_event(const XShapeEvent *e);

public:
    bool shaped;
    bool get_shape();

    int offset_x, offset_y, offset_w, offset_h;
    void set_size(int x, int y, int w, int h);

    int frame_flags()   const { return frame_flags_; }
    short decor_flags() const { return decor_flags_; }
    short func_flags()  const { return func_flags_; }
    short state_flags()  const { return state_flags_; }

    bool frame_flag(int i) { return ((frame_flags_&i)==i) ? true : false; }
    void set_frame_flag(int i)   { frame_flags_ |= i; }
    void clear_frame_flag(int i) { frame_flags_ &=~i; }
    void clear_frame_flags() { frame_flags_ = 0; }

    bool decor_flag(short i) { return ((decor_flags_&i)==i) ? true : false; }
    void set_decor_flag(short i)   { decor_flags_ |= i; }
    void clear_decor_flag(short i) { decor_flags_ &=~i; }
    void clear_decor_flags() { decor_flags_ = 0; }

    bool func_flag(short i) { return ((func_flags_&i)==i) ? true : false; }
    void set_func_flag(short i)   { func_flags_ |= i; }
    void clear_func_flag(short i) { func_flags_ &=~i; }
    void clear_func_flags() { func_flags_ = 0; }

    bool state_flag(short i) { return ((state_flags_&i)==i) ? true : false; }
    void set_state_flag(short i)   { state_flags_ |= i; }
    void clear_state_flag(short i) { state_flags_ &=~i; }
    void clear_state_flags() { state_flags_ = 0; }

    void cb_button_close(Fl_Button*);
    void cb_button_kill(Fl_Button*);
    void cb_button_max(Fl_Button*);    
    void cb_button_min(Fl_Button*);    

    int handle(int e);	// handle fltk events
    int handle(const XEvent *e);
    void handle_move(int &nx, int &ny);

    void window_type(int t) { wintype=t; }
    int window_type() { return wintype; }

    Frame(Window, XWindowAttributes* = 0);
    ~Frame();

    void destroy_frame();

    void settings_changed();
    static void settings_changed_all();

    Fl_Rect *strut() { return strut_; }
    Icon *icon() { return icon_; }

    Window window() const {return window_;}
    Frame* transient_for() const {return transient_for_;}
    int is_transient_for(const Frame*) const;

    Desktop* desktop() const {return desktop_;}
    void desktop(Desktop*);
    static void desktop(Window wid, int desktop);

     // Wm calls this, when user clicks inside frames
    void content_click();

    // Raises and put window top of stack
    void raise();
    static void raise(Window wid);

    // Lowers and put window back of stack
    void lower();

    void iconize();
	void maximize();
	void restore();

    void throw_focus(int destructor = 0);

    // Closes window with given protocol, if theres no protocol we just kill it
    static void close(Window wid);
    void close();
    void kill();

    // returns true if it actually sets active state(focus)
    bool activate();
    bool activate_if_transient();
    void deactivate();

    // Get or set new state, handles iconizing, mapping and unmapping!
    short state() const {return state_;}
    void state(short newstate); // don't call this unless you know what you are doing!

    int active() const {return active_==this;}
    static Frame* activeFrame() {return active_;}

    static bool do_opaque;
	static bool focus_follows_mouse;

    static bool animate;
    static int animate_speed;

    bool maximized;
};

// handy wrappers for those ugly X routines:
extern void* getProperty(Window, Atom, Atom = AnyPropertyType, unsigned long* length = 0, int *ret=0);
extern int   getIntProperty(Window, Atom, Atom = AnyPropertyType, int deflt = 0, int *ret=0);
extern void  setProperty(Window, Atom, Atom, int);

extern void draw_overlay(int x, int y, int w, int h);
void clear_overlay();

static inline int max(int a, int b) {return a > b ? a : b;}
static inline int min(int a, int b) {return a < b ? a : b;}

#endif

