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


#ifndef _FL_TIME_INPUT_H_
#define _FL_TIME_INPUT_H_



#include <FL/Fl_Input.H>
#include <string.h>
#include <stdlib.h>
#include <time.h>


class Fl_Time_Input : public Fl_Input {
	int h,m,s;

	// Parse time into variables h,m,s
	// Return true if time is valid, else returns false
	bool parse_time(const char* time) {
		if (!time) return false;
		h=m=s=0;
		char* p = (char*)time;
		while (*p != ':') {
			if (*p<'0' || *p>'9') return false;
			h=h*10+(*p++-48);
		}
		p++;
		while (*p != ':') {
			if (*p<'0' || *p>'9') return false;
			m=m*10+(*p++-48);
		}
		p++;
		while (*p != '\0') {
			if (*p<'0' || *p>'9') return false;
			s=s*10+(*p++-48);
		}
		if (h>23 || m>59 || s>59) return false;
		return true;
	}

public:
	Fl_Time_Input(int x, int y, int w, int h, const char *l = 0) : Fl_Input(x,y,w,h,l) { value("00:00:00"); }

	void value(const char* l) {
		int p = position(), m = mark();
		if (parse_time(l)) Fl_Input::value(l);
		else Fl_Input::value("00:00:00");
		position(p,m);
	}
	const char* value() { return Fl_Input::value(); }

	int handle(int e) {
		char* oldvalue = strdup(Fl_Input::value());
		int ret = Fl_Input::handle(e);
		if (!parse_time(Fl_Input::value())) Fl_Input::value(oldvalue);
		free(oldvalue);
		return ret;
	}

	int hour() { parse_time(value()); return h; }
	int minute() { parse_time(value()); return m; }
	int second() { parse_time(value()); return s;  }

	void hour(int val) { 
		parse_time(value()); 
		char tmp[10]; 
		snprintf(tmp, 10, "%02d:%02d:%02d", val, m, s); 
		value(tmp); 
	}
	void minute(int val) { 
		parse_time(value()); 
		char tmp[10]; 
		snprintf(tmp, 10, "%02d:%02d:%02d", h, val, s); 
		value(tmp); 
	}
	void second(int val) { 
		parse_time(value()); 
		char tmp[10]; 
		snprintf(tmp, 10, "%02d:%02d:%02d", h, m, val); 
		value(tmp); 
	}

	void epoch(time_t val) {
		struct tm* t = localtime(&val);
		char tmp[10]; 
		snprintf(tmp, 10, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
		value(tmp);
	}
	time_t epoch() {
		time_t ep = time(0);
		struct tm* t = localtime(&ep);
		t->tm_hour = hour();
		t->tm_min = minute();
		t->tm_sec = second();
		return mktime(t);
	}
};


#endif
