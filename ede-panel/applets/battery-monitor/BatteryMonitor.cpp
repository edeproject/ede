/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2013 Sanel Zukan
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
#include <FL/Fl_Box.H>
#include <edelib/Nls.h>
#include "Applet.h"

#ifdef EDELIB_HAVE_DBUS
#include <edelib/EdbusConnection.h>
#include <edelib/EdbusMessage.h>
#include <edelib/EdbusList.h>
#include <edelib/EdbusObjectPath.h>
#include <edelib/EdbusData.h>
#include <edelib/List.h>
#include <edelib/Debug.h>
#include <edelib/IconLoader.h>

EDELIB_NS_USING(EdbusConnection)
EDELIB_NS_USING(EdbusMessage)
EDELIB_NS_USING(EdbusList)
EDELIB_NS_USING(EdbusObjectPath)
EDELIB_NS_USING(EdbusData)
EDELIB_NS_USING(EdbusVariant)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(ICON_SIZE_SMALL)
EDELIB_NS_USING(list)
EDELIB_NS_USING(EDBUS_SYSTEM)

#define UPOWER_TYPE_BATTERY 2

#define UPOWER_STATE_CHARGING    1
#define UPOWER_STATE_DISCHARGING 2
#define UPOWER_STATE_EMPTY       3
#define UPOWER_STATE_CHARGED     4
	
#define UPOWER_SERVICE   "org.freedesktop.UPower"
#define UPOWER_INTERFACE "org.freedesktop.UPower.Device"
#define UPOWER_PATH      "/org/freedesktop/UPower"

#define BATTERY_MIN         10 /* minimal time before icon to BATTERY_CAUTION_IMG was changed */
#define BATTERY_IMG         "battery"
#define BATTERY_CAUTION_IMG "battery-caution"

typedef list<EdbusObjectPath> BatteryList;
typedef list<EdbusObjectPath>::iterator BatteryListIt;

/*
 * For now all query-ing is done via UPower. Applet is support multiple batteries, but
 * status will be reported as cumulative state all of them.

 * TODO: add support for other methods, like DeviceKit (is this obsolete??)
 */
class BatteryMonitor : public Fl_Box {
private:
    const char *bimg;
	char tip[128];

	EdbusConnection con;
	BatteryList     batts;

public:
	BatteryMonitor() : Fl_Box(0, 0, 30, 25), bimg(0) { 
		box(FL_FLAT_BOX);
		scan_and_init();
	}

	EdbusConnection &connection() { return con; }

	void tooltip_printf(const char *fmt, ...);
	void scan_and_init(void);
	int  update_icon_and_tooltip(void);
	void set_icon(double percentage);

	BatteryList &get_batteries(void);
};

static bool bus_property_get(EdbusConnection &con,
							 const char *service,
							 const char *path,
							 const char *iface,
							 const char *prop,
							 EdbusMessage *ret)
{
	EdbusMessage msg;
	msg.create_method_call(service, path, "org.freedesktop.DBus.Properties", "Get");
	msg << EdbusData::from_string(iface);
	msg << EdbusData::from_string(prop);
	
	E_RETURN_VAL_IF_FAIL(con.send_with_reply_and_block(msg, 1000, *ret), false);
	E_RETURN_VAL_IF_FAIL(ret->size() == 1, false);
	return true;
}

static bool is_battery(EdbusConnection &con, const char *path) {
	EdbusMessage reply;
	E_RETURN_VAL_IF_FAIL(bus_property_get(con, UPOWER_SERVICE, path, UPOWER_INTERFACE, "Type", &reply), false);
	
	EdbusMessage::const_iterator it = reply.begin();
	E_RETURN_VAL_IF_FAIL(it->is_variant(), false);
	
	EdbusVariant v = it->to_variant();
	E_RETURN_VAL_IF_FAIL(v.value.is_uint32(), false);

	return v.value.to_uint32() == UPOWER_TYPE_BATTERY;
}

static bool get_percentage(EdbusConnection &con, const char *path, double *ret) {
	EdbusMessage reply;
	E_RETURN_VAL_IF_FAIL(bus_property_get(con, UPOWER_SERVICE, path, UPOWER_INTERFACE, "Percentage", &reply), false);
	
	EdbusMessage::const_iterator it = reply.begin();
	E_RETURN_VAL_IF_FAIL(it->is_variant(), false);
	
	EdbusVariant v = it->to_variant();
	E_RETURN_VAL_IF_FAIL(v.value.is_double(), false);

	*ret = v.value.to_double();
	return true;
}

static bool get_state(EdbusConnection &con, const char *path, int *ret) {
	EdbusMessage reply;
	E_RETURN_VAL_IF_FAIL(bus_property_get(con, UPOWER_SERVICE, path, UPOWER_INTERFACE, "State", &reply), false);
	
	EdbusMessage::const_iterator it = reply.begin();
	E_RETURN_VAL_IF_FAIL(it->is_variant(), false);
	
	EdbusVariant v = it->to_variant();
	E_RETURN_VAL_IF_FAIL(v.value.is_uint32(), false);

	*ret = v.value.to_uint32();
	return true;
}

static const char *get_state_str(EdbusConnection &con, const char *path) {
	const char * fallback = _("unknown");
	int ret;
	E_RETURN_VAL_IF_FAIL(get_state(con, path, &ret), fallback);
	
	switch(ret) {
		case UPOWER_STATE_CHARGED:     return _("charged");
		case UPOWER_STATE_CHARGING:    return _("charging");
		case UPOWER_STATE_DISCHARGING: return _("discharging");
		case UPOWER_STATE_EMPTY:       return _("empty");
	}
	
	return fallback;
}

static int signal_cb(const EdbusMessage *msg, void *data) {
	/* in case some org.freedesktop.DBus (like NameAcquired) received */
	if(strcmp(msg->interface(), UPOWER_INTERFACE) != 0)
		return 0;
	BatteryMonitor *self = (BatteryMonitor*)data;
	return self->update_icon_and_tooltip();
}

void BatteryMonitor::tooltip_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vsnprintf(tip, sizeof(tip), fmt, args);
	va_end(args);
	
	tooltip(tip);
}

void BatteryMonitor::scan_and_init(void) {
	if(con.connected()) return;
	E_RETURN_IF_FAIL(con.connect(EDBUS_SYSTEM));
	
	/* get battery devices */
	EdbusMessage msg, reply;
	msg.create_method_call(UPOWER_SERVICE, UPOWER_PATH, UPOWER_SERVICE, "EnumerateDevices");

	E_RETURN_IF_FAIL(con.send_with_reply_and_block(msg, 1000, reply));
	E_RETURN_IF_FAIL(reply.size() == 1);

	EdbusMessage::const_iterator it = reply.begin();
	E_RETURN_IF_FAIL(it->is_array());

	EdbusList arr = it->to_array();
	it = arr.begin();
	EdbusMessage::const_iterator ite = arr.end();

	for(; it != ite; ++it) {
		if(!it->is_object_path()) continue;

		EdbusObjectPath p = it->to_object_path();

		if(is_battery(con, p.path())) {
			/* filters so signal_cb() doesn't get clobbered with uninterested dbus signals */
			con.add_signal_match(p.path(), UPOWER_INTERFACE, "Changed");
			//con.add_signal_match(p.path(), UPOWER_SERVICE, "DeviceChanged");
			batts.push_back(p);
		}
	}
	
	update_icon_and_tooltip();

	/* ready to receive signals */
	con.signal_callback(signal_cb, this);
	con.setup_listener_with_fltk();
}

int BatteryMonitor::update_icon_and_tooltip(void) {
	/* in case connection failed somehow */
	if(!con.connected()) {
		label("0");
		return 0;
	}

	E_RETURN_VAL_IF_FAIL(batts.size() > 0, 0);
	double p = 0, ret = 0;

	if(batts.size() == 1) {
		E_RETURN_VAL_IF_FAIL(get_percentage(con, batts.front().path(), &ret), 0);
		p = ret;
		tooltip_printf(_("Battery %s: %i%%"), get_state_str(con, batts.front().path()), (int)ret);
	} else {
		for(BatteryListIt it = batts.begin(), ite = batts.end(); it != ite; ++it) {
			if(!get_percentage(con, it->path(), &ret)) continue;
			p += ret;
		}
	
		p /= batts.size();
		tooltip_printf(_("%i batteries: %i%%"), batts.size(), (int)p);
	}

	set_icon(p);
	/* returning state is mainly for signal_cb() */
	return 1;
}

#define BATTERY_MIN 10	

void BatteryMonitor::set_icon(double percentage) {
	if(E_UNLIKELY(IconLoader::inited() == false)) {
		char buf[8];

		snprintf(buf, sizeof(buf), "%i%%", (int)percentage);
		copy_label(buf);
		return;
	}
	
	const char *icon = (percentage >= BATTERY_MIN) ? BATTERY_IMG : BATTERY_CAUTION_IMG;
	/* small check to prevent image loading when not needed */
	if(icon == bimg) return;

	IconLoader::set(this, icon, ICON_SIZE_SMALL);
	bimg = icon;
}

#else /* EDELIB_HAVE_DBUS */
class BatteryMonitor : public Fl_Box {
public:
	BatteryMonitor() : Fl_Box(0, 0, 30, 25) { 
		tooltip(_("Battery status not available"));
		label("bat");
	}
};
#endif
	

EDE_PANEL_APPLET_EXPORT (
 BatteryMonitor, 
 EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT,
 "Battery monitor",
 "0.1",
 "empty",
 "Sanel Zukan"
)
