// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include <stdio.h>
/*#include <efltk/Fl.h>
#include <efltk/Fl_Menu_Button.h>
#include <efltk/fl_draw.h>
#include <efltk/Fl_Group.h>
#include <efltk/Fl_Item.h>*/

#include <fltk/Fl.h>
#include <fltk/Menu_Button.h>
#include <fltk/draw.h>
#include <fltk/Group.h>
#include <fltk/Item.h>

#include "panelbutton.h"

extern fltk::Widget* fl_did_clipping;


// class PanelMenu 
// This is a standard eworkpanel applet class. It is a button that pops
// up menu when pressed. Typical use is workspace switcher.

PanelMenu::PanelMenu(int x, int y, int w, int h, fltk::Boxtype up_c, fltk::Boxtype down_c, const char *label)
    : fltk::Menu_Button(x, y, w, h, label)
{
    m_open = false;
    Height = 0;
    up = up_c;
    down = down_c;

    anim_speed(2);
    anim_flags(BOTTOM_TO_TOP);
    accept_focus(false);
}


// This function is modified from Fl_Menu_Button

void PanelMenu::draw()
{
    fltk::Boxtype box = up;
    fltk::Flags flags;
    fltk::Color color;

    if (!active_r()) {
        // Button is disabled
        flags = fltk::INACTIVE;
        color = this->color();
    } else if (m_open) {
        // Menu is open, make the button pushed and highlighted
        flags = fltk::HIGHLIGHT;
        color = highlight_color();
        if (!color) color = this->color();
        box = down;
    } else if (belowmouse()) {
        // Menu is not open, but button is below mouse - highlight
        flags = fltk::HIGHLIGHT;
        color = highlight_color();
        if (!color) color = this->color();
    } else {
        // Plain
        flags = 0;
        color = this->color();
    }

    if(!box->fills_rectangle()) {
        fltk::push_clip(0, 0, this->w(), this->h());
        parent()->draw_group_box();
        fltk::pop_clip();
    }

    box->draw(0, 0, this->w(), this->h(), color, flags);

    int x,y,w,h;
    x = y = 0;
    w = this->w(); h = this->h();
    box->inset(x,y,w,h);
    draw_inside_label(x,y,w,h,flags);
}


// Used to properly redraw menu

void PanelMenu::calculate_height()
{
    fltk::Style *s = fltk::Style::find("Menu");
    Height = s->box->dh();
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *i = child(n);
        if(!i) break;
        if(!i->visible()) continue;
        fltk::font(i->label_font(), i->label_size());
        Height += i->height()+s->leading;
    }
}


// Popup the menu. Global property m_open is useful to detect 
// if the menu is visible, e.g. to disable autohiding panel.

int PanelMenu::popup()
{
    m_open = true;
    redraw(); // push down button
    calculate_height();
    int retval = fltk::Menu_::popup(0, 0-Height);//, w(), h());
    m_open = false;
    redraw();
    return retval;
}


// class PanelButton 
// A simplified case of PanelMenu - by Vedran
// Used e.g. by show desktop button

PanelButton::PanelButton(int x, int y, int w, int h, fltk::Boxtype up_c, fltk::Boxtype down_c, const char *label)
    : fltk::Button(x, y, w, h, label)
{
//    Height = 0;
    up = up_c;
    down = down_c;
    accept_focus(false);
}


void PanelButton::draw()
{
    fltk::Boxtype box = up;
    fltk::Flags flags;
    fltk::Color color;

    if (belowmouse())
    {
        // Highlight button when below mouse
        flags = fltk::HIGHLIGHT;
        color = highlight_color();
        if (!color) color = this->color();
//        box = down;
    } else {
        flags = 0;
        color = this->color();
    }
    
    if (value()) box=down; // Push down button when pressed

    if(!box->fills_rectangle()) {
        fltk::push_clip(0, 0, this->w(), this->h());
        parent()->draw_group_box();
        fltk::pop_clip();
    }

    box->draw(0, 0, this->w(), this->h(), color, flags);

    int x,y,w,h;
    x = y = 0;
    w = this->w(); h = this->h();
    box->inset(x,y,w,h);
    draw_inside_label(x,y,w,h,flags);
}
