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


#ifndef _EDE_CALENDAR_H_
#define _EDE_CALENDAR_H_



#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Repeat_Button.H>

#include <edelib/DateTime.h>

#include <stdio.h>

/** EDE_Calendar */
class EDE_Calendar : public Fl_Group {
private:
	static void cbSwitchButtonClicked(Fl_Widget *,long);
	
	Fl_Group	*m_headerBox;
	Fl_Group	*m_buttonBox;
	Fl_Box		*m_monthNameBox;
	Fl_Box		*m_dayNameBoxes[7];
	Fl_Box		*m_dayButtons[31];
	Fl_Repeat_Button *m_switchButtons[4];
	Fl_Box		*m_filler[3];

	edelib::Date  today_date_;
	edelib::Date  active_date_;
	
	// Update color and appearance of boxes and buttons
	void update_calendar();

	// Resize various boxes and buttons
	void layout(int X, int Y, int W, int H);


	// Debugging:
	void printdate(const char* c, edelib::Date d) { fprintf(stderr, "%s: %d.%d.%d\n", c, d.day(), d.month(), d.year()); }

public:
	/** The traditional constructor creates the calendar using the position, size, and label. */
	EDE_Calendar(int x,int y,int w,int h,const char *lbl=0L);

	/** Reset active and current date to system date */
	virtual void reset() {
		today_date_.set(edelib::Date::YearNow, edelib::Date::MonthNow, edelib::Date::DayNow); 
		edelib::Date d1 = active_date_;
		active_date_.set(edelib::Date::YearNow, edelib::Date::MonthNow, edelib::Date::DayNow); 
		if (d1.month()!= edelib::Date::MonthNow || d1.year()!=edelib::Date::YearNow)
			layout(x(),y(),w(),h());
		update_calendar();
	}
	
	/** Change active date */
	void active_date(const edelib::Date d) {
		edelib::Date d1 = active_date_;
		active_date_ = d; 
		// turn calendar to another page
		if (d1.month()!=d.month() || d1.year()!=d.year())
			layout(x(),y(),w(),h());
		update_calendar();
	}
	const edelib::Date active_date() const { return active_date_; }
	
	/** Change today date */
	void today_date(const edelib::Date d) { 
		today_date_ = d; 
		update_calendar(); 
	}
	const edelib::Date today_date() const { return today_date_; }

	// Keyboard and mouse handling
	int handle(int e);

	// Overload resize to call our layout()
	void resize(int X, int Y, int W, int H);

	// Since box colors are based on Calendar color, we need to 
	// overload color changing methods
	void color(Fl_Color c) { Fl_Widget::color(c); update_calendar(); }
	void color(Fl_Color c1, Fl_Color c2) { Fl_Widget::color(c1,c2); update_calendar(); }
	Fl_Color color() { return Fl_Widget::color(); }
};


#endif
