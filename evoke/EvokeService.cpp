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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <edelib/File.h>
#include <edelib/FileTest.h>
#include <edelib/Debug.h>
#include <edelib/Config.h>
#include <edelib/Resource.h>
#include <edelib/StrUtil.h>
#include <edelib/MessageBox.h>
#include <edelib/Nls.h>
#include <edelib/Run.h>

#if EDELIB_HAVE_DBUS
#include <edelib/EdbusMessage.h>
#include <edelib/EdbusConnection.h>
#endif

#include "EvokeService.h"
#include "Splash.h"
#include "Logout.h"
#include "Xsm.h"
#include "Xshutdown.h"

EDELIB_NS_USING(Config)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(RES_SYS_ONLY)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(file_test)
EDELIB_NS_USING(str_trim)
EDELIB_NS_USING(run_sync)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(ask)
EDELIB_NS_USING(FILE_TEST_IS_REGULAR)

#if EDELIB_HAVE_DBUS
EDELIB_NS_USING(EdbusMessage)
EDELIB_NS_USING(EdbusConnection)
EDELIB_NS_USING(EdbusError)
EDELIB_NS_USING(EDBUS_SESSION)
EDELIB_NS_USING(EDBUS_SYSTEM)
#endif

#ifdef USE_LOCAL_CONFIG
# define CONFIG_GET_STRVAL(object, section, key, buff) object.get(section, key, buff, sizeof(buff))
#else
# define CONFIG_GET_STRVAL(object, section, key, buff) object.get(section, key, buff, sizeof(buff), RES_SYS_ONLY)
#endif

/* stolen from xfce's xfsm-shutdown-helper */
#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define POWEROFF_CMD  "/sbin/shutdown -p now"
# define REBOOT_CMD    "/sbin/shutdown -r now"
#elif defined(sun) || defined(__sun)
# define POWEROFF_CMD  "/usr/sbin/shutdown -i 5 -g 0 -y"
# define REBOOT_CMD    "/usr/sbin/shutdown -i 6 -g 0 -y"
#else
# define POWEROFF_CMD  "/sbin/shutdown -h now"
# define REBOOT_CMD    "/sbin/shutdown -r now"
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

static void send_dbus_ede_quit(void) {
#ifdef EDELIB_HAVE_DBUS
	EdbusConnection c;
	E_RETURN_IF_FAIL(c.connect(EDBUS_SESSION));

	EdbusMessage msg;
	msg.create_signal("/org/equinoxproject/Shutdown", "org.equinoxproject.Shutdown", "Shutdown");
	c.send(msg);
#endif
}

#define CONSOLEKIT_SERVICE   "org.freedesktop.ConsoleKit"
#define CONSOLEKIT_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CONSOLEKIT_INTERFACE "org.freedesktop.ConsoleKit.Manager"

static bool do_shutdown_or_restart(bool restart) {
	const char *action;
	int r = 1;

#ifdef EDELIB_HAVE_DBUS
	const char *can_do_action;

	EdbusConnection c;
	if(!c.connect(EDBUS_SYSTEM)) {
		alert(_("Unable to connect to DBus daemon. Make sure system DBus daemon is running and have permissions to access it"));
		return false;
	}

	/* determine what to check first; ConsoleKit shutdown action calls 'Stop' */
	can_do_action = restart ? "CanRestart" : "CanStop";
	action        = restart ? "Restart" : "Stop";

	EdbusMessage msg, ret;
	msg.create_method_call(CONSOLEKIT_SERVICE,
						   CONSOLEKIT_PATH,
						   CONSOLEKIT_INTERFACE,
						   can_do_action);

	if(!c.send_with_reply_and_block(msg, 100, ret)) {
		EdbusError *err = c.error();
		alert(_("Unable to communicate with DBus properly. Got '%s' (%s)"), err->name(), err->message());
		return false;
	}

	if(ret.size() != 1) {
		alert(_("Bad reply size. Expected '1' element, but got '%i' instead"), ret.size());
		return false;
	}

	EdbusMessage::iterator it = ret.begin();
	if(it->is_bool() && it->to_bool() == true) {
		/* call action */
		msg.create_method_call(CONSOLEKIT_SERVICE,
							   CONSOLEKIT_PATH,
							   CONSOLEKIT_INTERFACE,
							   action);
		c.send(msg);
		return true;
	}

	r = ask(_("You are not allowed to execute this command. Please consult ConsoleKit documentation on how to allow privileged actions. "
			  "Would you like to try to execute system commands?"));
#endif /* EDELIB_HAVE_DBUS */

	/* try to do things manually */
	if(!r) return false;
	action = _("restart");

	if(restart) {
		r = run_sync(REBOOT_CMD);
	} else {
		r = run_sync(POWEROFF_CMD);
		action = _("shutdown");
	}

	if(r) {
		alert(_("Unable to %s the computer. Probably you do not have enough permissions to do that"), action);
		return false;
	}

	return true;
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
	if(file_test(name, FILE_TEST_IS_REGULAR))
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
	int ret = c.load("ede-startup");
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

			int ret = logout_dialog_show(dw, dh, LOGOUT_OPT_SHUTDOWN | LOGOUT_OPT_RESTART);
			if(ret == LOGOUT_RET_CANCEL)
				return 1;

			send_dbus_ede_quit();

			switch(ret) {
				case LOGOUT_RET_RESTART:
					if(!do_shutdown_or_restart(true))
						return 1;
					break;
				case LOGOUT_RET_SHUTDOWN:
					if(!do_shutdown_or_restart(false))
						return 1;
					break;
				case LOGOUT_RET_LOGOUT:
				default:
					x_shutdown();
					break;
			}

			stop();
		}
	}

	return 0;
}
