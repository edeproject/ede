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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ede-timedate.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Repeat_Button.H>
#define FL_PATH_MAX 1024

#include <edelib/MessageBox.h>
#include <edelib/Run.h>
#include <edelib/StrUtil.h>
#include <edelib/Config.h>
#include <edelib/IconTheme.h>
#include <edelib/Debug.h>
#include <edelib/Window.h>
#include <edelib/Ede.h>


#include "icons/world2.xpm"

static edelib::Window *timedateWindow = (edelib::Window*)0;
EDE_Calendar *calendar=(EDE_Calendar *)0;
Fl_Time_Input *timeBox=(Fl_Time_Input*)0;
Fl_Choice *timeFormat=(Fl_Choice *)0;
Fl_Menu_Item menu_timeFormat[] = {
 {_("12 hour")},
 {_("24 hour")},
 {0}
};

static Fl_Pixmap image_world((const char**)world2_xpm);

Fl_Choice* timeZonesList=(Fl_Choice *)0;
Fl_Button* applyButton;
Fl_Input_Choice* serverList;
Fl_Clock_Output* clk;

// Status variables
bool time_changed, format_changed, tz_changed, date_changed;

// config file for workpanel (time format)
edelib::Config config_file;

const char* zonetab_dir = 0;

// Time servers - all in one string, so that translators can easily add new servers
const char* time_servers = _("International (pool.ntp.org)\nEurope (europe.pool.ntp.org)\nAsia (asia.pool.ntp.org)\nNorth America (north-america.pool.ntp.org)\nAustralia and Oceania (oceania.pool.ntp.org)");




// --------------------------------------------
//  Timezone funcs
// --------------------------------------------



// Return string describing current timezone

const char* get_current_timezone() {
	static char current_tz[100] = "";
	char tmp[100];
	FILE *f;

	if(readlink("/etc/localtime", tmp, sizeof(tmp)-1)>0) {
		char *tz = strstr(tmp, "/zoneinfo/") + strlen("/zoneinfo/");
		strncpy(current_tz, tz, 99);
		current_tz[99]='\0';
//		timeZonesList->value(tz);
//		timeZonesList->text(tz);
	} else {
		// some distros just copy the file instead of symlinking
		// But /etc/sysconfig/clock should contain ZONE=Continent/City
		if((f = fopen("/etc/sysconfig/clock", "r")) != NULL) {
			while (fgets(tmp,99,f) != NULL) {
				// last char is newline, let's strip that:
				if (tmp[strlen(tmp)-1] == '\n')
					tmp[strlen(tmp)-1] = '\0';
				if (strstr(tmp,"ZONE=") == tmp) {
					strncpy(current_tz, tmp+5, 99);
					current_tz[99]='\0';
//					timeZonesList->value(tempstring+5);
//					timeZonesList->text(tempstring+5);
				}
			}
		} else {
//			timeZonesList->value(_("Zone information not found."));
			timeZonesList->add(_("Zone information not found."));
		}
	}
	return current_tz;
}


// Fill timeZonesList widget with time zones

void fill_timezones(const char* current) {
	FILE *f;
	char tempstring[101] = "";
	
	struct stat s;
	if(stat("/usr/share/zoneinfo/zone.tab",&s)==0) {
		edelib::run_sync("cat /usr/share/zoneinfo/zone.tab | grep -e  ^[^#] | cut -f 3 |sort > /tmp/_tzone_.txt");
		zonetab_dir = "/usr/share/zoneinfo/";
	}
	else if(stat("/usr/local/share/zoneinfo/zone.tab",&s)==0) {
		edelib::run_sync("cat /usr/local/share/zoneinfo/zone.tab | grep -e  ^[^#] | cut -f 3 | sort > /tmp/_tzone_.txt");
		zonetab_dir = "/usr/local/share/zoneinfo/";
	} else {
		timeZonesList->add(_("Zone information not found."));
		return;
	}
	
	if((f = fopen("/tmp/_tzone_.txt", "r")) != NULL)
	{
		timeZonesList->clear();
		while(fgets(tempstring, 100, f) != NULL)
		{
			
			timeZonesList->add(edelib::str_trim(tempstring));
		}
		fclose(f);
		timeZonesList->value(timeZonesList->find_item(current));
	} else {
		timeZonesList->add(_("Zone information not found."));
	}
	remove("/tmp/_tzone_.txt");
// TODO:    Group::current()->array().sort(sort_f);
}


// Change time zone

char* apply_timezone() {
	static char cmd[FL_PATH_MAX] = "";
	char tz[FL_PATH_MAX];

	if(!zonetab_dir) { // this should be set by fill_timezones
		// although, if there are no timezones, tz_changed should never become true
		edelib::alert(_("Zone information not found."));
		return 0;
	}

	timeZonesList->item_pathname(cmd, sizeof(cmd)-1);
	snprintf(tz, sizeof(tz)-1, "%s%s", zonetab_dir, cmd);
	snprintf(cmd, sizeof(cmd)-1, "rm /etc/localtime; ln -s %s /etc/localtime; ", tz);

	char val[FL_PATH_MAX];
	snprintf(val, sizeof(val)-1, "TZ=%s", tz);
	putenv(val);

	tz_changed=false;
	return cmd;
}




// --------------------------------------------
//  Time format
// --------------------------------------------


// Get current time format

int get_format() {
	if (config_file.load("ede.conf")) {
		char timeformatstr[5];
		config_file.get("Clock", "TimeFormat", timeformatstr, 5);
		if (strcmp(timeformatstr,"24") == 0)
			return 1;
	}
	//Default to 12 hour is there's a problem
	return 0;
}


// Set time format 

void apply_format(int format) {
	if (config_file.load("ede.conf")) {
		if (format == 1)
			config_file.set("Clock", "TimeFormat", "24");
		else
			config_file.set("Clock", "TimeFormat", "12");
		config_file.save("ede.conf");
	}
	format_changed = false;
}


// --------------------------------------------
//  Apply
// --------------------------------------------


char* apply_date_time() {
	static char cmd2[FL_PATH_MAX] = "";

	edelib::Date date = calendar->today_date();
	int mmonth = date.month();
	int mday = date.day();
	int myear = date.year();
	int mhour = 0; //timeBox->hour();
	int mminute = 0; //timeBox->minute();

	snprintf(cmd2, sizeof(cmd2)-1, "date %.2d%.2d%.2d%.2d%.2d", mmonth, mday, mhour, mminute, (myear-2000));

	time_changed=false; 
	date_changed=false;
	return cmd2;
}



// Little helper func
char* strdupcat(char* dest, char* src) {
	if (!src) return dest;
	if (!dest) return strdup(src);
	dest = (char*)realloc(dest, strlen(dest)+strlen(src)+1);
	dest = strcat(dest, src);
	return dest;
}



// Apply all changes

void apply_all() {
	edelib::alert("Not implemented yet");
#if 0
	// Combine results into a single string so that we can run all those commands as root
	char *cmd = 0;

	if (format_changed) apply_format(timeFormat->value()); // don't need root for this

	if (tz_changed) cmd = strdupcat(cmd, apply_timezone());

	if (time_changed || date_changed) cmd = strdupcat (cmd, apply_date_time());

	int ret = 0;
	if (cmd) ret = edelib::run_program(cmd, /*wait=*/true , /*root=*/true );

	if (ret != 0) {
		edelib::MessageBox mb;
		mb.set_theme_icon(MSGBOX_ICON_ERROR); // specified in edelib/MessageBox.h
		mb.set_text(_("Error setting date or time."));
		mb.add_button(_("&Close"));
		mb.set_modal();
		mb.run();
	}

	// Funcs should reset *_changed to false
	else if (!time_changed && !format_changed && !tz_changed && !date_changed)
		applyButton->deactivate();
#endif
}




// --------------------------------------------
//  Synchronize
// --------------------------------------------

void synchronize(const char* server) {
	edelib::alert("Not implemented yet");
#if 0
	char buffer[1024];
	snprintf(buffer, 1024, "ntpdate %s", server);
fprintf(stderr, "run: %s\n", buffer);
	long ret = edelib::run_program(buffer, /*wait=*/true, /*root=*/true);
	if (ret == edelib::RUN_NOT_FOUND)
		edelib::alert(_("Program <b>ntpdate</b> is required for time synchronization feature."));
	else if (ret>=edelib::RUN_USER_CANCELED)
		edelib::alert(_("Internal error: %d."), ret);
	else if (ret != 0)
		edelib::alert(_("ntpdate failed with the following return value: %d\nPlease consult ntpdate manual for details."), ret);
#endif
}

void populate_servers() {
	char* tmp = strdup(time_servers);
	char* server = strtok(tmp, "\n");
	while (server) {
		serverList->add(server);
		server = strtok(NULL, "\n");
	}
	free(tmp);
	serverList->value(0);
}





// --------------------------------------------
//  Callbacks
// --------------------------------------------



static void cb_Apply(Fl_Button*, void*) {
	apply_all();
}

static void cb_Close(Fl_Button*, void*) {
	if (time_changed || format_changed || tz_changed || date_changed) {
		edelib::MessageBox mb;
		mb.set_theme_icon(MSGBOX_ICON_WARNING); // specified in edelib/MessageBox.h
		mb.set_text(_("You have unsaved changes in this window!\nDo you want to close it anyway?"));
		mb.add_button(_("Go &back"));
		mb.add_button(_("&Discard changes"));
		mb.set_modal();
		if (mb.run() != 1) return;
	}
	exit(0);
}

static void cb_timeChanged(Fl_Widget*, void*) {
	clk->value(timeBox->hour(),timeBox->minute(),timeBox->second());
	if (time_changed) return;
	time_changed=true;
	applyButton->activate();
}

static void cb_timeFormatChanged(Fl_Widget*, void*) {
	if (format_changed) return;
	format_changed=true;
	applyButton->activate();
}

static void cb_tzChanged(Fl_Widget*, void*) {
	if (tz_changed) return;
	tz_changed=true;
	applyButton->activate();
}

static void cb_dateChanged(Fl_Widget*, void*) {
	if (date_changed) return;
	date_changed=true;
	applyButton->activate();
}

static void cb_sync(Fl_Widget*, void*) {
	char buffer[1024];
	strncpy(buffer, serverList->value(), 1024);
	buffer[1023]='\0';

	// If part of string is in braces (), take just that
	char* k1 = strchr(buffer,'(');
	char* k2;
	if (k1) k2 = strchr(k1, ')');
	if (k1 && k2) {
		int i=0;
		for (char* p=k1+1; p!=k2; p++, i++)
			buffer[i]=*p;
		buffer[i]='\0';
	}

	synchronize(buffer);
}

static void cb_hour(Fl_Widget*, void*) {
	int k = timeBox->hour()-1;
	if (k==-1) k=23;
	timeBox->hour(k);
	timeBox->do_callback();
}
static void cb_hour1(Fl_Widget*, void*) {
	int k = timeBox->hour()+1;
	if (k==24) k=0;
	timeBox->hour(k);
	timeBox->do_callback();
}
static void cb_minute(Fl_Widget*, void*) {
	int l = timeBox->minute()-1;
	if (l==-1) { 
		l = 59;
		int k = timeBox->hour()-1;
		if (k==-1) k=23;
		timeBox->hour(k);
	}
	timeBox->minute(l);
	timeBox->do_callback();
}
static void cb_minute1(Fl_Widget*, void*) {
	int l = timeBox->minute()+1;
	if (l==60) {
		l = 0;
		int k = timeBox->hour()+1;
		if (k==24) k=0;
		timeBox->hour(k);
	}
	timeBox->minute(l);
	timeBox->do_callback();
}


static void tick(void *v) {
	if (time_changed) {
		clk->value(clk->value()+1-3600);
	} else {
		clk->value(time(0));
		if (timeBox) timeBox->epoch(time(0));
	}
	Fl::add_timeout(1.0, tick, v);
}


// --------------------------------------------
// Main window design
// --------------------------------------------


int main(int argc, char **argv) {
	EDE_APPLICATION("ede-timedate");

	time_changed = format_changed = tz_changed = date_changed = false;

  { timedateWindow = new edelib::Window(415, 320, _("Time and date"));
    { Fl_Tabs* o = new Fl_Tabs(5, 5, 405, 270);
      { Fl_Group* o = new Fl_Group(5, 30, 405, 245, _("&Time/date"));
        { calendar = new EDE_Calendar(10, 35, 220, 202);
          calendar->box(FL_DOWN_BOX);
          calendar->color(FL_BACKGROUND2_COLOR);
          calendar->callback(cb_dateChanged);
        } // EDE_Calendar* calendar
        { clk = new Fl_Clock_Output(235, 35, 170, 177);
          clk->box(FL_DOWN_BOX);
          clk->color(FL_BACKGROUND2_COLOR);
          tick(clk);
        } // Fl_Clock* o
        { Fl_Repeat_Button* o = new Fl_Repeat_Button(235, 212, 25, 23);
          o->callback(cb_hour);
          o->label("@-1<<");
          o->labelcolor(fl_darker(FL_BACKGROUND_COLOR));
          o->selection_color(fl_lighter(FL_BACKGROUND_COLOR));
        } // TimeBox* timeBox
        { Fl_Repeat_Button* o = new Fl_Repeat_Button(260, 212, 25, 23);
          o->callback(cb_minute);
          o->label("@-1<");
          o->labelcolor(fl_darker(FL_BACKGROUND_COLOR));
          o->selection_color(fl_lighter(FL_BACKGROUND_COLOR));
        } // TimeBox* timeBox
        { timeBox = new Fl_Time_Input(285, 212, 70, 23);
          timeBox->box(FL_DOWN_BOX);
          timeBox->callback(cb_timeChanged);
          timeBox->when(FL_WHEN_CHANGED);
          timeBox->epoch(time(0));
        } // TimeBox* timeBox
        { Fl_Repeat_Button* o = new Fl_Repeat_Button(355, 212, 25, 23);
          o->callback(cb_minute1);
          o->label("@-1>");
          o->labelcolor(fl_darker(FL_BACKGROUND_COLOR));
          o->selection_color(fl_lighter(FL_BACKGROUND_COLOR));
        } // TimeBox* timeBox
        { Fl_Repeat_Button* o = new Fl_Repeat_Button(380, 212, 25, 23);
          o->callback(cb_hour1);
          o->label("@-1>>");
          o->labelcolor(fl_darker(FL_BACKGROUND_COLOR));
          o->selection_color(fl_lighter(FL_BACKGROUND_COLOR));
        } // TimeBox* timeBox
        { timeFormat = new Fl_Choice(305, 240, 100, 25, _("Time format:"));
          timeFormat->down_box(FL_BORDER_BOX);
          timeFormat->menu(menu_timeFormat);
          timeFormat->callback(cb_timeFormatChanged);
          timeFormat->value(get_format());
        } // Fl_Choice* timeFormat
        o->end();
      } // Fl_Group* o
      { Fl_Group* o = new Fl_Group(5, 30, 405, 245, _("Time &zones"));
        o->hide();
        { Fl_Group* o = new Fl_Group(15, 40, 385, 190);
          o->box(FL_DOWN_BOX);
          o->color(FL_FOREGROUND_COLOR);
          { Fl_Box* o = new Fl_Box(25, 55, 350, 160);
            o->box(FL_FLAT_BOX);
            o->color(FL_FOREGROUND_COLOR);
            o->image(image_world);
          } // Fl_Box* o
          o->end();
        } // Fl_Group* o
        { timeZonesList = new Fl_Choice(120, 235, 280, 25, _("Time zone:"));
          timeZonesList->down_box(FL_BORDER_BOX);
          timeZonesList->callback(cb_tzChanged);
          fill_timezones(get_current_timezone());
        } // Fl_Choice* timeZonesList
        o->end();
      } // Fl_Group* o
      { Fl_Group* o = new Fl_Group(5, 30, 405, 245, _("&Synchronize"));
        o->hide();
        { Fl_Box* o = new Fl_Box(15, 55, 385, 50, _("Select the server with which you want to synhronize your system clock:"));
          o->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_WRAP);
          o->box(FL_NO_BOX);
        } // Fl_Box* o
        { serverList = new Fl_Input_Choice(15, 115, 250, 25, "");
          populate_servers();
        } // Fl_Input_Choice* serverList
        { Fl_Button* o = new Fl_Button(55, 150, 90, 25, _("Synchronize"));
          o->callback(cb_sync);
        }
      } // Fl_Group* o
      o->end();
    } // Fl_Tabs* o
    { Fl_Group* o = new Fl_Group(5, 285, 405, 25);
      { applyButton = new Fl_Button(220, 285, 90, 25, _("&Apply"));
        applyButton->tooltip(_("Set system time"));
        applyButton->callback((Fl_Callback*)cb_Apply);
        applyButton->deactivate();
      } // Fl_Button* applyButton
      { Fl_Button* o = new Fl_Button(320, 285, 90, 25, _("&Close"));
        o->callback((Fl_Callback*)cb_Close);
      } // Fl_Button* o
      o->end();
    } // Fl_Group* o
    timedateWindow->end();
  } // Fl_Double_Window* timedateWindow
  calendar->take_focus();
  timedateWindow->show(argc, argv);
  return Fl::run();
}
