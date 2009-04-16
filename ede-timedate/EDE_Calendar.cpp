/*
 * $Id$
 *
 * Application for setting system date, time and local timezone
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


// This is the Calendar widget class inspired by the Calendar originally
// developed by Alexey Parshin for SPTK and later imported into efltk.



#include "EDE_Calendar.h"


#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <edelib/Nls.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint;


// TODO: replace this with a simple loop when operator++ is implemented in edelib::Date
long date_distance(edelib::Date da1, edelib::Date da2) {
	if (da1 > da2) {
		edelib::Date tmp = da2;
		da2=da1;
		da1=tmp;
	}
	int d1=da1.day(), m1=da1.month(), y1=da1.year(), d2=da2.day(), m2=da2.month(), y2=da2.year();
	long result=0;
	
	while (d1!=d2 || m1!=m2 || y1!=y2) {
		result++;
		d1++;
		if (!da1.is_valid(y1,m1,d1)) {
			d1=1;
			m1++;
			if (m1>12) {
				m1=1;
				y1++;
			}
		}
	}
	return result;
}



// Constants

static const char *switchLabels[4] = {
	"@-1<<","@-1<","@-1>","@-1>>"
};

static const int monthChanges[4] = {
	-12,-1,1,12
};

// TODO: read this from locale
const bool weekStartsOnMonday = false;



// Callback function for fwd / back buttons

void EDE_Calendar::cbSwitchButtonClicked(Fl_Widget *button, long month_change) {
	EDE_Calendar* thecalendar = (EDE_Calendar*)button->parent();
	edelib::Date d = thecalendar->active_date();
	int year = d.year();
	int month = d.month()+month_change;
	int day = d.day();

	// Fix month
	while (month<1) { year--; month+=12; }
	while (month>12) { year++; month-=12; }

	// Switch to closest valid day
	while (day>27 && !d.is_valid(year, month, day)) day--;

	if (d.is_valid(year, month, day)) {
		d.set(year, month, day);
		thecalendar->active_date(d);
	}
}


// ctor

EDE_Calendar::EDE_Calendar(int x,int y,int w,int h,const char *lbl)
: Fl_Group(x,y,w,h,lbl) {
	unsigned int  i;
	
	// Calendar contents, correct size and position is set by layout()
	
	// Header box
	m_monthNameBox = new Fl_Box(x,y,w,16);
	m_monthNameBox->box(FL_NO_BOX);
	
	// Weekday headers
	for (i = 0; i < 7; i++) {
 		m_dayNameBoxes[i] = new Fl_Box(x+i*16,y+16,16,16);
		m_dayNameBoxes[i]->box(FL_FLAT_BOX);
		m_dayNameBoxes[i]->color(fl_color_average(color(), FL_GREEN, 0.8));
		
		// get first two letters of day name
		edelib::Date d;
		d.set(1900,1,7+i); // 1.1.1900 was Monday
		char tmp[3];
		snprintf(tmp,3,"%s", d.day_name());
		m_dayNameBoxes[i]->copy_label(tmp);
	}
	
	// Fillers (parts of calendar without day buttons)
	for (int i=0; i<3; i++) {
		m_filler[i] = new Fl_Box(x,y,16,16);
		m_filler[i]->box(FL_FLAT_BOX);
		m_filler[i]->color(fl_color_average(color(), FL_BLACK, 0.95)); // very mild grayish
	}

	// Day buttons
	for (i = 0; i < 31; i++) {
		m_dayButtons[i] = new Fl_Box(0,0,16,16);
		char tmp[3];
		snprintf(tmp,3, "%d", (i+1));
		m_dayButtons[i]->copy_label(tmp);
	}
	
	// Switch buttons
	for (i = 0; i < 4; i++) {
		Fl_Repeat_Button* o;
		m_switchButtons[i] = o = new Fl_Repeat_Button(x,y,16,16,switchLabels[i]);
		o->callback(EDE_Calendar::cbSwitchButtonClicked, (long)monthChanges[i]);
		o->labelcolor(fl_darker(FL_BACKGROUND_COLOR));
		o->selection_color(fl_lighter(FL_BACKGROUND_COLOR));
	}
	
	end();
	
	reset(); // this will eventually call layout()
}


// Number of rows and columns

#define CAL_ROWS 9
#define CAL_COLS 7


// Calculate positions and sizes of various boxes and buttons
// NOTE: This method is costly! Avoid calling it unless calendar is actually resized
void EDE_Calendar::layout(int X, int Y, int W, int H) {
	unsigned int i;
	
	// Leave some room for edges
	int x_ = X+2;
	int y_ = Y+2;
	int w_ = W-4;
	int h_ = H-4;
	
	// Precalculate button grid
	// By doing this we try to avoid floating point math while 
	// having coords without holes that add up to a desired width
	int bx[CAL_COLS], bw[CAL_COLS], by[CAL_ROWS], bh[CAL_ROWS];
	bx[0] = x_; by[0] = y_;
	for (i=1; i<CAL_COLS; i++) {
		bx[i] = x_ + (w_*i)/CAL_COLS;
		bw[i-1] = bx[i]-bx[i-1];
	}
	bw[CAL_COLS-1] = x_ + w_ - bx[CAL_COLS-1];
	for (i=1; i<CAL_ROWS; i++) {
		by[i] = y_ + (h_*i)/CAL_ROWS;
		bh[i-1] = by[i]-by[i-1];
	}
	bh[CAL_ROWS-1] = y_ + h_ - by[CAL_ROWS-1];
	
	
	// Title
	m_monthNameBox->resize( x_,y_,w_, bh[0] );
	
	// Labels
	for (i=0; i<7; i++) 
		m_dayNameBoxes[i]->resize( bx[i], by[1], bw[i], bh[1] );
	
	// Find first day of week
	edelib::Date d=active_date_;
	d.set(d.year(), d.month(), 1);
	uint day_of_week = d.day_of_week()-1;
	
	// First filler
	if (day_of_week>0) {
		int k=0;
		for (i=0; i<day_of_week; i++) k += bw[i];
		m_filler[0]->resize( x_, by[2], k, bh[2] );
	}
	
	// Days
	int row=2;
	for (i=0; i<d.days_in_month(); i++) {
		m_dayButtons[i]->resize( bx[day_of_week], by[row], bw[day_of_week], bh[row] );
		day_of_week++;
		if (day_of_week==7) { day_of_week=0; row++; }
	}

	// Second filler
	if (day_of_week<6) {
		int k=0;
		for (i=day_of_week; i<7; i++) k += bw[i];
		m_filler[1]->resize( bx[day_of_week], by[row], k, bh[row] );
	}
		
	// Third filler
	if (row<7)
		m_filler[2]->resize( x_, by[7], w_, bh[7] );
	
	// Switch buttons
	m_switchButtons[0]->resize ( x_, by[8], bw[0], bh[8] );
	m_switchButtons[1]->resize ( bx[1], by[8], bw[1], bh[8] );
	m_switchButtons[2]->resize ( bx[5], by[8], bw[5], bh[8] );
	m_switchButtons[3]->resize ( bx[6], by[8], bw[6], bh[8] );
}



// Overloaded resize (used to call layout) 

void EDE_Calendar::resize(int X, int Y, int W, int H) {
	// avoid unnecessary resizing
	static int oldw=0, oldh=0;
	if (W==oldw && H==oldh) return Fl_Group::resize(X,Y,W,H);
	oldw=W; oldh=H;
	
	layout(X,Y,W,H);
	Fl_Widget::resize(X,Y,W,H); // avoid Fl_Group resizing because we already resized everything
}




// Set visual appearance & colors of day buttons and tooltip

void EDE_Calendar::update_calendar() {
	unsigned int i;
	
	// Find first day of week
	edelib::Date d=active_date_;
	d.set(d.year(), d.month(), 1);
	int day_of_week = d.day_of_week()-1;
	
	// Show/hide first filler
	if (day_of_week>0)
		m_filler[0]->show();
	else
		m_filler[0]->hide();
	
	// Days
	int row=2;
	for (i=0; i<d.days_in_month(); i++) {
		Fl_Box* btn = m_dayButtons[i]; // shortcut
		btn->show();
		
		// Set button color
		Fl_Color daycolor = color(); // base color is the color of calendar

		if (day_of_week==0) // Special color for sunday
			daycolor = fl_color_average(daycolor, FL_BLUE, 0.8);
		
		if (i==(uint)today_date_.day()-1 && d.month()==today_date_.month() && d.year()==today_date_.year()) 
			btn->color(fl_color_average(daycolor, FL_RED, 0.5)); // today
		else if (i==(uint)active_date_.day()-1)
			btn->color(fl_lighter(daycolor));
		else
			btn->color(daycolor);
		
		// Set downbox for active day
		if (i==(uint)active_date_.day()-1)
			btn->box(FL_DOWN_BOX);
		else
			btn->box(FL_FLAT_BOX);

		day_of_week++;
		if (day_of_week==7) { day_of_week=0; row++; }
	}
	
	// Hide remaining buttons
	for (i=d.days_in_month(); i<31; i++)
		m_dayButtons[i]->hide();
	
	// Show/hide second filler
	if (day_of_week<6)
		m_filler[1]->show();
	else
		m_filler[1]->hide();
		
	// Show/hide third filler
	if (row<7)
		m_filler[2]->show();
	else
		m_filler[2]->hide();
	
	// Set title
	static char title[30]; // No month name should be larger than 24 chars, even when localized
			// and we can't show it anyway cause the box is too small
	snprintf (title, 30, "%s, %d", d.month_name(), d.year());
	m_monthNameBox->copy_label(title);
	
	
	// Calculate tooltip (distance between today and active date)
	static char tooltip_str[1024];
	tooltip_str[0] = '\0';
	
	if (today_date_ != active_date_) {
		long dist = date_distance(today_date_, active_date_);
		long weeks = dist/7;
		int wdays = dist%7;
		int months=0;
		int mdays=0;
		int years=0;
		int ymonths=0;
		
		// Find lower date, first part of tooltip
		edelib::Date d1,d2;
		if (today_date_ < active_date_) {
			d1=today_date_;
			d2=active_date_;
			snprintf(tooltip_str, 1023, _("In %ld days"), dist);
		} else {
			d2=today_date_;
			d1=active_date_;
			snprintf(tooltip_str, 1023, _("%ld days ago"), dist);
		}
		
		// If necessary, calculate distance in m/d and y/m/d format
		if (dist>30) {
			months = d2.month() - d1.month() + (d2.year() - d1.year())*12; 
			mdays = d2.day() - d1.day();
			if (mdays<1) {
				mdays += d2.days_in_month();
				months--;
			}
		}
		if (months>11) {
			years = months/12;
			ymonths = months%12;
		}
	
		// Append those strings using snprintf
		if (weeks) {
			char* tmp = strdup(tooltip_str);
			snprintf(tooltip_str, 1023, _("%s\n%ld weeks and %d days"), tmp, weeks, wdays);
			free(tmp);
		}
		
		if (months) {
			char* tmp = strdup(tooltip_str);
			snprintf(tooltip_str, 1023, _("%s\n%d months and %d days"), tmp, months, mdays);
			free(tmp);
		}
		
		if (years) {
			char* tmp = strdup(tooltip_str);
			snprintf(tooltip_str, 1023, _("%s\n%d years, %d months and %d days"), tmp, years, ymonths, mdays);
			free(tmp);
		}
	}
	
	tooltip(tooltip_str);
	
	layout(x(),y(),w(),h()); // relayout buttons if neccessary
	redraw();
}


int EDE_Calendar::handle(int e) {
	// Forward events to switch buttons
	for (int i=0; i<4; i++)
		if (Fl::event_inside(m_switchButtons[i])) return m_switchButtons[i]->handle(e);

	// Accept focus
	if (e==FL_FOCUS) return 1;

	if (e==FL_PUSH || e==FL_ENTER) return 1;
	
	// Click on date to set active, doubleclick to set as today
	if (e==FL_RELEASE) {
		for (int i=0; i<31; i++)
			if (Fl::event_inside(m_dayButtons[i])) {
				edelib::Date d = active_date_;
				d.set(d.year(), d.month(), i+1);
				if (Fl::event_clicks() == 1)
					today_date(d);
				else
					active_date(d);
				return 1;
			}
		take_focus();

	} else if (e==FL_KEYBOARD) {
		int k=Fl::event_key();

		// arrow navigation
		if (k==FL_Up || k==FL_Down || k==FL_Left || k==FL_Right) {
			edelib::Date ad = active_date_;
			int d = ad.day();
			int m = ad.month();
			int y = ad.year();
			
			if (k==FL_Up) d -= 7;
			else if (k==FL_Down) d += 7;
			else if (k==FL_Left) d--;
			else if (k==FL_Right) d++;
			
			if (d < 1) {
				m--;
				if (m<1) {
					y--;
					m+=12;
				}
				d += ad.days_in_month(y,m);
			}
			if (d > ad.days_in_month(y,m)) {
				d -= ad.days_in_month(y,m);
				m++;
				if (m>12) {
					y++;
					m-=12;
				}
			}
			ad.set(y,m,d);
			active_date(ad);
			return 1;
		}
		
		// Return to today with Home
		if (k==FL_Home) {
			active_date(today_date());
			return 1;
		}
		
		// Set active day as today with Space
		if (k==' ') {
			today_date(active_date());
			return 1;
		}

		// Allow moving focus with Tab key
		if (k==FL_Tab) return Fl_Group::handle(e);
	}
	return Fl_Group::handle(e);
}
