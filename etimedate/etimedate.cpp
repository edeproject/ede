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

#include "etimedate.h"
//#include <edeconf.h>
#include <fltk/SharedImage.h>
#include <fltk/xpmImage.h>
#include <fltk/run.h>
#include <fltk/ask.h>
#include <fltk/Item.h>

#include "../edelib2/NLS.h"
#include "../edelib2/Run.h"

// graphics
#include "icons/world2.xpm"

using namespace fltk;
using namespace edelib;



static Window* timedateWindow;
TimeBox* timeBox;
EDE_Calendar* calendar;
InputBrowser* timeZonesList;

Button* applyButton;
bool time_changed = false;
bool date_changed = false;
bool tz_changed = false;

int phour, pminute;

// Constants



// --------------------------------------------
// timezone functions

// From efltk/filename.h
#define FL_PATH_MAX     1024
// end filename.h


// wait - whether to wait for process to finish

// end Util.cpp
#include <unistd.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <sys/stat.h> 

void getCurrentTimeZone()
{
	char szZone[100],tempstring[101];
	FILE *f;

	if(readlink("/etc/localtime", szZone, sizeof(szZone)-1)>0) {
		char *tz = strstr(szZone, "/zoneinfo/") + strlen("/zoneinfo/");
//		timeZonesList->value(tz);
		timeZonesList->text(tz);
	} else {
		// some distros just copy the file instead of symlinking
		// But /etc/sysconfig/clock should contain ZONE=Continent/City
		if((f = fopen("/etc/sysconfig/clock", "r")) != NULL) {
			while (fgets(tempstring,100,f) != NULL) {
				// last char is newline, let's strip that:
				if (tempstring[strlen(tempstring)-1] == '\n')
					tempstring[strlen(tempstring)-1] = '\0';
				if (strstr(tempstring,"ZONE=") == tempstring) {
//					timeZonesList->value(tempstring+5);
					timeZonesList->text(tempstring+5);
				}
			}
		} else {
//			timeZonesList->value(_("Zone information not found."));
			timeZonesList->text(_("Zone information not found."));
		}
	}
}

static char *zonetab_dir = 0;
void saveTimeZone()
{
}

/*int sort_f(const void *w1, const void *w2) {
    Widget *widget1 = *(Widget**)w1;
    Widget *widget2 = *(Widget**)w2;
    return strcmp(widget1->label(), widget2->label());
}*/

// wstrim() - for trimming characters (used in parser)
// parts of former fl_trimleft and fl_trimright from Fl_Util.cpp
#include <ctype.h>
char* wstrim2(char *string)
{
    char *start;

    if(string == NULL )
        return NULL;

    if (*string) {
        int len = strlen(string);
        if (len) {
            char *p = string + len;
            do {
                p--;
                if ( !isspace(*p) ) break;
            } while ( p != string );
            
            if ( !isspace(*p) ) p++;
            *p = 0;
        }
    }
    
    for(start = string; *start && isspace (*start); start++);
    memmove(string, start, strlen(start) + 1);

    return string;
}


void fillTimeZones()
{
    // This funtion is very lame :)

    FILE *f;
    char tempstring[101] = "Unknown";

    struct stat s;
    if(stat("/usr/share/zoneinfo/zone.tab",&s)==0) {
        run_program("cat /usr/share/zoneinfo/zone.tab | grep -e  ^[^#] | cut -f 3 |sort > /tmp/_tzone_.txt");
        zonetab_dir = "/usr/share/zoneinfo/";
    }
    else if(stat("/usr/local/share/zoneinfo/zone.tab",&s)==0) {
        run_program("cat /usr/local/share/zoneinfo/zone.tab | grep -e  ^[^#] | cut -f 3 | sort > /tmp/_tzone_.txt");
        zonetab_dir = "/usr/local/share/zoneinfo/";
    } else {
        Item *o = new Item(_("Zone information not found."));
        o->textcolor(RED);
        return;
    }

    if((f = fopen("/tmp/_tzone_.txt", "r")) != NULL)
    {
        while(fgets(tempstring, 100, f) != NULL)
        {
            Item *o = new Item();
            o->copy_label(wstrim2(tempstring));
        }
        fclose(f);
    } else {
        Item *o = new Item(_("Zone information not found."));
        o->textcolor(RED);
        return;
    }
    remove("/tmp/_tzone_.txt");
// TODO:    Group::current()->array().sort(sort_f);
}


// --------------------------------------------
// Callback functions

void applyAll() {
	char cmd1[FL_PATH_MAX];
	char cmd2[FL_PATH_MAX];
	cmd1[0]='\0'; cmd2[0]='\0';

	if (tz_changed) { 
		if(!zonetab_dir) {
			alert(_("Zone information not found."));
			return;
		}
	
		char tz[FL_PATH_MAX];
//		snprintf(tz, sizeof(tz)-1, "%s%s", zonetab_dir, timeZonesList->value());
		snprintf(tz, sizeof(tz)-1, "%s%s", zonetab_dir, timeZonesList->text());
	
		snprintf(cmd1, sizeof(cmd1)-1, "rm /etc/localtime; ln -s %s /etc/localtime; ", tz);
	
		char val[FL_PATH_MAX];
		snprintf(val, sizeof(val)-1, "TZ=%s", tz);
		putenv(val);

		tz_changed=false;
	}
	if (time_changed || date_changed) {
		Fl_Date_Time date = calendar->date();
		int mmonth = date.month();
		int mday = date.day();
		int myear = date.year();
		int mhour = timeBox->hour();
		int mminute = timeBox->minute();

		snprintf(cmd2, sizeof(cmd2)-1, "date %.2d%.2d%.2d%.2d%.2d", mmonth, mday, mhour, mminute, myear);

		time_changed=false; 
		date_changed = false;
	}
	char *cmd = (char*)malloc(FL_PATH_MAX*2);
	
	// Now run everything as root
	strcpy(cmd,cmd1);
	strcat(cmd,cmd2);
	run_program(cmd, true, true, false);
}

static void cb_OK(Button*, void*) {
	applyAll();
	exit(0);
}

static void cb_Apply(Button*, void*) {
	const char *pwd = password(_("This program requires administrator privileges.\nPlease enter your password below:"),0,"somebody");
printf ("Pwd: %s\n",pwd);
exit(0);
	applyAll();
	applyButton->deactivate();
}

static void cb_Close(Button*, void*) {
	if (date_changed || time_changed) {
		int answer = choice_alert(_("You have unsaved changes in this window!\nDo you want to close it anyway?"), 0, _("Go &Back"), _("&Discard Changes"));
		if (answer != 2) return;
	}
	exit(0);
}

static void cb_tzChanged(Widget*, void*) {
	if (tz_changed) return;
	tz_changed=true;
	applyButton->activate();
}

static void cb_dateChanged(Widget*, void*) {
	if (date_changed) return;
	date_changed=true;
	applyButton->activate();
}

static void cb_timeChanged(TimeBox* w, void*) {
	if (time_changed) return;
	time_changed=true;
	applyButton->activate();
}

// --------------------------------------------
// Main window design

int main (int argc, char **argv) {
  Window* w;
  //fl_init_locale_support("etimedate", PREFIX"/share/locale");
   {Window* o = timedateWindow = new Window(435, 300, _("Time and date"));
    w = o;
    o->begin();
     {TabGroup* o = new TabGroup(10, 10, 415, 245);
       o->begin();
       {Group* o = new Group(0, 25, 415, 220, _("Time and date"));
	 o->begin();
         {Group* o = new Group(10, 10, 220, 200);
          o->box(DOWN_BOX);
          o->color((Color)7);
	  o->begin();
           {EDE_Calendar* o = calendar = new EDE_Calendar(10, 10, 200, 200);
            //o->textfont(fonts+9); // TODO: what does this mean!?
            o->color((Color)0xffffff00);
            o->textcolor((Color)18);
            o->labelsize(10);
            o->textsize(14);
            o->callback((Callback*)cb_dateChanged);
          }
          o->end();
        }
        {TimeBox* o = timeBox = new TimeBox(240, 10, 165, 200);
           timeBox->callback((Callback*)cb_timeChanged);
        }
        o->end();
      }
       {Group* o = new Group(0, 25, 415, 220, _("Timezones"));
        o->hide();
	o->begin();
         {Group* o = new Group(10, 10, 395, 170);
          o->box(DOWN_BOX);
          o->color((Color)0x7b00);
	  o->begin();
           {InvisibleBox* o = new InvisibleBox(0, 0, 395, 170);
	    xpmImage* i = new xpmImage((const char**)world2_xpm);
//	    i->draw(Rectangle(10,5,350,160));
//	    i->over(Rectangle(0,0,i->w(),i->h()),*o);
            o->image(i);
            o->box(FLAT_BOX);
            o->color((Color)0x8000);
          }
          o->end();
        }
         {InputBrowser* o = timeZonesList = new InputBrowser(10, 185, 395, 25); o->begin();
          o->type(1); fillTimeZones();
          getCurrentTimeZone();
          o->end();
	  o->callback((Callback*)cb_tzChanged);
        }
        o->end();
      }
      o->end();
    }
     {Group* o = new Group(0, 265, 415, 33);
       o->begin();
//       {Button* o = new Button(154, 0, 90, 25, _("&OK"));
//        o->callback((Callback*)cb_OK);
//      }
       {Button* o = applyButton = new Button(235, 0, 90, 25, _("&Apply"));
        o->callback((Callback*)cb_Apply);
//        o->tooltip(_("Set system time. ->Just root user!<-"));
	o->deactivate();
      }
       {Button* o = new Button(335, 0, 90, 25, _("&Close"));
        o->callback((Callback*)cb_Close);
      }
      o->end();
    }
    o->end();
  }
  w->show(argc, argv);
  return  run();
}
