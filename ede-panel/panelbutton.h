// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#ifndef panelbutton_h
#define panelbutton_h

#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>
#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/Fl_Item.h>

class PanelMenu : public Fl_Menu_Button 
{
public:
    PanelMenu(int, int, int, int, Fl_Boxtype, Fl_Boxtype, const char *l=0);
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

class PanelButton : public Fl_Button 
{
public:
    PanelButton(int, int, int, int, Fl_Boxtype, Fl_Boxtype, const char *l=0);

    virtual void draw();

    virtual void preferred_size(int &w, int &h) const { w=this->w(); }

private:
    int Height;
    Fl_Boxtype up, down;
};

#endif
