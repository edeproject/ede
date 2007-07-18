/***************************************************************************
                          Fl_Date_Time.cpp  -  description
                             -------------------
    begin                : Tue Dec 14 1999
    copyright            : (C) 1999 by Alexey Parshin
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

//#include <config.h>
//#include <edeconfig.h>

/*#include <efltk/Fl_Date_Time.h>
#include <efltk/Fl_Util.h>*/

#include "Fl_Date_Time.h"

// For NLS stuff
//#include "../core/fl_internal.h"
#include "../edelib2/NLS.h"
#include <string.h>

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef _WIN32
# include <sys/time.h>
#else
# include <windows.h>
#endif

static const char *dayname[] = {
   "Sunday", "Monday", "Tuesday", "Wednesday",
   "Thursday", "Friday", "Saturday"
};

static const char *mname[] = {
   "January", "February", "March", "April", "May", "June",
   "July", "August", "September", "October", "November", "December"
};

static const short _monthDays[2][12] = {
   {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} //Leap year
};

static const short _monthDaySums[2][12] = {
   {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
   {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335} //Leap year
};

#define DateDelta 693594

char Fl_Date_Time::dateInputFormat[32];
char Fl_Date_Time::timeInputFormat[32];
char Fl_Date_Time::dateFormat[32];
char Fl_Date_Time::datePartsOrder[4];
char Fl_Date_Time::timeFormat[32];
bool Fl_Date_Time::time24Mode;
char Fl_Date_Time::dateSeparator;
char Fl_Date_Time::timeSeparator;
double Fl_Date_Time::dateTimeOffset;

static void upperCase(char *dest, const char *src, int dest_len) {
   int i = 0;
   int len = strlen(src);
   if(len>dest_len) len=dest_len; //No buffer overflow.
   for (; i < len; i++)
      dest[i] = (char)toupper( src[i] );
   dest[i] = '\0';
}

static int trimRight(char *s) {
   int len = strlen(s);
   while( (len--) >= 0) {
      if( (unsigned char)s[len] > 32 ) {
         len++;
         s[len] = '\0';
         break;
      }
   }
   return len;
}

static char parseDateOrTime(char *format,char *dt) {
   char  separator[] = " ";

   // Cut-off trailing non-digit characters
   int   len = strlen(dt);
   char *ptr = dt + len - 1;
   while (!isdigit(*ptr)) ptr--;
   *(ptr+1) = 0;
   // find a separator char
   ptr = dt;
   while (isalnum(*ptr) || *ptr == ' ') ptr++;
   separator[0] = *ptr;
   ptr = strtok(dt,separator);
   strcpy(format,"");

   const char *pattern;
   while (ptr) {
      int number = atoi(ptr);
      switch (number) {
      case 10: pattern = "HH";   // hour (12-hour mode)
               Fl_Date_Time::time24Mode = false;
               break;
      case 22: pattern = "HH";	// hour (24-hour mode)
               Fl_Date_Time::time24Mode = true;
               break;
      case 48: pattern = "NN";	// minute
               break;
      case 59: pattern = "SS";	// second
               break;
      case 17: pattern = "DD";   // day
               strcat(Fl_Date_Time::datePartsOrder,"D");	
               break;
      case 6:  pattern = "MM";   // month
               strcat(Fl_Date_Time::datePartsOrder,"M");	
               break;
      case 2000:
      case 0:  pattern = "YYYY"; // year
               strcat(Fl_Date_Time::datePartsOrder,"Y");	
          break;
      default: pattern = NULL; break;
      }
      if (pattern) {
         strcat(format,pattern);
         strcat(format,separator);
      }
      ptr = strtok(NULL,separator);
   }
   len = strlen(format);
   if (len)
      format[len-1] = 0;

   return separator[0];
}

class Fl_Date_Time_Format {
public:
   Fl_Date_Time_Format();
};

static void buildDateInputFormat() {
   Fl_Date_Time::dateInputFormat[0] = 0;
   char separator[] = { Fl_Date_Time::dateSeparator, 0 };
   for (int i = 0; i < 3; i++) 
   switch (Fl_Date_Time::datePartsOrder[i]) {
   case 'D':   strcat(Fl_Date_Time::dateInputFormat,"39\\");
               strcat(Fl_Date_Time::dateInputFormat,separator);
               break;
   case 'M':   strcat(Fl_Date_Time::dateInputFormat,"19\\");
               strcat(Fl_Date_Time::dateInputFormat,separator);
               break;
   case 'Y':   strcat(Fl_Date_Time::dateInputFormat,"2999\\");
               strcat(Fl_Date_Time::dateInputFormat,separator);
               break;
   }
   int len = strlen(Fl_Date_Time::dateInputFormat);
   if (len) 
      Fl_Date_Time::dateInputFormat[len-2] = 0;
}

static void buildTimeInputFormat() {
   if (Fl_Date_Time::time24Mode)
         strcpy(Fl_Date_Time::timeInputFormat,"29\\:59");
   else  strcpy(Fl_Date_Time::timeInputFormat,"19\\:59T\\M");
}

Fl_Date_Time_Format::Fl_Date_Time_Format() {
   char  dateBuffer[32];
   char  timeBuffer[32];
   // make a special date and time - today :)
   struct tm t;
   t.tm_year = 100;	// since 1900, -> 2000
   t.tm_mon  = 5;  	// June (January=0)
   t.tm_mday = 17;
   t.tm_hour = 22;
   t.tm_min  = 48;
   t.tm_sec  = 59;

   t.tm_wday = 0; // Sunday

   // Build local data and time
   strftime(timeBuffer,32,"%X",&t);
   strftime(dateBuffer,32,"%x",&t);

   // Build local date and time formats
   Fl_Date_Time::datePartsOrder[0] = 0;
   Fl_Date_Time::time24Mode = false; // to be determined below
   Fl_Date_Time::dateSeparator = parseDateOrTime(Fl_Date_Time::dateFormat,dateBuffer);
   Fl_Date_Time::timeSeparator = parseDateOrTime(Fl_Date_Time::timeFormat,timeBuffer);
   if (!Fl_Date_Time::time24Mode)
      strcat(Fl_Date_Time::timeFormat,"AM");
   buildDateInputFormat();
   buildTimeInputFormat();
}

// This is the only instance to Fl_Date_Time_Format. 
static Fl_Date_Time_Format dateFormat;

bool Fl_Date_Time::is_leap_year(const short year) {
   return ((year&3) == 0 && year%100 != 0 || year%400 == 0);
}

void Fl_Date_Time::encode_date(double &dt,short year,short month,short day) {
   if (year == 0 && month == 0 && day == 0) {
      dt = 0;
      return;
   }
   if (month < 1 || month > 12) {
      dt = 0;
      return;
   }
   int yearKind = is_leap_year(year);
   if (day < 1 || day > _monthDays[yearKind][month-1]) {
      dt = 0;
      return;
   }      
      
   if (year <= 0 || year > 9999) {
      dt = 0;
      return;
   }

   day += _monthDaySums[yearKind][month-1];
   int i = year - 1;
   dt = i * 365 + i / 4 - i / 100 + i / 400 + day - DateDelta;
}

void Fl_Date_Time::encode_date(double &dt,const char *dat) {
   char     bdat[64];
   short    datePart[7], partNumber = 0;
   char     *ptr = NULL;
   int i;

   memset(datePart,0,sizeof(datePart));
   upperCase(bdat, dat, sizeof(bdat));

   if (strcmp(bdat,"TODAY") == 0) {
      dt = Date();        // Sets the current date
      return;
   } else {
      int len = strlen(bdat);
      for(i = 0; i <= len && partNumber < 7; i++) {
         char c = bdat[i];
         if (c == dateSeparator || c == timeSeparator || c == ' ' || c == 0) {
            if (c == timeSeparator && partNumber < 3) partNumber = 3;
            if (ptr) { // end of token
               bdat[i] = 0;
               datePart[partNumber] = (short)atoi(ptr);
               partNumber++;
               ptr = NULL;
            }
         } else {
            if (!isdigit(c)) {
               dt = 0;
               return;
            }
            if (!ptr) ptr = bdat + i;
         }
      }
      if (partNumber < 3) { // Not enough date parts
         dt = 0;
         return;
      }
      short month=0, day=0, year=0;
      for(i = 0; i < 3; i++)
      switch (datePartsOrder[i]) {
      case 'M': month = datePart[i]; break;
      case 'D': day   = datePart[i]; break;
      case 'Y': year  = datePart[i]; break;
      }
      if (year < 100) {
         if (year < 35) year = short(year + 2000);
         else           year = short(year + 1900);
      }
      double dd;
      encode_date(dd,year,month,day);
      if (partNumber > 3) { // Time part included into string
         double d;
         encode_time(d,datePart[3],datePart[4],datePart[5],datePart[6]);
         dd += d;
      }
      dt = dd;

   }
}

void Fl_Date_Time::encode_time(double& dt,short h,short m,short s,short ms) {
   dt = (h + ((m + (s + ms / 100.0) / 60.0) / 60.0)) / 24.0;
}

void Fl_Date_Time::encode_time(double& dt,const char *tim) {
   char  bdat[32];
   short timePart[4] = { 0, 0, 0, 0},
         partNumber = 0;
   char  *ptr = NULL;
   bool  afternoon = false;

   upperCase(bdat, tim, sizeof(bdat));

   if (!trimRight(bdat)) {
      dt = 0;
      return;
   }

   if (strcmp(bdat,"TIME") == 0) {
      dt = Time();        // Sets the current date
      return;
   } else {
      char *p = strstr(bdat,"AM");
      if (p) {
         *p = 0;
      } else {
         p = strstr(bdat,"PM");
         if (p) {
            *p = 0;
            afternoon = true;
         }
      }
      trimRight(bdat);
      int len = strlen(bdat);
      for (int i = 0; i <= len && partNumber < 4; i++) {
         char c = bdat[i];
         if (c == timeSeparator || c == ' ' || c == '.' || c == 0) {
            if (ptr) { // end of token
               bdat[i] = 0;
               timePart[partNumber] = (short) atoi(ptr);
               partNumber++;
               ptr = NULL;
            }
         } else {
            if (!isdigit(c)) {
               dt = 0;
               return;
            }
            if (!ptr) ptr = bdat + i;
         }
      }
      if (afternoon && timePart[0] != 12)
         timePart[0] = short(timePart[0] + 12);
      encode_time(dt,timePart[0],timePart[1],timePart[2],timePart[3]);
   }
}

const int S1 = 24 * 60 * 60; // seconds in 1 day

void Fl_Date_Time::decode_time(const double dt,short& h,short& m,short& s,short& ms) {
   double t = dt - (int) dt;

   int secs = int(t * S1 + 0.5);
   h = short(secs / 3600);
   secs = secs % 3600;
   m = short(secs / 60);
   secs = secs % 60;
   s = short(secs);
   ms = 0;
}

const int D1 = 365;              // Days in 1 year
const int D4 = D1 * 4 + 1;       // Days in 4 years
const int D100 = D4 * 25 - 1;    // Days in 100 years
const int D400 = D100 * 4 + 1;   // Days in 400 years

static void DivMod(int op1, int op2, int& div, int& mod) {
   div = op1 / op2;
   mod = op1 % op2;
}

void Fl_Date_Time::decode_date(const double dat, short& year, short& month, short& day) {
   int Y, M, D, I;
   int T = (int) dat + DateDelta;

   T--;
   Y = 1;
   while (T >= D400) {
      T -= D400;
      Y += 400;
   }

   DivMod(T, D100, I, D);
   if (I == 4) {
      I--;
      D += D100;
   }

   Y += I * 100;
   DivMod(D, D4, I, D);
   Y += I * 4;
   DivMod(D, D1, I, D);
   if (I == 4) {
      I--;
      D += D1;
   }
   Y += I;
   year = Y;
   //year  = short (Y + 1900);

   int leapYear = is_leap_year(short(year));
   for (M = 0;;M++) {
      I = _monthDays[leapYear][M];
      if (D < I)
         break;
      D -= I;
   }

   month = short (M + 1);
   day   = short (D + 1);
}

//----------------------------------------------------------------
// Constructors
//----------------------------------------------------------------
Fl_Date_Time::Fl_Date_Time (short year,short month,short day,short hour,short minute,short second) {
   double t;
   int i;
   // NLS stuff
   for (i=0; i<7;i++) dayname[i]=_(dayname[i]);
   for (i=0; i<12;i++) mname[i]=_(mname[i]);   
  
   encode_date(m_dateTime,year,month,day);
   encode_time(t,hour,minute,second);
   m_dateTime += t;
}

Fl_Date_Time::Fl_Date_Time (const char * dat) {

   int i;
   // NLS stuff
   for (i=0; i<7;i++) dayname[i]=_(dayname[i]);
   for (i=0; i<12;i++) mname[i]=_(mname[i]);   

   char* s1 = strdup(dat);//( Fl_String(dat).trim() );
   // TODO: s1.trim()
   char* s2;

   if (!*dat) {
      m_dateTime = 0;
      return;
   }
   s1 = strtok(s1, " ");
   char* p = strtok(NULL, " ");
   if (p != NULL) {
      s2 = strdup(p+1);
      if (strlen(s2)>21) s2[21] = '\0';
      // TODO: s2.trim()
      // s1[p] = '\0'; - strtok() already did that
   }
   if ( strchr(s1,dateSeparator) ) {
      encode_date(m_dateTime, s1);
      if ( strchr(s2,timeSeparator) ) {
         double dt;
         encode_time(dt, s2);
         m_dateTime += dt;
      }
   }
   else  encode_time(m_dateTime, s1);
}

Fl_Date_Time::Fl_Date_Time (const Fl_Date_Time &dt) {
   int i;
   // NLS stuff
   for (i=0; i<7;i++) dayname[i]=_(dayname[i]);
   for (i=0; i<12;i++) mname[i]=_(mname[i]);   

   m_dateTime = dt.m_dateTime;
}

Fl_Date_Time::Fl_Date_Time (const double dt) {
   int i;
   // NLS stuff
   for (i=0; i<7;i++) dayname[i]=_(dayname[i]);
   for (i=0; i<12;i++) mname[i]=_(mname[i]);   

   m_dateTime = dt;
}
//----------------------------------------------------------------
// Assignments
//----------------------------------------------------------------
void Fl_Date_Time::operator = (const Fl_Date_Time &dt) {
   m_dateTime = dt.m_dateTime;
}

void Fl_Date_Time::operator = (const char * dat) {
   encode_date(m_dateTime, dat);
}

//----------------------------------------------------------------
// Conversion operations
//----------------------------------------------------------------
// Fl_Date_Time::operator int (void) { return (int) dateTime; }

Fl_Date_Time::operator double (void) const { return m_dateTime; }

//----------------------------------------------------------------
// Date Arithmetic
//----------------------------------------------------------------
Fl_Date_Time Fl_Date_Time::operator + (int i) {
    return Fl_Date_Time(m_dateTime + i);
}

Fl_Date_Time Fl_Date_Time::operator - (int i) {
    return Fl_Date_Time(m_dateTime - i);
}

Fl_Date_Time Fl_Date_Time::operator + (Fl_Date_Time& dt) {
    return Fl_Date_Time(m_dateTime + dt.m_dateTime);
}

Fl_Date_Time Fl_Date_Time::operator - (Fl_Date_Time& dt) {
    return Fl_Date_Time(m_dateTime - dt.m_dateTime);
}

Fl_Date_Time& Fl_Date_Time::operator += (int i) {
    m_dateTime += i;
    return *this;
}

Fl_Date_Time& Fl_Date_Time::operator -= (int i) {
    m_dateTime -= i;
    return *this;
}

Fl_Date_Time& Fl_Date_Time::operator += (Fl_Date_Time& dt) {
    m_dateTime += dt.m_dateTime;
    return *this;
}

Fl_Date_Time& Fl_Date_Time::operator -= (Fl_Date_Time& dt) {
    m_dateTime -= dt.m_dateTime;
    return *this;
}

Fl_Date_Time& Fl_Date_Time::operator ++() {
    m_dateTime += 1;
    return *this;
}

Fl_Date_Time &Fl_Date_Time::operator ++(int) {
    m_dateTime += 1;
    return *this;
}

Fl_Date_Time &Fl_Date_Time::operator --() {
    m_dateTime -= 1;
    return *this;
}

Fl_Date_Time &Fl_Date_Time::operator --(int) {
    m_dateTime -= 1;
    return *this;
}

//----------------------------------------------------------------
// Format routine
//----------------------------------------------------------------
void Fl_Date_Time::format_date (char *str) const {
   char *ptr = str;
   short month, day, year;

   if (m_dateTime == 0) {
      *str = 0;
      return;
   }
   decode_date(m_dateTime,year,month,day);
   for (int i = 0; i < 3; i++) {
      switch (datePartsOrder[i]) {
      case 'M':   sprintf(ptr,"%02i%c",month,dateSeparator);
                  break;
      case 'D':   sprintf(ptr,"%02i%c",day,dateSeparator);
                  break;
      case 'Y':   sprintf(ptr,"%04i%c",year,dateSeparator);
                  break;
      }
      ptr += strlen(ptr);
   }
   *(ptr-1) = 0;
}

void Fl_Date_Time::format_time (char *str,bool ampm) const {
   short h,m,s,ms;

   if (m_dateTime == 0) {
      *str = 0;
      return;
   }
   decode_time(m_dateTime,h,m,s,ms);
   if (ampm) {
      char format[] = "%02i%c%02iAM";
      if (h > 11) format[10] = 'P';
      sprintf(str,format,h%12,timeSeparator,m);
   }
   else  sprintf(str,"%02i%c%02i%c%02i",h,timeSeparator,m,timeSeparator,s);
}
//----------------------------------------------------------------
//  Miscellaneous Routines
//----------------------------------------------------------------
short Fl_Date_Time::day_of_year( void ) const {
    Fl_Date_Time temp( 1, 1, year() );

    return (short) (m_dateTime - temp.m_dateTime);
}

Fl_Date_Time Fl_Date_Time::convert(const long tt) {
   struct tm *t = localtime((time_t*)&tt);
   double dat,tim;
   encode_date(dat,short(t->tm_year+1900),short(t->tm_mon+1),short(t->tm_mday));
   encode_time(tim,short(t->tm_hour),short(t->tm_min),short(t->tm_sec),short(0));
   return dat + tim;
}

#ifdef _WIN32
#define FILETIME_1970 0x019db1ded53e8000
const BYTE DWLEN = sizeof(DWORD) * 8;
/* Code ripped from some xntp implementation on http://src.openresources.com. */
long get_usec()
{
  FILETIME ft;
  __int64 msec;
  GetSystemTimeAsFileTime(&ft);
  msec = (__int64) ft.dwHighDateTime << DWLEN | ft.dwLowDateTime;
  msec = (msec - FILETIME_1970) / 10;
  return (long) (msec % 1000000);
}
#endif

// Get the current system time
Fl_Date_Time Fl_Date_Time::System() {
   time_t tt;
   time(&tt);
   double datetime = convert(tt);
#ifndef _WIN32
   timeval tp;
   gettimeofday(&tp,0L);
   double mcsec = tp.tv_usec / 1000000.0 / (3600 * 24);
#else
   // This works now!
   double mcsec = get_usec() / 1000000.0 / (3600 * 24);
#endif
   return datetime + mcsec;
}

// Get the current system time with optional synchronization offset
Fl_Date_Time Fl_Date_Time::Now() {
   return (double)Fl_Date_Time::System() + Fl_Date_Time::dateTimeOffset;
}

// Set the synchronization offset
void Fl_Date_Time::Now(Fl_Date_Time dt) {
   Fl_Date_Time::dateTimeOffset = (double)dt - (double)Fl_Date_Time::System(); 
}

Fl_Date_Time Fl_Date_Time::Date() {
   double dat = Now();
   return double(int(dat));
}

Fl_Date_Time Fl_Date_Time::Time() {
   double dat = Now();
   return dat - int(dat);
}

short Fl_Date_Time::days_in_month() const {
   short y, m, d;
   decode_date(m_dateTime,y,m,d);
   return _monthDays[is_leap_year(y)][m-1];
}

unsigned Fl_Date_Time::date() const {
   return unsigned(m_dateTime);
}

short Fl_Date_Time::day() const {
   short y, m, d;
   decode_date(m_dateTime,y,m,d);
   return d;
}

short Fl_Date_Time::month() const {
   short y, m, d;
   decode_date(m_dateTime,y,m,d);
   return m;
}

short Fl_Date_Time::year() const {
   short y, m, d;
   decode_date(m_dateTime,y,m,d);
   return y;
}

short Fl_Date_Time::day_of_week (void) const {
   return short((int(m_dateTime) - 1) % 7 + 1);
}

char* Fl_Date_Time::day_name (void) const {
   return strdup(dayname[day_of_week() - 1]);
}

char* Fl_Date_Time::month_name() const {
   return strdup(mname[month()-1]);
}

char* Fl_Date_Time::date_string() const {	
   char  buffer[32];
   format_date(buffer);
   return strdup(buffer);
}

char* Fl_Date_Time::time_string() const {
   char  buffer[32];
   format_time(buffer,!time24Mode);
   return strdup(buffer);
}

void Fl_Date_Time::decode_date(short *y,short *m,short *d) const {
   decode_date(m_dateTime,*y,*m,*d);
}

void Fl_Date_Time::decode_time(short *h,short *m,short *s,short *ms) const {
   decode_time(m_dateTime,*h,*m,*s,*ms);
}

