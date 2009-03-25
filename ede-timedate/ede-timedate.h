/*
 * $Id$
 *
 * Application for setting system date, time and local timezone
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#ifndef __EDE_TIMEDATE_H__
#define __EDE_TIMEDATE_H__

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Clock.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>

#include "EDE_Calendar.h"
#include "Fl_Time_Input.h"

//#define Fl_Time_Input Fl_Input


extern EDE_Calendar *calendar;
extern Fl_Time_Input *timeBox;
extern Fl_Choice *timeFormat;
extern Fl_Choice *timeZonesList;
extern Fl_Menu_Item menu_timeFormat[];
extern Fl_Button *applyButton;
extern Fl_Input_Choice* serverList;
extern Fl_Clock_Output* clk;


#endif
