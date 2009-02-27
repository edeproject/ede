/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <edelib/XSettingsClient.h>
#include <edelib/Debug.h>
#include "SettingsApplicator.h"

EDELIB_NS_USING(XSettingsClient)
EDELIB_NS_USING(XSettingsAction)
EDELIB_NS_USING(XSettingsSetting)
EDELIB_NS_USING(XSETTINGS_ACTION_NEW)
EDELIB_NS_USING(XSETTINGS_ACTION_CHANGED)
EDELIB_NS_USING(XSETTINGS_ACTION_DELETED)
EDELIB_NS_USING(XSETTINGS_TYPE_INT)

static XSettingsClient* client = NULL;
static Display*         client_display = NULL;
static int              client_screen;

static void xsettings_cb(const char* name, XSettingsAction a, XSettingsSetting* s, void* data) {
	if(!client)
		return;

	if(strcmp(name, "Bell/Volume") == 0 && s->type == XSETTINGS_TYPE_INT) {
		XKeyboardControl kc;

		kc.bell_percent = s->data.v_int;
		XChangeKeyboardControl(client_display, KBBellPercent, &kc);
		return;
	}

	if(strcmp(name, "Bell/Pitch") == 0 && s->type == XSETTINGS_TYPE_INT) {
		XKeyboardControl kc;

		kc.bell_pitch = s->data.v_int;
		XChangeKeyboardControl(client_display, KBBellPitch, &kc);
		return;
	}

	if(strcmp(name, "Bell/Duration") == 0 && s->type == XSETTINGS_TYPE_INT) {
		XKeyboardControl kc;

		kc.bell_duration = s->data.v_int;
		XChangeKeyboardControl(client_display, KBBellDuration, &kc);
		return;
	}
}

void xsettings_applicator_init(Display* dpy, int scr) {
	/* 
	 * make sure we set display first, because after 'init()' callback
	 * will be imediately called
	 */
	client_display = dpy;
	client_screen  = scr;

	client = new XSettingsClient;
	if(!client->init(dpy, scr, xsettings_cb, NULL)) {
		delete client;
		client = NULL;
		return;
	}
}

void xsettings_applicator_shutdown(void) {
	delete client;
	client = NULL;
}

void xsettings_applicator_process_event(const XEvent* xev) {
	if(client)
		client->process_xevent(xev);
}
