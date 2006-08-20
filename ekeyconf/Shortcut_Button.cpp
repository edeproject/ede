// -- copyied over from fluid (Fl_Menu_Type.cpp)
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "Shortcut_Button.h"

#include <fltk/Button.h> //#include <efltk/Fl_Button.h>
#include <fltk/draw.h> //#include <efltk/fl_draw.h>
//#include <efltk/Fl.h>
#include <fltk/events.h>


void Shortcut_Button::draw() {
  label(fltk::key_name(svalue));
  color(value() ? default_style->selection_color() : default_style->color());
  textcolor(value() ? default_style->selection_textcolor() : default_style->textcolor());
#ifdef _WIN32
  Button::draw();
#else
  fltk::Button::draw();
#endif
}


#include <stdio.h>
int Shortcut_Button::handle(int e) {
  when(0); type(TOGGLE);
  if (e == fltk::KEY) {
    if (!value()) return 0;
    int v = fltk::event_text()[0];
    if (v > 32 && v < 0x7f || v > 0xa0 && v <= 0xff) {
      v = v | fltk::event_state()&(fltk::META|fltk::ALT|fltk::CTRL);
    } else {
      v = fltk::event_state()&(fltk::META|fltk::ALT|fltk::CTRL|fltk::SHIFT) | fltk::event_key();
      if (v == fltk::BackSpaceKey && svalue) v = 0;
    }
    if (v != svalue) {svalue = v; do_callback(); redraw();}
    return 1;
  } else if (e == fltk::UNFOCUS) {
    int c = changed(); value(0); if (c) set_changed();
    return 1;
  } else if (e == fltk::FOCUS) {
    return value();
  } else {
#ifdef _WIN32
    int r = Button::handle(e);
#else
    int r = fltk::Button::handle(e);
#endif
    if (e == fltk::RELEASE && value() && fltk::focus() != this) take_focus();
    return r;
  }
}
