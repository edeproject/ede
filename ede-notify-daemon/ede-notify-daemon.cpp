/*
 * $Id: ede-panel.cpp 3330 2012-05-28 10:57:50Z karijes $
 *
 * Copyright (C) 2012 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <edelib/Ede.h>
#include <edelib/Debug.h>

#ifdef EDELIB_HAVE_DBUS
#include <FL/Fl.H>
#include <FL/Fl_Shared_Image.H>

#include <edelib/EdbusConnection.h>
#include <edelib/EdbusMessage.h>
#include <edelib/EdbusData.h>
#include <edelib/EdbusList.h>
#include <edelib/EdbusDict.h>
#include <edelib/IconLoader.h>
#include <edelib/Netwm.h>
#include <edelib/Missing.h>

#include "NotifyWindow.h"

/* server info sent via GetServerInformation */
#define EDE_NOTIFY_DAEMON_NAME         "EDE Notification Daemon"
#define EDE_NOTIFY_DAEMON_VENDOR       "ede"
#define EDE_NOTIFY_DAEMON_VERSION      "0.2"
#define EDE_NOTIFY_DAEMON_SPEC_VERSION "1.2"

#define NOTIFICATIONS_DBUS_PATH         "/org/freedesktop/Notifications"
#define NOTIFICATIONS_DBUS_SERVICE_NAME "org.freedesktop.Notifications"

/* types of urgency levels */
#define URGENCY_LOW      0
#define URGENCY_NORMAL   1
#define URGENCY_CRITICAL 2

/* from FLTK */
#define FOREVER 1e20

/* space between shown windows (on both side) */
#define WINDOWS_PADDING 10

#define IS_MEMBER(m, s1) (strcmp((m->member()), (s1)) == 0)

EDELIB_NS_USING(EdbusConnection)
EDELIB_NS_USING(EdbusMessage)
EDELIB_NS_USING(EdbusData)
EDELIB_NS_USING(EdbusList)
EDELIB_NS_USING(EdbusDict)
EDELIB_NS_USING(EdbusVariant)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(netwm_workarea_get_size)
EDELIB_NS_USING(EDBUS_SESSION)
EDELIB_NS_USING(EDBUS_TYPE_INVALID)

static bool server_running;

/*
 * list of server capabilities
 * check all available on: http://people.gnome.org/~mccann/docs/notification-spec/notification-spec-latest.html
 */
static const char *server_caps[] = {"actions", "body", "icon-static", 0};

/* increased every time new notification window is shown; must be less than UINT_MAX */
static unsigned int notify_id;

static bool empty_str(const char *s) {
	if(!s || !strlen(s))
		return true;
	return false;
}

static byte_t get_urgency_level(EdbusDict &hints) {
	EdbusData ret = hints.find(EdbusData::from_string("urgency"));
	/* default */
	E_RETURN_VAL_IF_FAIL(ret.type() != EDBUS_TYPE_INVALID, URGENCY_NORMAL);

	/* FIXME: spec said this is byte by I'm getting variant here ??? */
	E_RETURN_VAL_IF_FAIL(ret.is_variant(), URGENCY_NORMAL);

	EdbusVariant v = ret.to_variant();
	/* must be byte */
	E_RETURN_VAL_IF_FAIL(v.value.is_byte(), URGENCY_NORMAL);
	return v.value.to_byte();
}

static bool get_int_coordinate(const char *n, EdbusDict &hints, int &c) {
	EdbusData ret = hints.find(EdbusData::from_string(n));

	/* it is fine to be not present */
	if(ret.type() == EDBUS_TYPE_INVALID)
		return false;

	/* coordinates are 'int'; but which fucking 'int'; int32, int16, int64 ??? */
	if(ret.is_int32())
		c = (int)ret.to_int32();
	else if(ret.is_int16())
		c = (int)ret.to_int16();
	else if(ret.is_int64())
		c = (int)ret.to_int64();
	else
		return false;

	return true;
}

static void show_window(unsigned int id,
						const char *app_name,
						const char *app_icon,
						const char *summary,
						const char *body,
						int expire_timeout,
						EdbusDict &hints)
{
	byte_t u = get_urgency_level(hints);

	NotifyWindow *win = new NotifyWindow();

	if(!empty_str(summary))
		win->set_summary(summary);
	if(!empty_str(body))
		win->set_body(body);
	if(empty_str(app_icon)) {
		switch(u) {
			case URGENCY_CRITICAL:
				app_icon = "dialog-error";
				break;
			case URGENCY_LOW:
			case URGENCY_NORMAL:
			default:
				app_icon = "dialog-information";
		}
	}

	win->set_icon(app_icon);
	win->set_id(id);
	win->set_expire(expire_timeout);

	/* according to spec, both coordinates must exist so window can be positioned as desired */
	int X, Y;
	if(get_int_coordinate("x", hints, X) &&
	   get_int_coordinate("y", hints, Y))
	{
		win->position(X, Y);
	} else {
		int sx, sy, sw, sh;
		if(!netwm_workarea_get_size(sx, sy, sw, sh))
			Fl::screen_xywh(sx, sy, sw, sh);

		/* default positions */
		int px, py;
		px = sw - WINDOWS_PADDING - win->w();
		py = sh - WINDOWS_PADDING - win->h();

		/*
		 * iterate through shown windows and find position where to put our one
		 * FIXME: this is quite primitive window position deducing facility
		 *
		 * TODO: add locking here
		 */
		Fl_Window *wi;
		for(wi = Fl::first_window(); wi; wi = Fl::next_window(wi)) {
			if(wi->type() != NOTIFYWINDOW_TYPE) continue;

			py -= wi->h() + WINDOWS_PADDING;

			if(py < sy) {
				px -= wi->w() + WINDOWS_PADDING;
				py = sh - WINDOWS_PADDING - wi->h();
			}
		}

		win->position(px, py);
	}

	/* we are already running loop, so window will handle events */
	win->show();
}

static int handle_notify(EdbusConnection *dbus, const EdbusMessage *m) {
	if(m->size() != 8) {
		E_WARNING(E_STRLOC ": Received malformed 'Notify'. Ignoring...\n");
		return 0;
	}

	const char *app_name, *app_icon, *summary, *body;
	app_name = app_icon = summary = body = NULL;
	unsigned int replaces_id;
	int expire_timeout;

	EdbusMessage::const_iterator it = m->begin();
	E_RETURN_VAL_IF_FAIL(it->is_string(), 0);

	app_name = it->to_string();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_uint32(), 0);
	replaces_id = it->to_uint32();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_string(), 0);
	app_icon = it->to_string();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_string(), 0);
	summary = it->to_string();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_string(), 0);
	body = it->to_string();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_array(), 0);
	EdbusList array = it->to_array();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_dict(), 0);
	/* we are supporting only 'urgency' and x/y hints here */
	EdbusDict hints = it->to_dict();
	++it;

	E_RETURN_VAL_IF_FAIL(it->is_int32(), 0);
	expire_timeout = it->to_int32();

	/* specification dumb stuff: what if we got UINT_MAX?? here we will reverse to first ID */
	if(++notify_id == UINT_MAX) notify_id = 1;

	if(replaces_id) {
		//replaces_id == notify_id;
	} else {
		show_window(notify_id, app_name, app_icon, summary, body, expire_timeout, hints);
	}

	/* reply sent to client */
	EdbusMessage reply;
	reply.create_reply(*m);
	reply << EdbusData::from_uint32(replaces_id);
	dbus->send(reply);

	return 1;
}

static int notifications_dbus_method_cb(const EdbusMessage *m, void *d) {
	EdbusConnection *dbus = (EdbusConnection*)d;

	/* void org.freedesktop.Notifications.GetServerInformation (out STRING name, out STRING vendor, out STRING version) */
	if(IS_MEMBER(m, "GetServerInformation")) {
		EdbusMessage reply;
		reply.create_reply(*m);
		reply << EdbusData::from_string(EDE_NOTIFY_DAEMON_NAME)
			  << EdbusData::from_string(EDE_NOTIFY_DAEMON_VENDOR)
			  << EdbusData::from_string(EDE_NOTIFY_DAEMON_VERSION)
			  << EdbusData::from_string(EDE_NOTIFY_DAEMON_SPEC_VERSION); /* without this notify-send will not work */

		dbus->send(reply);
		return 1;
	}

	/* STRING_ARRAY org.freedesktop.Notifications.GetCapabilities (void) */
	if(IS_MEMBER(m, "GetCapabilities")) {
		EdbusList array = EdbusList::create_array();
		for(int i = 0; server_caps[i]; i++)
			array << EdbusData::from_string(server_caps[i]);

		EdbusMessage reply;
		reply.create_reply(*m);
		reply << array;
		dbus->send(reply);
		return 1;
	}

	/* UINT32 org.freedesktop.Notifications.Notify (STRING app_name, UINT32 replaces_id, STRING app_icon, STRING summary, STRING body, ARRAY actions, DICT hints, INT32 expire_timeout) */
	if(IS_MEMBER(m, "Notify"))
		return handle_notify(dbus, m);
	return 1;
}

#endif /* EDELIB_HAVE_DBUS */

#if 0
static int notifications_dbus_signal_cb(const EdbusMessage *m, void *d) {
	E_DEBUG("+=> %s\n", m->member());
	return 1;
}	
#endif

static void help(void) {
	puts("Usage: ede-notify-daemon [OPTIONS]");
	puts("Background service responsible for displaying various notifications");
	puts("Options:");
	puts("  -h, --help       this help");
	puts("  -n, --no-daemon  do not run in background");
}

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

int main(int argc, char **argv) {
	/* daemon behaves as GUI app, as will use icon theme and etc. */
	EDE_APPLICATION("ede-notify-daemon");

	bool daemonize = true;
	if(argc > 1) {
		if(CHECK_ARGV(argv[1], "-h", "--help")) {
			help();
			return 0;
		}

		if(CHECK_ARGV(argv[1], "-n", "--no-daemon")) {
			daemonize = false;
		}
	}

#ifdef EDELIB_HAVE_DBUS
	server_running  = false;
	notify_id       = 0;
	EdbusConnection dbus;

	IconLoader::init();
	fl_register_images();
	fl_open_display();

	if(!dbus.connect(EDBUS_SESSION)) {
		E_WARNING(E_STRLOC ": Unable to connect to session bus. Is your dbus daemon running?");
		return 1;
	}

	if(!dbus.request_name(NOTIFICATIONS_DBUS_SERVICE_NAME)) {
		E_WARNING(E_STRLOC ": Seems notification daemon is already running. Quitting...\n");
		return 1;
	}

	if(daemonize) edelib_daemon(0, 0);

	dbus.register_object(NOTIFICATIONS_DBUS_PATH);
	dbus.method_callback(notifications_dbus_method_cb, &dbus);
	//dbus.signal_callback(notifications_dbus_signal_cb, &dbus);
	dbus.setup_listener_with_fltk();
	server_running = true;

	while(server_running)
		Fl::wait(FOREVER);
	
	IconLoader::shutdown();
#else
	E_WARNING(E_STRLOC ": edelib is compiled without DBus so notification daemon is not able to receive notification messages\n");
#endif
	return 0;
}
