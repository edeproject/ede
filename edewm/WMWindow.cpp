// X does not echo back the window-map events (it probably should when
// override_redirect is off).  Unfortunately this means you have to use
// this subclass if you want a "normal" fltk window, it will force a
// Frame to be created and destroy it upon hide.

// Warning: modal() does not work!  Don't turn it on as it screws up the
// interface with the window borders.  You can use set_non_modal() to
// disable the iconize box but the window manager must be written to
// not be modal.

#include <efltk/Fl.h>
#include "WMWindow.h"
#include "Frame.h"
#include "Windowmanager.h"

extern int dont_set_event_mask;

void WMWindow::create()
{
    Fl_Window::create();
    if(!frame) {
        dont_set_event_mask = 1;
        frame = new Frame(fl_xid(this));
        dont_set_event_mask = 0;
    }
}

void WMWindow::destroy()
{
    if(frame) {
        frame->destroy_frame();
        frame=0;
    }
    Fl_Window::destroy();
}

int WMWindow::handle(int e)
{
    if(e==FL_PUSH || e==FL_MOUSEWHEEL) {
        frame->content_click();
    }
    return Fl_Window::handle(e);
}
