/*
 * $Id$
 *
 * ede-bell-conf, a tool to configure system bell
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Value_Slider.H>

#include <edelib/Window.h>
#include <edelib/Nls.h>
#include <edelib/XSettingsClient.h>

EDELIB_NS_USING(XSettingsClient)
EDELIB_NS_USING(XSettingsAction)
EDELIB_NS_USING(XSettingsSetting)
EDELIB_NS_USING(XSETTINGS_ACTION_NEW)
EDELIB_NS_USING(XSETTINGS_ACTION_CHANGED)
EDELIB_NS_USING(XSETTINGS_ACTION_DELETED)
EDELIB_NS_USING(XSETTINGS_TYPE_INT)

static Fl_Value_Slider* vol_slide;
static Fl_Value_Slider* pitch_slide;
static Fl_Value_Slider* dur_slide;

static edelib::Window*  win;
static XSettingsClient* xsc;

static bool block_xsettings_cb = false;

#define CHECK_SETTING(n, setting, action) (strcmp(setting->name, n) == 0) && \
		(action == XSETTINGS_ACTION_NEW) || (action == XSETTINGS_ACTION_CHANGED)

#define KEY_VOLUME   "Bell/Volume"
#define KEY_PITCH    "Bell/Pitch"
#define KEY_DURATION "Bell/Duration"

static int xevent_handler(int e) {
	if(xsc)
		xsc->process_xevent(fl_xevent);
	return 1;
}

static void xsettings_cb(const char* name, XSettingsAction a, XSettingsSetting* s, void* data) {
	if(s->type != XSETTINGS_TYPE_INT)
		return;

	if(block_xsettings_cb)
		return;

	if(CHECK_SETTING(KEY_VOLUME, s, a))
		vol_slide->value(s->data.v_int);
	if(CHECK_SETTING(KEY_PITCH, s, a))
		pitch_slide->value(s->data.v_int);
	if(CHECK_SETTING(KEY_DURATION, s, a))
		dur_slide->value(s->data.v_int);
}

static void set_values(bool save) {
	unsigned long v = KBBellPercent | KBBellPitch | KBBellDuration;
	XKeyboardControl kc;
	kc.bell_percent  = (unsigned int)vol_slide->value();
	kc.bell_pitch    = (unsigned int)pitch_slide->value();
	kc.bell_duration = (unsigned int)dur_slide->value();

	XChangeKeyboardControl(fl_display, v, &kc);

	if(save && xsc) {
		/* disable callback, since modifying the value will trigger it */
		block_xsettings_cb = true;

		xsc->set(KEY_VOLUME, (unsigned int)vol_slide->value());
		xsc->set(KEY_PITCH,  (unsigned int)pitch_slide->value());
		xsc->set(KEY_DURATION, (unsigned int)dur_slide->value());
		xsc->manager_notify();

		block_xsettings_cb = false;
	}
}

static void cancel_cb(Fl_Widget*, void*) {
	win->hide();
}

static void ok_cb(Fl_Widget*, void*) {
	set_values(true);
	win->hide();
}

static void test_cb(Fl_Widget*, void*) {
	set_values(false);
	XBell(fl_display, 0);
}

int main(int argc, char **argv) {
	win = new edelib::Window(330, 210, _("System bell configuration"));
	win->begin();
		vol_slide = new Fl_Value_Slider(10, 30, 310, 25, _("Volume"));
		vol_slide->type(5);
		vol_slide->step(1);
		vol_slide->maximum(100);
		vol_slide->align(FL_ALIGN_TOP);

		pitch_slide = new Fl_Value_Slider(10, 80, 310, 25, _("Pitch"));
		pitch_slide->type(5);
		pitch_slide->step(1);
		pitch_slide->minimum(100);
		pitch_slide->maximum(1000);
		pitch_slide->align(FL_ALIGN_TOP);

		dur_slide = new Fl_Value_Slider(10, 130, 310, 25, _("Duration"));
		dur_slide->type(5);
		dur_slide->step(1);
		dur_slide->minimum(0);
		dur_slide->maximum(1000);
		dur_slide->align(FL_ALIGN_TOP);

		Fl_Button* ok = new Fl_Button(135, 175, 90, 25, _("&OK"));
		ok->callback(ok_cb);

		Fl_Button* cancel = new Fl_Button(230, 175, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);

		Fl_Button* test = new Fl_Button(10, 175, 90, 25, _("&Test"));
		test->callback(test_cb);
	win->end();
	win->show(argc, argv);

	XSettingsClient x;
	if(!x.init(fl_display, fl_screen, xsettings_cb, NULL))
		xsc = NULL;
	else
		xsc = &x;

	Fl::add_handler(xevent_handler);
	return Fl::run();
}
