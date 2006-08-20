/*
 * $Id: Fl_Calendar.h,v 1.7 2003/04/05 20:44:12 parshin Exp $
 *
 * Extended Fast Light Toolkit (EFLTK)
 * Copyright (C) 2002-2003 by EDE-Team
 * WWW: http://www.sourceforge.net/projects/ede
 *
 * Fast Light Toolkit (FLTK)
 * Copyright (C) 1998-2003 by Bill Spitzak and others.
 * WWW: http://www.fltk.org
 *
 * This library is distributed under the GNU LIBRARY GENERAL PUBLIC LICENSE
 * version 2. See COPYING for details.
 *
 * Author : Alexey Parshin
 * Email  : alexey@fltk.net
 *
 * Please report all bugs and problems to "efltk-bugs@fltk.net"
 *
 */
//
// Ported to FLTK2 by Vedran Ljubovic <vljubovic@smartnet.ba>, 2005.

#ifndef _EDE_CALENDAR_H_
#define _EDE_CALENDAR_H_

/*
#include <efltk/Fl.h>
#include <efltk/Fl_Popup_Window.h>
#include <efltk/Fl_Date_Time.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Button.h>*/

#include "Fl_Date_Time.h"
#include <fltk/InvisibleBox.h>
#include <fltk/Button.h>
#include <fltk/Group.h>
#include <fltk/Symbol.h>

/** Fl_Calendar */
class EDE_Calendar : public fltk::Group {
public:
    static fltk::NamedStyle* default_style;

    /** The traditional constructor creates the calendar using the position, size, and label. */
    EDE_Calendar(int x,int y,int w,int h,const char *lbl=0L);

    /** The new style constructor creates the calendar using the label, size, alignment, and label_width. */
//    Fl_Calendar(const char* l = 0,int layout_size=30,fltk::Align layout_al=fltk::ALIGN_TOP,int label_w=100);

    virtual void layout();
    virtual void draw();
    virtual void measure(int& w,int& h) const;

    virtual void reset() { date(Fl_Date_Time::Now()); }

    void date(Fl_Date_Time dt);
    Fl_Date_Time date() const;

    void dayButtonClicked(unsigned day);
    void dayButtonChanged(unsigned day);
    void switchButtonClicked(int monthChange);

private:
    static void cbDayButtonClicked(fltk::Widget *,void *);
    static void cbSwitchButtonClicked(fltk::Widget *,void *);

    fltk::Group     *m_headerBox;
    fltk::Group     *m_buttonBox;
    fltk::InvisibleBox       *m_monthNameBox;
    fltk::InvisibleBox       *m_dayNameBoxes[7];
    fltk::Button    *m_dayButtons[31];
    fltk::Button    *m_switchButtons[4];
    Fl_Date_Time  m_todayDate;
    Fl_Date_Time  m_activeDate;
    int m_globalx, m_globaly;
    char m_headerLabel[20]; // month+year shouldn't get larger in any locale
    

    void ctor_init(int x,int y,int w,int h);
};

/* We lack Fl_Popup_Window to make this work... Maybe later...

class Fl_Popup_Calendar : public Fl_Popup_Window {
public:
    static fltk::NamedStyle* default_style;

    Fl_Popup_Calendar(fltk::Widget *dateControl=NULL);

    Fl_Calendar *calendar() { return m_calendar; }

    void clicked() { set_value(); }
    void layout();
    void draw();
    int  handle(int);

    void date(Fl_Date_Time dt) { m_calendar->date(dt); }
    Fl_Date_Time date() const       { return m_calendar->date(); }

    bool popup();
    // Popup calendar, relative to widget
    bool popup(fltk::Widget *dateControl, int X, int Y, int W=0, int H=0);

private:
    friend class Fl_Calendar;
    Fl_Calendar *m_calendar;
    fltk::Widget   *m_dateControl;
};*/

#endif
