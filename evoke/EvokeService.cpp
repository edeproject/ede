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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <edelib/File.h>
#include <edelib/Debug.h>
#include <edelib/Config.h>
#include <edelib/Resource.h>
#include <edelib/StrUtil.h>
#include <edelib/MessageBox.h>
#include <edelib/Nls.h>

#include "EvokeService.h"
#include "Splash.h"
#include "Logout.h"
#include "Xsm.h"

EDELIB_NS_USING(Config)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(RES_SYS_ONLY)
EDELIB_NS_USING(file_exists)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(str_trim)

#ifdef USE_LOCAL_CONFIG
 #define CONFIG_GET_STRVAL(object, section, key, buff) object.get(section, key, buff, sizeof(buff))
#else
 #define CONFIG_GET_STRVAL(object, section, key, buff) object.get(section, key, buff, sizeof(buff), RES_SYS_ONLY)
#endif

static Atom XA_EDE_EVOKE_SHUTDOWN_ALL;
static Atom XA_EDE_EVOKE_QUIT;

static int get_int_property_value(Atom at) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen),
			at, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra,
			(unsigned char**)&prop);
	int ret = -1;
	if(status != Success && !prop)
		return ret;
	ret = int(*(long*)prop);
	XFree(prop);
	return ret;
}

EvokeService::EvokeService() : lock_name(NULL), xsm(NULL), is_running(false) { 
	/* TODO: or add setup_atoms() function */
	XA_EDE_EVOKE_SHUTDOWN_ALL = XInternAtom(fl_display, "_EDE_EVOKE_SHUTDOWN_ALL", False);
	XA_EDE_EVOKE_QUIT         = XInternAtom(fl_display, "_EDE_EVOKE_QUIT", False);
}

EvokeService::~EvokeService() { 
	clear_startup_items();
	stop_xsettings_manager(true);
	remove_lock();
}

EvokeService* EvokeService::instance(void) {
	static EvokeService es;
	return &es;
}

bool EvokeService::setup_lock(const char* name) {
	if(file_exists(name))
		return false;

	FILE* f = fopen(name, "w");
	if(!f) 
		return false;
	fclose(f);

	lock_name = strdup(name);
	return true;
}

void EvokeService::remove_lock(void) {
	if(!lock_name)
		return;

	file_remove(lock_name);
	free(lock_name);
	lock_name = NULL;
}

void EvokeService::clear_startup_items(void) {
	if(startup_items.empty())
		return;

	StartupItemListIter it = startup_items.begin(), it_end = startup_items.end();
	for(; it != it_end; ++it)
		delete *it;

	startup_items.clear();
}

void EvokeService::read_startup(void) {
#ifdef USE_LOCAL_CONFIG
	/* 
	 * this will load SETTINGS_FILENAME only from local directory;
	 * intended for development and testing only
	 */
	Config c;
	int ret = c.load("ede-startup.conf");
#else
	/* only system resource will be loaded; use ede-startup will be skipped */
	Resource c;
	int ret = c.load("ede/ede-startup");
#endif
	if(!ret) {
		E_WARNING(E_STRLOC ": Unable to load EDE startup file\n");
		return;
	}

	char tok_buff[256], buff[256];

	/* if nothing found, exit */
	if(!CONFIG_GET_STRVAL(c, "Startup", "start_order", tok_buff))
		return;

	if(CONFIG_GET_STRVAL(c, "Startup", "splash_theme", buff))
		splash_theme = buff;

	for(const char* sect = strtok(tok_buff, ","); sect; sect = strtok(NULL, ",")) {
		/* remove leading/ending spaces, if exists */
		str_trim(buff);

		/* assure each startup item has 'exec' key */
		if(!CONFIG_GET_STRVAL(c, sect, "exec", buff)) {
			E_WARNING(E_STRLOC ": Startup item '%s' does not have anything to execute. Skipping...\n", sect);
			continue;
		}

		StartupItem *s = new StartupItem;
		s->exec = buff;

		if(CONFIG_GET_STRVAL(c, sect, "icon", buff))
			s->icon = buff;
		if(CONFIG_GET_STRVAL(c, sect, "description", buff))
			s->description = buff;

		startup_items.push_back(s);
	}
}

void EvokeService::run_startup(bool splash, bool dryrun) {
	if(startup_items.empty())
		return;

	Splash s(startup_items, splash_theme, splash, dryrun);
	s.run();
	clear_startup_items();
}

void EvokeService::start_xsettings_manager(void) {
	xsm = new Xsm;

	if(Xsm::manager_running(fl_display, fl_screen)) {
		int ret = edelib::ask(_("XSETTINGS manager already running on this screen. Would you like to replace it?"));
		if(ret < 1) {
			stop_xsettings_manager(false);
			return;
		}
	}

	if(!xsm->init(fl_display, fl_screen)) {
		edelib::alert(_("Unable to load XSETTINGS manager properly :-("));
		stop_xsettings_manager(false);
	}

	E_RETURN_IF_FAIL(xsm);

	if(xsm->load_serialized())
		xsm->notify();
}

void EvokeService::stop_xsettings_manager(bool serialize) {
	if(!xsm)
		return;

	if(serialize)
		xsm->save_serialized();

	delete xsm;
	xsm = NULL;
}

int EvokeService::handle(const XEvent* xev) {
	if(xsm && xsm->should_terminate(xev)) {
		E_DEBUG(E_STRLOC ": Terminating XSETTINGS manager on request by X event\n");
		stop_xsettings_manager(true);
		return 1;
	}

	if(xev->xproperty.atom == XA_EDE_EVOKE_QUIT) {
		int val = get_int_property_value(XA_EDE_EVOKE_QUIT);
		if(val == 1)
			stop();
	}

	if(xev->xproperty.atom == XA_EDE_EVOKE_SHUTDOWN_ALL) {
		int val = get_int_property_value(XA_EDE_EVOKE_SHUTDOWN_ALL);
		if(val == 1) {
			int dw = DisplayWidth(fl_display, fl_screen);
			int dh = DisplayHeight(fl_display, fl_screen);

			logout_dialog_show(dw, dh, LOGOUT_OPT_SHUTDOWN | LOGOUT_OPT_RESTART);
		}
	}

	return 0;
}
