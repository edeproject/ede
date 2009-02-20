// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#ifndef keyboardchooser_h
#define keyboardchooser_h

#include <efltk/Fl.h>
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
#include <efltk/Fl_Locale.h>

#include "icons/keyboard.xpm"


class KeyboardChooser : public Fl_Menu_Button 
{
public:
    KeyboardChooser(int, int, int, int, Fl_Boxtype, Fl_Boxtype, const char *l=0);
    void calculate_height();

    virtual void draw();
    virtual int popup();

    virtual void preferred_size(int &w, int &h) const { w=this->w(); }
    
    bool is_open() { return m_open; }

private:
    int Height;
    Fl_Boxtype up, down;
    bool m_open;
};

#endif
