// X does not echo back the window-map events (it probably should when
// override_redirect is off).  Unfortunately this means you have to use
// this subclass if you want a "normal" fltk window, it will force a
// Frame to be created and destroy it upon hide.

// Warning: modal() does not work!  Don't turn it on as it screws up the
// interface with the window borders.  You can use set_non_modal() to
// disable the iconize box but the window manager must be written to
// not be modal.

#ifndef _WMWINDOW_H_
#define _WMWINDOW_H_

#include <efltk/Fl_Window.h>

class Frame;

class WMWindow : public Fl_Window {
    Frame* frame;
    static void cb(Fl_Widget *w, void *) { ((Fl_Window*)w)->destroy(); }
public:
    WMWindow(int W, int H, const char* L = 0) : Fl_Window(W,H,L) { frame=0; callback(cb); }

    virtual void create();
    virtual void destroy();

    virtual int handle(int e);
};

#endif
