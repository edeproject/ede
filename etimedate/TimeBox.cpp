// TimeBox.cxx
// Class that displays a clock with ability to set time
// Part of Equinox Desktop Environment (EDE)
//
// Based on Fl_Time.cxx
// Copyright (C) 2000 Softfield Research Ltd.
//
// Changes 02/09/2001 by Martin Pekar
// Ported to FLTK2 and redesigned by Vedran Ljubovic <vljubovic@smartnet.ba>, 2005.
//
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

// TODO: Update from clock example in latest fltk2 test

// Note V.Lj.: I don't think we need all of this, some code could be
// safely removed

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <fltk/Item.h>
#include <fltk/ask.h>
#include "../edelib2/NLS.h"
#include <fltk/filename.h>
#include <fltk/events.h>

#include "etimedate.h"
#include "TimeBox.h"


// Constructor

TimeBox::TimeBox(int x, int y, int w, int h, char *l) : fltk::Group(x, y, w, h, l)
{
	// Basic layout of TimeBox
	// TODO: this should be part of draw() so that the widget could be resizable
	int button_width; // button_height=button_width
	int realx,realy,realh,realw; //realh = realw + realw/7 (buttons)
	
	if (h >= w * 8/7) {
		realx = x;
		realy = y + (h-(int)(w*8/7))/2;
		realw = w;
		realh = (int)w * 8/7;
	} else {
		realx = x + (w-(int)(h*7/8))/2;
		realy = y;
		realw = (int)h * 7/8;
		realh = h;
	}
 	button_width = (int)(realw/7);
   
	clock = new fltk::ClockOutput(realx, realy, realw, realw);

	input_time = new fltk::Input(realx, realy+realw, realw - button_width * 4, button_width, 0);
	input_time->callback(input_changed_cb, this);
	input_time->when(fltk::WHEN_CHANGED);
//    input_time->textsize(12);

	button_decrease_hour = new fltk::Button(realx + realw - 4 * button_width, realy+realw + 2, button_width, button_width-4, "@-1<<");
	button_decrease_hour->callback(button_cb, this);
	button_decrease_hour->labeltype(fltk::SYMBOL_LABEL);
	button_decrease_hour->box(fltk::THIN_UP_BOX);
//    button_decrease_hour->labelcolor(buttoncolor());
//    button_decrease_hour->highlight_color(fltk::color_average(buttoncolor(), fltk::GRAY, .5f)); 
	button_decrease_hour->highlight_color(fltk::lerp(buttoncolor(),fltk::WHITE,.67f));	// ex. fl_lighter()

	button_increase_hour = new fltk::Button(realx + realw - 3 * button_width, realy+realw + 2, button_width, button_width-4, "@-1>>");
	button_increase_hour->callback(button_cb, this);
	button_increase_hour->labeltype(fltk::SYMBOL_LABEL);
	button_increase_hour->box(fltk::THIN_UP_BOX);
//    button_increase_hour->labelcolor(buttoncolor());
//    button_increase_hour->highlightcolor(fltk::color_average(buttoncolor(), fltk::GRAY, .5f)); 
	button_increase_hour->highlight_color(fltk::lerp(buttoncolor(),fltk::WHITE,.67f));	// ex. fl_lighter()
    
	button_decrease_minute = new fltk::Button(realx + realw - 2 * button_width, realy+realw + 2, button_width, button_width-4, "@-1<");
	button_decrease_minute->callback(button_cb, this);
	button_decrease_minute->labeltype(fltk::SYMBOL_LABEL);
	button_decrease_minute->box(fltk::THIN_UP_BOX);
//    button_decrease_minute->labelcolor(buttoncolor());
//    button_decrease_minute->highlight_color(fltk::color_average(buttoncolor(), fltk::GRAY, .5f)); 
	button_decrease_minute->highlight_color(fltk::lerp(buttoncolor(),fltk::WHITE,.67f));	// ex. fl_lighter()

	button_increase_minute = new fltk::Button(realx + realw - button_width, realy+realw + 2, button_width, button_width-4, "@-1>");
	button_increase_minute->callback(button_cb, this);
	button_increase_minute->labeltype(fltk::SYMBOL_LABEL);
	button_increase_minute->box(fltk::THIN_UP_BOX);
//    button_increase_minute->labelcolor(buttoncolor());
//    button_increase_minute->highlight_color(fltk::color_average(buttoncolor(), fltk::GRAY, .5f)); 
	button_increase_minute->highlight_color(fltk::lerp(buttoncolor(),fltk::WHITE,.67f));	// ex. fl_lighter()

	end();

//type(fltk::TIME_12HOUR);
	// TODO: read this from current locale
	type(TIMEBOX_24HOUR);
	current_tv = (struct timeval*)malloc(sizeof(struct timeval)-1);
	display_tv = (struct timeval*)malloc(sizeof(struct timeval)-1);
	current_time();
}


// dtor
TimeBox::~TimeBox()
{
	free(current_tv);
	free(display_tv);
}


// Reset the clock to current system time

void TimeBox::current_time()
{
	struct tm* display_time_tm;

	gettimeofday(current_tv, 0);
	display_tv->tv_sec = current_tv->tv_sec;
	display_tv->tv_usec = current_tv->tv_usec;
	display_time_tm = localtime(&current_tv->tv_sec);

	if(type() == TIMEBOX_24HOUR) strftime(time_string, 19, "%2H:%2M", display_time_tm);
	else strftime(time_string, 19, "%2I:%2M %p", display_time_tm);

	input_time->value(time_string);
    
	// Start the clock and display current time
	clock->value(display_tv->tv_sec);
	add_timeout(1.0f);
}


// Update the clock each second

int TimeBox::handle(int event)
{
	switch(event) {
	case fltk::TIMEOUT:
	{
		struct timeval t; gettimeofday(&t, 0);
		clock->value(t.tv_sec);
		float delay = 1.0f-float(t.tv_usec)*.000001f;
		if (delay < .1f || delay > .9f) delay = 1.0f;
		add_timeout(delay);
	}
	break;
	}
	return clock->handle(event);	// right?
}


// This apparently has the side effect of moving the clock relatively to
// the current time, e.g
//         system time  TimeBox
// before     1:00        1:02
// after      1:03        1:05
// etc.

void TimeBox::refresh()
{
    long different;

    if (valid())
    {
        different = - display_tv->tv_sec + current_tv->tv_sec;
        gettimeofday(current_tv, 0);

        display_tv->tv_sec = current_tv->tv_sec - different;
	redisplay();
    }
}


// Update the text input field

void TimeBox::redisplay()
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);

    if(type() == TIMEBOX_24HOUR) strftime(time_string, 19, "%2H:%2M", display_time_tm);
    else strftime(time_string, 19, "%2I:%2M %p", display_time_tm);

    input_time->value(time_string);
}


// Stop the clock and update it (move hands)

void TimeBox::move_hands() 
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);

    remove_timeout();
    clock->value(display_tv->tv_sec);
}


int TimeBox::hour()
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);
    return display_time_tm->tm_hour;
}


int TimeBox::minute()
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);
    return display_time_tm->tm_min;
}


void TimeBox::hour(int value)
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);
    display_time_tm->tm_hour = value;
    display_tv->tv_sec = mktime(display_time_tm);
}


void TimeBox::minute(int value)
{
    struct tm *display_time_tm;

    display_time_tm = localtime(&display_tv->tv_sec);
    if(value < 0)
    {
        display_time_tm->tm_min = 59;
	display_time_tm->tm_hour--;
    }
    else if(value >= 0 && value <= 59)
    {
        display_time_tm->tm_min = value;
    }
    else if (value > 59)
    {
        display_time_tm->tm_min = 0;
	display_time_tm->tm_hour++;
    }
    display_time_tm->tm_sec = 0;
    display_tv->tv_sec = mktime(display_time_tm);
}


void TimeBox::settime()
{
    struct tm *display_time_tm;
    display_time_tm = localtime(&display_tv->tv_sec);
    //  return display_time_tm->tm_min;
    time_t ct = mktime (display_time_tm);
    if (stime(&ct)!=0)
        fltk::alert(_("Error setting time. You are probably not superuser!"));
}


void TimeBox::value(int h, int m)
{
    hour(h);
    minute(m);
}


bool TimeBox::valid()
{
    int h, m;
    char a[5];

    if(type() == TIMEBOX_12HOUR)
    {
        if (sscanf(input_time->value(), "%d:%d %s",&h, &m, a) == 3)
        {
            if (h >= 1 && h <= 12 && m >= 0 && m <= 59 && (strcasecmp(a, "am") == 0 || strcasecmp(a, "pm") == 0))
            {
                last_valid = true;
                return true;
            }
        }
    }
    else
    {
        if (sscanf(input_time->value(), "%d:%d",&h, &m) == 2)
        {
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59)
            {
                last_valid = true;
                return true;
            }
        }
    }
    last_valid = false;
    return false;
}


void TimeBox::input_changed_cb(fltk::Widget* widget, void* data)
{
    TimeBox* t = (TimeBox*) data;
    int h, m;
    char a[5];

    if (t->valid())
    {
        if(t->type() == TIMEBOX_12HOUR)
        {
            sscanf(t->input_time->value(), "%d:%d %2s",&h, &m, a);
            if(strcasecmp(a, "am") == 0)
            {
                if(h < 12)
                {
                    t->hour(h);
                }
                else
                {
                    t->hour(0);
                }
            }
            else
            {
                if(h < 12)
                {
                    t->hour(h + 12);
                }
                else
                {
                    t->hour(12);
                }
            }
        }
        else
        {
            sscanf(t->input_time->value(), "%d:%d",&h, &m);
            t->hour(h);
        }
        t->minute(m);
	t->move_hands();
    }
    t->do_callback();
}


void TimeBox::button_cb(fltk::Widget* widget, void* data)
{
    TimeBox* t = (TimeBox*) data;

    if(widget == t->button_decrease_hour)
    {
        t->hour(t->hour()-1);
    }
    if (widget == t->button_decrease_minute)
    {
        t->minute(t->minute()-1);
    }
    if (widget == t->button_increase_minute)
    {
        t->minute(t->minute()+1);
    }
    if (widget == t->button_increase_hour)
    {
        t->hour(t->hour()+1);
    }
    t->redisplay();
    t->move_hands();
    t->do_callback();
}


void TimeBox::textsize(int size)
{
    input_time->textsize(size);
}


void TimeBox::labelsize(int size)
{
    button_decrease_hour->labelsize(size);
    button_decrease_minute->labelsize(size);
    button_increase_minute->labelsize(size);
    button_increase_hour->labelsize(size);
    fltk::Group::labelsize(size);
}


void TimeBox::textfont(fltk::Font* font)
{
    input_time->textfont(font);
}


void TimeBox::labelfont(fltk::Font* font)
{
    button_decrease_hour->labelfont(font);
    button_decrease_minute->labelfont(font);
    button_increase_minute->labelfont(font);
    button_increase_hour->labelfont(font);
    fltk::Group::labelfont(font);
}


void TimeBox::textcolor(fltk::Color color)
{
    input_time->textcolor(color);
}


void TimeBox::labelcolor(fltk::Color color)
{
    button_decrease_hour->labelcolor(color);
    button_decrease_minute->labelcolor(color);
    button_increase_minute->labelcolor(color);
    button_increase_hour->labelcolor(color);
    fltk::Group::labelcolor(color);
}


int TimeBox::textsize()
{
    return (int)input_time->textsize();
}


int TimeBox::labelsize()
{
    return (int)button_decrease_hour->labelsize();
}


fltk::Font* TimeBox::labelfont()
{
    return button_decrease_hour->labelfont();
}


fltk::Font* TimeBox::textfont()
{
    return input_time->textfont();
}


fltk::Color TimeBox::labelcolor()
{
    return button_decrease_hour->labelcolor();
}


fltk::Color TimeBox::textcolor()
{
    return input_time->textcolor();
}
