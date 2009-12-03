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

#ifndef __XSM_H__
#define __XSM_H__

#include <edelib/XSettingsManager.h>
#include <edelib/EdbusConnection.h>

EDELIB_NS_USING(EdbusConnection)
EDELIB_NS_USING(XSettingsData)

/* XSETTINGS manager with serialization capability. Also it will write/undo to xrdb (X Resource database). */
class Xsm : public edelib::XSettingsManager {
private:
	EdbusConnection* dbus_conn;

	/* replace XResource values from one from XSETTINGS */
	void xresource_replace(void);

	/* undo old XResource values */
	void xresource_undo(void);

	/* serve XSETTINGS via D-Bus */
	void xsettings_dbus_serve(void);
public:
	Xsm() : dbus_conn(NULL) { }
	~Xsm();

	/* return loaded D-Bus connection */
	EdbusConnection* get_dbus_connection(void) { return dbus_conn; }

	/* access to manager content */
	XSettingsData* get_manager_data(void) { return manager_data; }

	/* load stored settings */
	bool load_serialized(void);

	/* store known settings */
	bool save_serialized(void);
};

#endif
