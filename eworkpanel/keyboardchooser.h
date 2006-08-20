// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#ifndef keyboardchooser_h
#define keyboardchooser_h

/*#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>
#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/Fl_Item.h>
#include <efltk/Fl_Util.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/fl_draw.h>
#include <efltk/Fl_Locale.h>*/

#include <fltk/Fl.h>
#include <fltk/Window.h>
#include <fltk/x.h>
#include <fltk/Menu_Button.h>
#include <fltk/Button.h>
#include <fltk/Item_Group.h>
#include <fltk/Item.h>
#include <fltk/Util.h>
#include "EDE_Config.h"
#include <fltk/Image.h>
#include <fltk/Divider.h>
#include <fltk/Item_Group.h>
#include <fltk/draw.h>

#include "icons/keyboard.xpm"


class KeyboardChooser : public fltk::Menu_Button 
{
public:
    KeyboardChooser(int, int, int, int, fltk::Boxtype, fltk::Boxtype, const char *l=0);
    void calculate_height();

    virtual void draw();
    virtual int popup();

    virtual void preferred_size(int &w, int &h) const { w=this->w(); }
    
    bool is_open() { return m_open; }

private:
    int Height;
    fltk::Boxtype up, down;
    bool m_open;
};

#endif
