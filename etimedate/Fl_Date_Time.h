/*
 * $Id$
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

#ifndef _FL_DATE_TIME_H_
#define _FL_DATE_TIME_H_

/*#include "Fl_Export.h"
#include "Fl_String.h"*/

/** Fl_Date_Time */
class Fl_Date_Time {
public:
    static char   dateInputFormat[32];
    static char   timeInputFormat[32];
    static char   dateFormat[32];
    static char   timeFormat[32];
    static char   datePartsOrder[4];
    static char   dateSeparator;
    static char   timeSeparator;
    static bool   time24Mode;

    Fl_Date_Time (short y,short m,short d,short hour=0,short minute=0,short second=0);
    Fl_Date_Time (const char * dat);
    Fl_Date_Time (const Fl_Date_Time &dt);
    Fl_Date_Time (const double dt=0);

    static Fl_Date_Time convert (const long);

    void format_date(char *str) const;
    void format_time(char *str, bool ampm=true) const;

    // These functions don't affect the actual system time.
    // You can only alter the time for the current program.
    static void Now(Fl_Date_Time dt);      // Sets to current date and time
    static Fl_Date_Time System();          // Gets to current system date and time
    static Fl_Date_Time Now();             // Gets to current date and time
    static Fl_Date_Time Date();            // Gets to current date
    static Fl_Date_Time Time();            // Gets to current time

    short days_in_month() const;           // Number of days in month (1..31)
    short day_of_week() const;             // (1..7)
    short day_of_year() const;             // returns relative date since Jan. 1

    char* day_name() const;            // Character Day Of Week ('Sunday'..'Saturday')
    char* month_name() const;          // Character Month name

    unsigned date() const;                 // Numeric date of date object
    short day() const;                     // Numeric day of date object
    short month() const;                   // Month number (1..12)
    short year() const;

    char* date_string() const;
    char* time_string() const;

    void decode_date(short *y,short *m,short *d) const;
    void decode_time(short *h,short *m,short *s,short *ms) const;

    operator double (void) const;

    void operator = (const Fl_Date_Time& date);
    void operator = (const char * dat);

    Fl_Date_Time  operator + (int i);
    Fl_Date_Time  operator - (int i);
    Fl_Date_Time  operator + (Fl_Date_Time& dt);
    Fl_Date_Time  operator - (Fl_Date_Time& dt);

    Fl_Date_Time& operator += (int i);
    Fl_Date_Time& operator -= (int i);
    Fl_Date_Time& operator += (Fl_Date_Time& dt);
    Fl_Date_Time& operator -= (Fl_Date_Time& dt);

    Fl_Date_Time& operator ++ ();     // Prefix increment
    Fl_Date_Time& operator ++ (int);  // Postfix increment
    Fl_Date_Time& operator -- ();     // Prefix decrement
    Fl_Date_Time& operator -- (int);  // Postfix decrement

    static void   decode_date(const double dt,short& y,short& m,short& d);
    static void   decode_time(const double dt,short& h,short& m,short& s,short& ms);
    static void   encode_date(double &dt,short y=0,short m=0,short d=0);
    static void   encode_date(double &dt,const char *dat);
    static void   encode_time(double &dt,short h=0,short m=0,short s=0,short ms=0);
    static void   encode_time(double &dt,const char *tim);
    static bool   is_leap_year(const short year);

protected:
    double        m_dateTime;
    static double dateTimeOffset;    // The offset from current' system time for synchronization
    // with outside system
};

// Date comparison
static inline bool operator <  (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 <  (double)dt2 ); }
static inline bool operator <= (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 <= (double)dt2 ); }
static inline bool operator >  (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 >  (double)dt2 ); }
static inline bool operator >= (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 >= (double)dt2 ); }
static inline bool operator == (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 == (double)dt2 ); }
static inline bool operator != (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 != (double)dt2 ); }

#endif
