/*
 * Date_Time class for FLTK
 * Copyright (C) Vedran Ljubovic <vljubovic@smartnet.ba>, 2005.
 * WWW: http://www.sourceforge.net/projects/ede
 *
 * This library is distributed under the GNU LIBRARY GENERAL PUBLIC LICENSE
 * version 2. See COPYING for details.
 *
 */

#ifndef _DATE_TIME_H_
#define _DATE_TIME_H_

class DateTime;

/** \class Timezone 
	This class enables various manipulations of timezones needed for the DateTime class.
*/
class Timezone {
public:
/** \fn Timezone::Timezone()
	Creates a variable that contains UTC (Coordinated Universal Time)
*/
	TimeZone ();

/** \fn char* Timezone::place()
	Returns location of current timezone e.g. Europe/Sarajevo
*/
	char* place();

/** \fn void Timezone::parse(char*)
	Parse given string into timezone. String can be place as returned by place() or a part of it. 
*/
	void parse(char*);
	
/** \fn int Timezone::correction(DateTime*)
	Correction i.e. the number of minutes that needs to be added to UTC to get the current time zone. Because of daylight saving, this number can be different during the year - so DateTime needs to be specified. From this number a RFC822-compliant correction can be calculated - e.g. 180 means "+0300".
*/
	int correction(DateTime*);

/** \fn static char** Timezone::list()
	A useful function to return a list of timezone names as used by place() and parse().
*/
	static char** list();
}

/** \class DateTime 
	DateTime 
*/
class DateTime {
public:
/** \fn DateTime::DateTime()
	Creates DateTime at January 1st, year 0, 00:00:00
*/
	DateTime ();

/** \fn DateTime::DateTime(double)
	Creates DateTime with given time_t value (as returned by time())
*/
	DateTime (double);

	int year();
	int month();
	int day();
	int hour();
	int minute();
	int second();

	bool year(int);
	bool month(int);
	bool day(int);
	bool hour(int);
	bool minute(int);
	bool second(int);

	

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

protected:
    double        m_dateTime;
};

// Date comparison
static inline bool operator <  (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 <  (double)dt2 ); }
static inline bool operator <= (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 <= (double)dt2 ); }
static inline bool operator >  (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 >  (double)dt2 ); }
static inline bool operator >= (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 >= (double)dt2 ); }
static inline bool operator == (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 == (double)dt2 ); }
static inline bool operator != (const Fl_Date_Time &dt1, const Fl_Date_Time &dt2) { return ( (double)dt1 != (double)dt2 ); }

#endif
