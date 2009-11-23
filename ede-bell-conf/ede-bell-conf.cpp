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

static Fl_Value_Slider* vol_slide   = NULL;
static Fl_Value_Slider* pitch_slide = NULL;
static Fl_Value_Slider* dur_slide   = NULL;

static edelib::Window*  win;
static XSettingsClient* xsc;

static unsigned int vol_val;
static unsigned int pitch_val;
static unsigned int dur_val;

static bool block_xsettings_cb = false;

#define CHECK_SETTING(n, setting, action) (strcmp(setting->name, n) == 0) && \
		((action == XSETTINGS_ACTION_NEW) || (action == XSETTINGS_ACTION_CHANGED))

#define KEY_VOLUME   "Bell/Volume"
#define KEY_PITCH    "Bell/Pitch"
#define KEY_DURATION "Bell/Duration"

static int xevent_handler(int e) {
	if(xsc)
		xsc->process_xevent(fl_xevent);
	/* make sure to return 0 so other events could be processed */
	return 0;
}

static void xsettings_cb(const char* name, XSettingsAction a, XSettingsSetting* s, void* data) {
	if(s->type != XSETTINGS_TYPE_INT)
		return;

	if(block_xsettings_cb)
		return;

	if(CHECK_SETTING(KEY_VOLUME, s, a)) {
		vol_val = s->data.v_int;
		if(vol_slide)
			vol_slide->value(vol_val);
	}

	if(CHECK_SETTING(KEY_PITCH, s, a)) {
		pitch_val = s->data.v_int;
		if(pitch_slide)
			pitch_slide->value(pitch_val);
	}

	if(CHECK_SETTING(KEY_DURATION, s, a)) {
		dur_val = s->data.v_int;
		if(dur_slide)
			dur_slide->value(dur_val);
	}
}

static void apply_values(void) {
	unsigned long v = KBBellPercent | KBBellPitch | KBBellDuration;
	XKeyboardControl kc;
	kc.bell_percent  = vol_val;
	kc.bell_pitch    = pitch_val;
	kc.bell_duration = dur_val;

	XChangeKeyboardControl(fl_display, v, &kc);
}

static void set_values(bool save) {
	vol_val = (unsigned int)vol_slide->value();
	pitch_val = (unsigned int)pitch_slide->value();
	dur_val = (unsigned int)dur_slide->value();

	apply_values();

	if(save && xsc) {
		/* disable callback, since modifying the value will trigger it */
		block_xsettings_cb = true;

		xsc->set(KEY_VOLUME, vol_val);
		xsc->set(KEY_PITCH,  pitch_val);
		xsc->set(KEY_DURATION, dur_val);
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

static void window_create(int argc, char** argv) {
	win = new edelib::Window(330, 210, _("System bell configuration"), edelib::WIN_INIT_NONE);
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
}

int main(int argc, char **argv) {
	bool win_show = true;

	if(argc > 1 && strcmp(argv[1], "--apply") == 0)
		win_show = false;

	if(win_show) {
		window_create(argc, argv);
		Fl::add_handler(xevent_handler);
	} else {
		/* if window is not going to be shown, we still have to open display */
		fl_open_display();
	}

	/* if window is not shown, xsettings_cb will be triggered anyway */
	XSettingsClient x;
	if(!x.init(fl_display, fl_screen, xsettings_cb, NULL))
		xsc = NULL;
	else
		xsc = &x;

	if(!win_show) {
		if(xsc) {
			apply_values();
			XSync(fl_display, False);
		}

		return 0;
	}

	return Fl::run();
}
