#include "Frame.h"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>

// I like the MSWindows resize cursors, so I duplicate them here:

#define CURSORSIZE 16
#define HOTXY 8
static struct TableEntry {
    uchar bits[CURSORSIZE*CURSORSIZE/8];
    uchar mask[CURSORSIZE*CURSORSIZE/8];
    Cursor cursor;
} table[] = {
    {{	// FL_CURSOR_NS
   0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00},
   {
   0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0xf0, 0x0f, 0xf0, 0x0f, 0xc0, 0x03,
   0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xf0, 0x0f,
   0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01}},
  {{	// FL_CURSOR_EW
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10,
   0x0c, 0x30, 0xfe, 0x7f, 0xfe, 0x7f, 0x0c, 0x30, 0x08, 0x10, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x1c, 0x38,
   0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0x1c, 0x38, 0x18, 0x18,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {{	// FL_CURSOR_NWSE
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x38, 0x00, 0x78, 0x00,
   0xe8, 0x00, 0xc0, 0x01, 0x80, 0x03, 0x00, 0x17, 0x00, 0x1e, 0x00, 0x1c,
   0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xec, 0x03, 0xc0, 0x37, 0x80, 0x3f, 0x00, 0x3f, 0x00, 0x3e,
   0x00, 0x3f, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00}},
  {{	// FL_CURSOR_NESW
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x1c, 0x00, 0x1e,
   0x00, 0x17, 0x80, 0x03, 0xc0, 0x01, 0xe8, 0x00, 0x78, 0x00, 0x38, 0x00,
   0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3f,
   0x80, 0x3f, 0xc0, 0x37, 0xec, 0x03, 0xfc, 0x01, 0xfc, 0x00, 0x7c, 0x00,
   0xfc, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {{0}, {0}} // FL_CURSOR_NONE & unknown
};

static int deleteit = 0;
static Cursor cursor;
void set_frame_cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg, Window wid)
{
    if(deleteit) XFreeCursor(fl_display, cursor);
    deleteit=0;

    if (!c) {
        cursor = None;
    } else {
        if (c >= FL_CURSOR_NS) {
            TableEntry *q = (c > FL_CURSOR_NESW) ? table+4 : table+(c-FL_CURSOR_NS);
            if (!(q->cursor)) {
                XColor dummy;
                Pixmap p = XCreateBitmapFromData(fl_display,
                                                 RootWindow(fl_display, fl_screen), (const char*)(q->bits),
                                                 CURSORSIZE, CURSORSIZE);
                Pixmap m = XCreateBitmapFromData(fl_display,
                                                 RootWindow(fl_display, fl_screen), (const char*)(q->mask),
                                                 CURSORSIZE, CURSORSIZE);
                q->cursor = XCreatePixmapCursor(fl_display, p,m,&dummy, &dummy,
                                                HOTXY, HOTXY);
                XFreePixmap(fl_display, m);
                XFreePixmap(fl_display, p);
            }
            cursor = q->cursor;
        } else {
            cursor = XCreateFontCursor(fl_display, (c-1)*2);
            deleteit = 1;
        }
        XColor fgc;
        fg = fl_get_color(fg);
        fgc.red = (fg>>16)&0xFF00;
        fgc.green = (fg>>8)&0xFF00;
        fgc.blue = fg & 0xFF00;
        XColor bgc;
        bg = fl_get_color(bg);
        bgc.red = (bg>>16)&0xFF00;
        bgc.green = (bg>>8)&0xFF00;
        bgc.blue = bg & 0xFF00;
        XRecolorCursor(fl_display, cursor, &fgc, &bgc);
    }
    XDefineCursor(fl_display, wid, cursor);
}

static bool grab_ = false;
void grab_cursor(bool grab)
{
    if (grab_) {
        grab_ = false;
        Fl::event_is_click(0); // avoid double click
        XAllowEvents(fl_display, Fl::event()==FL_PUSH ? ReplayPointer : AsyncPointer, CurrentTime);
        XUngrabPointer(fl_display, fl_event_time); // Qt did not do this...
        XFlush(fl_display); // make sure we are out of danger before continuing...
        // because we "pushed back" the FL_PUSH, make it think no buttons are down:
        Fl::e_state &= 0xffffff;
        Fl::e_keysym = 0;
    }

    // start the new grab:
    // Both X and Win32 have the annoying requirement that a visible window
    // be used as a target for the events, and it cannot disappear while the
    // grab is running. I just grab efltk's first window:
    if(grab && Frame::activeFrame()) {
        Window window = fl_xid(Frame::activeFrame());
        if(window &&
           XGrabPointer(fl_display,
                        window,
                        true, // owner_events
                        ButtonPressMask|ButtonReleaseMask|
                        ButtonMotionMask|PointerMotionMask,
                        GrabModeSync, // pointer_mode
                        GrabModeAsync, // keyboard_mode
                        None, // confine_to
                        cursor, // cursor
                        fl_event_time) == GrabSuccess)
        {
            grab_ = true;
            XAllowEvents(fl_display, AsyncPointer, CurrentTime);
        } else {
            //printf("XGrabPointer failed\n");
        }
    }
}

bool grab() {
    return grab_;
}


