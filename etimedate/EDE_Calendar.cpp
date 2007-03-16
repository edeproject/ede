/***************************************************************************
                          Fl_Calendar.cpp  -  description
                             -------------------
    begin                : Sun Aug 18 2002
    copyright            : (C) 2002 by Alexey Parshin
    email                : alexeyp@m7.tts-sf.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
// Ported to FLTK2 by Vedran Ljubovic <vljubovic@smartnet.ba>, 2005.

#include "EDE_Calendar.h"

// For NLS stuff
//#include "../core/fl_internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fltk/events.h>
#include <fltk/Rectangle.h>
#include <fltk/draw.h>
#include <fltk/events.h>

#include "../edelib2/NLS.h"

using namespace fltk;




// FIXME: dont do static
static EDE_Calendar* thecalendar;




// Constants

static const char *weekDayLabels[7] = {
    "Su","Mo","Tu","We","Th","Fr","Sa"
};

static const char *monthDayLabels[31] = {
    "1","2","3","4","5","6","7","8","9","10",
    "11","12","13","14","15","16","17","18","19","20",
    "21","22","23","24","25","26","27","28","29","30",
    "31"
};

static const char *switchLabels[4] = {
    "@-1<<","@-1<","@-1>","@-1>>"
};

static const int monthChanges[4] = {
    -12,-1,1,12
};

// TODO: read this from locale
const bool weekStartsOnMonday = false;


// Callback function for day buttons

void EDE_Calendar::cbDayButtonClicked(Widget *button, void *param) {
	if (event_clicks() == 1 || event_key() == ReturnKey) {
// NOTE: this used to read:
//   button->parent->dayButtonChanged...
// but it didn't work! FIXME
		thecalendar->dayButtonChanged((unsigned)param);
	} else {
		thecalendar->dayButtonClicked((unsigned)param);
	}
}

// Callback function for switch buttons

void EDE_Calendar::cbSwitchButtonClicked(Widget *button, void *param) {
	thecalendar->switchButtonClicked((int)param);
}


// Real callback functions:

void EDE_Calendar::dayButtonClicked(unsigned cday) {
    if (cday < 1 || cday > 31) return;
    short year, month, day;
    m_activeDate.decode_date(&year,&month,&day);
    Fl_Date_Time::encode_date((double&)m_activeDate,year,month,cday);
    redraw();
//    do_callback();     // callback only on changing "today" date
}

void EDE_Calendar::dayButtonChanged(unsigned cday) {
    if (cday < 1 || cday > 31) return;
    short year, month, day;
    m_todayDate.decode_date(&year,&month,&day);
    m_activeDate.decode_date(&year,&month,&day);
    Fl_Date_Time::encode_date((double&)m_todayDate,year,month,cday);
    redraw();
    do_callback();
}

void EDE_Calendar::switchButtonClicked(int monthChange) {
    short year, month, day;
    m_activeDate.decode_date(&year,&month,&day);
    month += monthChange;
    if (month < 1) {
        month += 12;
        year--;
    }
    if (month > 12) {
        month -= 12;
        year++;
    }
//    Fl_Date_Time newDate(year,month,day);
//    date(newDate);
    Fl_Date_Time::encode_date((double&)m_activeDate,year,month,day);
    redraw();
//    do_callback();
}


// This is stuff for NamedStyle - still needed?

static void revert(Style* s) {
    s->color_ = GRAY75;
    s->buttoncolor_ = GRAY75;
    s->box_ = FLAT_BOX;
    s->buttonbox_ = THIN_UP_BOX;
    s->textfont_ = HELVETICA_BOLD;
}

static NamedStyle style("Calendar", revert, &EDE_Calendar::default_style);
NamedStyle* EDE_Calendar::default_style = &::style;


// Constructor

EDE_Calendar::EDE_Calendar(int x,int y,int w,int h,const char *lbl)
: Group(2,2,w-2,h-2,lbl) {
    thecalendar = this;
    m_globalx = x; m_globaly = y;
    
    style(default_style);
    unsigned i;
    
    // Header box
    m_headerBox = new Group(x,y,w,32);
    m_monthNameBox = new InvisibleBox(x,y,w,16);
    m_monthNameBox->box(NO_BOX);

    // NLS stuff - FIXME this can't work because gettext needs literals inside _()
    for (i=0; i<7;i++) weekDayLabels[i]=_(weekDayLabels[i]);

    // Weekday headers
    for (i = 0; i < 7; i++) {
        m_dayNameBoxes[i] = new InvisibleBox(x+i*16,y+16,16,16,weekDayLabels[i]);
    }
    m_headerBox->end();

    // Day buttons, correct positions are set by resize()
    m_buttonBox = new Group(x,y+32,w,64);
    m_buttonBox->box(FLAT_BOX);
    for (i = 0; i < 31; i++) {
        Button *btn = new Button(0,0,16,16,monthDayLabels[i]);
        m_dayButtons[i] = btn;
        btn->callback(EDE_Calendar::cbDayButtonClicked, (void *)(i+1));
    }
    m_buttonBox->end();

    // Switch buttons, correct positions are set by resize()
    for (i = 0; i < 4; i++) {
        m_switchButtons[i] = new Button(x,y,16,16,switchLabels[i]);
        m_switchButtons[i]->callback(EDE_Calendar::cbSwitchButtonClicked, (void *)monthChanges[i]);
        m_switchButtons[i]->labeltype(SYMBOL_LABEL);
    }

    end();
    date(Fl_Date_Time::Now());
}

/*
// New style ctor
Fl_Calendar::Fl_Calendar(const char* l,int layout_size,Align layout_al,int label_w)
: Group (l,layout_size,layout_al,label_w) 
{
    
    ctor_init(0,0,w(),h());
}*/

void EDE_Calendar::layout() {
    int xx = m_globalx, yy = m_globaly;	// in FLTK2 positions are absolute, not relative
    int ww = w(), hh = h();
    Rectangle* rect = new Rectangle(xx,yy,ww,hh);
    box()->inset(*rect);
    unsigned i;

    // one daybox = boxh*boxw is unit of size
    int boxh = hh / 10;
    int boxw = ww / 7;
    
    // rounding dimensions to a whole number of boxes
    ww = boxw * 7;
    hh = hh / boxh * boxh;	// why not boxh * 10 ?

    // center horizontally inside this space
    xx = xx + (w()-ww)/2+1;
    
//    if(xx<box()->dx()) xx=box()->dx();	//TODO: dx() is no longer available

    // resize header
    m_headerBox->resize(xx, yy, ww, boxh*2+2);
    m_monthNameBox->resize(xx, yy, ww, boxh); // month name is actually larger

    // resize column titles (Su, Mo, Tu...)
    for (i=0; i < 7; i++) {
        m_dayNameBoxes[i]->resize(boxw*i + xx, boxh + yy+2, boxw, boxh); // why +2 ?
    }

    // compute the month start date
    short year, month, day;
    if ((double)m_todayDate < 1) m_todayDate = Fl_Date_Time::Now();
    if ((double)m_activeDate < 1) m_activeDate = m_todayDate;
    m_activeDate.decode_date(&year,&month,&day);
    Fl_Date_Time    monthDate(year,month,1);
    
    // create month name label
    char yearstr[4];
    snprintf(yearstr,4,"%d",year);
    strncpy(m_headerLabel, monthDate.month_name(), 13);
    strcat(m_headerLabel, ", ");
    strcat(m_headerLabel, yearstr);
    m_monthNameBox->label(m_headerLabel);

    // resize day buttons
    int topOffset = boxh*2 + yy+2;
    m_buttonBox->resize(xx, topOffset, boxw*7, boxh*6);	// background

    int dayOffset   = monthDate.day_of_week()-1;
    int daysInMonth = monthDate.days_in_month();
    for (i = 0; i < 31; i++) {
        Button *btn = m_dayButtons[i];
        btn->resize(dayOffset*boxw + xx, topOffset, boxw, boxh); // 32 = header; bh = daynameboxes
        if ((int)i < daysInMonth) {
            dayOffset++;
            if (dayOffset > 6) {
                dayOffset = 0;
                topOffset += boxh;
            }
            btn->show();
        }
        else  btn->hide();
    }

    int sby = m_buttonBox->y() + m_buttonBox->h();
    for (i = 0; i < 2; i++)
        m_switchButtons[i]->resize(i*boxw + xx, sby, boxw, boxh);

    int x1 = ww - boxw * 2;
    for (i = 2; i < 4; i++) {
        m_switchButtons[i]->resize((i-2)*boxw + x1 + xx, sby, boxw, boxh);
    }

   //Clear layout flags
    Widget::layout();
}

void EDE_Calendar::draw() {
    // Note - Fl_Calendar has fixed colors because themes could make it ugly or unreadable
    // TODO: Improve this!

//    Color btn_color = color_average(buttoncolor(), WHITE, .4f);
    //Color btn_color_hl = color_average(buttoncolor(), GRAY75, .5f);
    
    //Color btn_color = lerp(buttoncolor(), WHITE, .4f);
    Color btn_color = fltk::color(255,255,204);	// light yellowish grey a la paper - don't remove fltk:: !
    Color btn_color_hl = WHITE;
    Color label_color = BLACK;
    Color day_color = lerp(BLUE, GRAY85, .8f);
    Color day_color_wknd = lerp(BLUE, WHITE, .9f);	// light reddish gray

    unsigned i;

    short year, month, day;
    m_activeDate.decode_date(&year,&month,&day);
    short activeindex = day-1;
    short tyear, tmonth, tday;
    m_todayDate.decode_date(&tyear,&tmonth,&tday);
    short todayindex = tday-1;
    if (tyear != year || tmonth != month) todayindex=-1;
    
    
    for (i = 0; i < 31; i++) {
        Button *btn = m_dayButtons[i];
        btn->box(THIN_UP_BOX);
        //btn->focusbox(DOTTED_FRAME);
        btn->color(btn_color);
//        btn->highlight_color(btn_color_hov);
        btn->labelfont(labelfont());
        btn->labelcolor(label_color);
        btn->labelsize(labelsize());
        if((int)i==activeindex) {
            btn->box(FLAT_BOX);
            btn->color(btn_color_hl);
        }
        if((int)i==todayindex) {
            //btn->box(BORDER_FRAME);
	    // TODO: why is this rectangle drawn behind button?
//	    setcolor((Color)RED);
//	    drawline(btn->x(),btn->y(),btn->x(),btn->y()+btn->h());
//	    drawline(btn->x(),btn->y()+btn->h(),btn->x()+btn->w(),btn->y()+btn->h());
//	    drawline(btn->x()+btn->w(),btn->y()+btn->h(),btn->x()+btn->w(),btn->y());
//	    drawline(btn->x()+btn->w(),btn->y(),btn->x(),btn->y());
	    
	    btn->focusbox(BORDER_FRAME);
	    // How do I make a red frame? apparently not possible
//            btn->textcolor(RED);
            focus(btn);
        }
    }

    for (i = 0; i < 4; i++) {
        m_switchButtons[i]->box(FLAT_BOX);
        m_switchButtons[i]->color(WHITE);
        m_switchButtons[i]->labelcolor(BLACK);
        m_switchButtons[i]->highlight_color(GRAY75);
        m_switchButtons[i]->labelsize(labelsize());
    }

    for (i=0; i < 7; i++) {
        m_dayNameBoxes[i]->box(buttonbox());
        m_dayNameBoxes[i]->color(day_color);
        m_dayNameBoxes[i]->labelcolor(label_color);
        m_dayNameBoxes[i]->labelsize(labelsize());
        if(i==0 || i==6)       
            m_dayNameBoxes[i]->color(day_color_wknd);
//            m_dayNameBoxes[i]->labelcolor(RED);
    }

    m_monthNameBox->labelfont(textfont());
    m_monthNameBox->labelsize(textsize());
    m_monthNameBox->labelcolor(textcolor());

//    m_buttonBox->color(darker(buttoncolor()));
    m_buttonBox->color(lerp(buttoncolor(),BLACK,.67f));

    Group::draw();
}

void EDE_Calendar::measure(int& ww,int& hh) const {
    ww = (w() / 7) * 7;
    hh = (h() / 10) * 10;
}

void EDE_Calendar::date(Fl_Date_Time dt) {
    m_todayDate = dt;
    m_activeDate = dt;
    
    short year, month, day;
    m_todayDate.decode_date(&year,&month,&day);
    focus(m_dayButtons[day-1]);

    relayout();
    redraw();
}

Fl_Date_Time EDE_Calendar::date() const {
    short year, month, day;
    m_todayDate.decode_date(&year,&month,&day);
    return Fl_Date_Time(year, month, day);
}

//------------------------------------------------------------------------------------------------------

/* Fl_Popup_Calendar - we lack Fl_Popup_Window to make this work... maybe later

static void popup_revert(Style* s)
{
    s->color = GRAY75;
    s->buttoncolor = GRAY75;
    s->box = BORDER_BOX;
    s->buttonbox = THIN_UP_BOX;
    s->font = HELVETICA_BOLD;
}

static NamedStyle popup_style("Popup_Calendar", popup_revert, &Fl_Popup_Calendar::default_style);
NamedStyle* Fl_Popup_Calendar::default_style = &::popup_style;

void cb_clicked(Widget *w, void *d) {
    Window *win = w->window();
    if(win) {
        win->set_value();
        win->hide();
    }
    Fl::exit_modal(); //Just in case :)
}

Fl_Popup_Calendar::Fl_Popup_Calendar(Widget *dateControl)
    : Fl_Popup_Window(150,150,"Calendar")
{
    style(default_style);
    m_dateControl = dateControl;
    m_calendar = new Fl_Calendar(0,0,w(),h());
    m_calendar->callback(cb_clicked);
    m_calendar->box(NO_BOX);
    m_calendar->copy_style(style());

    end();
}

void Fl_Popup_Calendar::draw()
{
    m_calendar->copy_style(style());
    Fl_Popup_Window::draw();
}

void Fl_Popup_Calendar::layout() {
    m_calendar->resize(box()->dx(),box()->dy(),w()-box()->dw(),h()-box()->dh());
    m_calendar->layout();
    Fl_Popup_Window::layout();
}

bool Fl_Popup_Calendar::popup() {
    if (m_dateControl) {
        int width = m_dateControl->w();
        if (width < 175) width = 175;
        int X=0, Y=0;
        for(Widget* w = m_dateControl; w; w = w->parent()) {
            X += w->x();
            Y += w->y();
        }
        int height = 160;
        m_calendar->size(width,height);
        m_calendar->measure(width,height);

        resize(X, Y+m_dateControl->h()-1, width+box()->dw(), height+box()->dh());
    }
    return Fl_Popup_Window::show_popup();
}

bool Fl_Popup_Calendar::popup(Widget *dateControl, int X, int Y, int W, int H) {
    if(dateControl) {
        int width = (W>0) ? W : dateControl->w();
        if (width < 175) width = 175;
        int height = (H>0) ? H : 175;
        if (height < 175) height = 175;
        for(Widget* w = m_dateControl; w; w = w->parent()) {
            X += w->x();
            Y += w->y();
        }
        resize(X, Y, width, height);
    }
    return Fl_Popup_Window::show_popup();
}


int Fl_Popup_Calendar::handle(int event) {
    int rc = Fl_Popup_Window::handle(event);

    if (rc) return rc;

    return m_calendar->handle(event);
}*/
