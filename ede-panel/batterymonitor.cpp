// Battery monitor for EDE / eworkpanel
// 
// Copyright (c) 2005. EDE Authors
//
// Based in part on xbattbar
// Copyright (c) 1998-2001 Suguru Yamaguchi <suguru@wide.ad.jp>
// http://iplab.aist-nara.ac.jp/member/suguru/xbattbar.html
//
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "batterymonitor.h"

#include "icons/poweron.xpm"
#include "icons/battery.xpm"

#define UPDATE_INTERVAL .5f

void bat_timeout_cb(void *d) {
	((BatteryMonitor*)d)->update_status();
	Fl::repeat_timeout(UPDATE_INTERVAL, bat_timeout_cb, d);
}

BatteryMonitor::BatteryMonitor(Dock*dock)
: Fl_Widget(0,0,16,0)
{
	box(FL_FLAT_BOX);
	//box(FL_BORDER_BOX);
	
	// edit this to show/hide the icon when power is plugged
	m_show_line_on = true;

	// initial values
	m_line_on = true;
	m_undefined = false;
 	m_bat_percentage = m_bat_time = 0;
	m_blink = false;

	m_dock = dock;
	
	m_colors[2] = FL_BLUE;
	m_colors[1] = fl_darker(FL_YELLOW);
	m_colors[0] = FL_RED;
	
	this->color(this->button_color());
	Fl::add_timeout(UPDATE_INTERVAL, bat_timeout_cb, this);
}

BatteryMonitor::~BatteryMonitor() {
//    clear();
}

void BatteryMonitor::clear()
{
/*    if(!cpu) return;

    for (int i=0; i < samples(); i++) {
        delete cpu[i]; cpu[i] = 0;
    }
    delete cpu;
    cpu = 0;

    m_old_samples = -1;*/
}

void BatteryMonitor::draw()
{
	if (m_undefined) {
		// We don't draw anything for undefined state
		m_dock->remove_from_tray(this);
		return;
	}

	if (m_line_on) {
		if (m_show_line_on) {
			this->image(Fl_Image::read_xpm(0, (const char**)poweron_xpm));
			Fl_Widget::draw();
		}
		return;
	}

	this->image(Fl_Image::read_xpm(0, (const char**)battery_xpm));
	int pixels = (int) m_bat_percentage / 10;
	if (pixels > 9) pixels=9;	// if it's really 100%

	// Blinking battery
	if (pixels < 1) {
		if (m_blink) {
			this->image(0);
			m_blink = false;
		} else {
			m_blink = true;
		}
		Fl_Widget::draw();
		return;
	}
	Fl_Widget::draw();

	int color = (int)m_bat_percentage / 34;
	fl_color(m_colors[color]);

	// Some magic constants follow - this is based on design of battery.xpm
	int ddx = box()->dx();
	int ddy = box()->dy();
	fl_line (ddx+7, ddy+15-pixels, ddx+7, ddy+14);
	fl_line (ddx+8, ddy+15-pixels, ddx+8, ddy+14);
	fl_line (ddx+9, ddy+15-pixels, ddx+9, ddy+14);

}

void BatteryMonitor::layout()
{
	update_status();
	Fl_Widget::layout();
}

void BatteryMonitor::update_status()
{
	battery_check();

	// Update tooltip
	char load[255];
	if (m_undefined)
		snprintf(load, sizeof(load)-1, _("Power management not detected"));
	else if (m_line_on)
		snprintf(load, sizeof(load)-1, _("The power is plugged in"));
	else
		snprintf(load, sizeof(load)-1, 
		_("Battery is %d%% full (%d minutes remaining)"), m_bat_percentage, m_bat_time);
	tooltip(load);
}

// -------------------------------
// xbattbar.c,v 1.16.2.4 2001/02/02 05:25:29 suguru
//
// The following code is copied from xbattbar, with small modifications
// These modifications include:
//   - p is now bool instead of int
//   - we try to monitor remaining time, but it only works on linux (for now)
//   - in case of error, m_undefined is set and function isn't executed
// anymore, until eworkpanel restart
//   - signal() is removed - only redraw() is necessary
//   - elapsed_time is removed
//   - in struct apm_info (linux), had to replace const char with char
// -------------------------------

#ifdef __bsdi__

#include <machine/apm.h>
#include <machine/apmioctl.h>

int first = 1;
void battery_check(void)
{
  int fd;
  struct apmreq ar ;


  // No need to display error too many times
  if (m_undefined) return;

  ar.func = APM_GET_POWER_STATUS ;
  ar.dev = APM_DEV_ALL ;

  if ((fd = open(_PATH_DEVAPM, O_RDONLY)) < 0) {
    perror(_PATH_DEVAPM) ;
    m_undefined = true;
    return;
  }
  if (ioctl(fd, PIOCAPMREQ, &ar) < 0) {
    fprintf(stderr, "xbattbar: PIOCAPMREQ: APM_GET_POWER_STATUS error 0x%x\n", ar.err);
  }
  close (fd);

/*  if (first || ac_line != ((ar.bret >> 8) & 0xff)
      || battery_level != (ar.cret&0xff)) {
    first = 0;
    ac_line = (ar.bret >> 8) & 0xff;
    battery_level = ar.cret&0xff;
    redraw();
  }*/

  // Feeble attempt at fixing bsdi
  if (first || battery_level != (ar.cret&0xff)) {
    first = 0;
//    ac_line = (ar.bret >> 8) & 0xff;
    battery_level = ar.cret&0xff;
    m_line_on = false;
    m_bat_time = 0;
    redraw();
  }

}

#endif /* __bsdi__ */

#ifdef __FreeBSD__

#include <machine/apm_bios.h>

#define APMDEV21       "/dev/apm0"
#define APMDEV22       "/dev/apm"

#define        APM_STAT_UNKNOWN        255

#define        APM_STAT_LINE_OFF       0
#define        APM_STAT_LINE_ON        1

#define        APM_STAT_BATT_HIGH      0
#define        APM_STAT_BATT_LOW       1
#define        APM_STAT_BATT_CRITICAL  2
#define        APM_STAT_BATT_CHARGING  3

int first = 1;
void battery_check(void)
{
  int fd, r;
  bool p;
  struct apm_info     info;

  // No need to display error too many times
  if (m_undefined) return;

  if ((fd = open(APMDEV21, O_RDWR)) == -1 &&
      (fd = open(APMDEV22, O_RDWR)) == -1) {
    fprintf(stderr, "xbattbar: cannot open apm device\n");
    m_undefined = true;
    return;
  }
  if (ioctl(fd, APMIO_GETINFO, &info) == -1) {
    fprintf(stderr, "xbattbar: ioctl APMIO_GETINFO failed\n");
    m_undefined = true;
    return;
  }
  close (fd);

  /* get current status */
  if (info.ai_batt_life == APM_STAT_UNKNOWN) {
    switch (info.ai_batt_stat) {
    case APM_STAT_BATT_HIGH:
      r = 100;
      break;
    case APM_STAT_BATT_LOW:
      r = 40;
      break;
    case APM_STAT_BATT_CRITICAL:
      r = 10;
      break;
    default:        /* expected to be APM_STAT_UNKNOWN */
      r = 100;
    }
  } else if (info.ai_batt_life > 100) {
    /* some APM BIOSes return values slightly > 100 */
    r = 100;
  } else {
    r = info.ai_batt_life;
  }

  /* get AC-line status */
  if (info.ai_acline == APM_STAT_LINE_ON) {
    p = true;
  } else {
    p = false;
  }

  if (first || m_line_on != p || m_bat_percentage != r) {
    first = 0;
    m_line_on = p;
    m_bat_percentage = r;
    m_bat_time = 0; //FIXME: battery time on FreeBSD
    redraw();
  }
}

#endif /* __FreeBSD__ */

#ifdef __NetBSD__

#include <machine/apmvar.h>

#define _PATH_APM_SOCKET       "/var/run/apmdev"
#define _PATH_APM_CTLDEV       "/dev/apmctl"
#define _PATH_APM_NORMAL       "/dev/apm"

int first = 1;
void battery_check(void)
{
       int fd, r;
       bool p;
       struct apm_power_info info;

       // No need to display error too many times
       if (m_undefined) return;

       if ((fd = open(_PATH_APM_NORMAL, O_RDONLY)) == -1) {
               fprintf(stderr, "xbattbar: cannot open apm device\n");
               m_undefined = true;
               return;
       }

       if (ioctl(fd, APM_IOC_GETPOWER, &info) != 0) {
               fprintf(stderr, "xbattbar: ioctl APM_IOC_GETPOWER failed\n");
               m_undefined = true;
               return;
       }

       close(fd);

       /* get current remoain */
       if (info.battery_life > 100) {
               /* some APM BIOSes return values slightly > 100 */
               r = 100;
       } else {
               r = info.battery_life;
       }

       /* get AC-line status */
       if (info.ac_state == APM_AC_ON) {
               p = true;
       } else {
               p = false;
       }

       if (first || m_line_on != p || m_bat_percentage != r) {
               first = 0;
               m_line_on = p;
               m_bat_percentage = r;
               m_bat_time = 0; //FIXME: battery time on netbsd
               redraw();
       }
}

#endif /* __NetBSD__ */


#ifdef linux

#include <errno.h>
#include <linux/apm_bios.h>

#define APM_PROC	"/proc/apm"

#define        APM_STAT_LINE_OFF       0
#define        APM_STAT_LINE_ON        1

typedef struct apm_info {
   char driver_version[10];
   int        apm_version_major;
   int        apm_version_minor;
   int        apm_flags;
   int        ac_line_status;
   int        battery_status;
   int        battery_flags;
   int        battery_percentage;
   int        battery_time;
   int        using_minutes;
} apm_info;


int first = 1;
void BatteryMonitor::battery_check(void)
{
  int r;
  bool p;
  FILE *pt;
  struct apm_info i;
  char buf[100];

  // No need to display error too many times
  if (m_undefined) return;

  /* get current status */
  errno = 0;
  if ( (pt = fopen( APM_PROC, "r" )) == NULL) {
    fprintf(stderr, "xbattbar: Can't read proc info: %s\n", strerror(errno));
    m_undefined = true;
    return;
  }

  fgets( buf, sizeof( buf ) - 1, pt );
  buf[ sizeof( buf ) - 1 ] = '\0';
  sscanf( buf, "%s %d.%d %x %x %x %x %d%% %d %d\n",
	  &i.driver_version,
	  &i.apm_version_major,
	  &i.apm_version_minor,
	  &i.apm_flags,
	  &i.ac_line_status,
	  &i.battery_status,
	  &i.battery_flags,
	  &i.battery_percentage,
	  &i.battery_time,
	  &i.using_minutes );

  fclose (pt);

   /* some APM BIOSes return values slightly > 100 */
   if ( (r = i.battery_percentage) > 100 ){
     r = 100;
   }

   /* get AC-line status */
   if ( i.ac_line_status == APM_STAT_LINE_ON) {
     p = true;
   } else {
     p = false;
   }

  if (first || m_line_on != p || m_bat_percentage != r) {
    first = 0;
    m_line_on = p;
    m_bat_percentage = r;
    m_bat_time = i.battery_time;
    redraw();
  }
}

#endif /* linux */

