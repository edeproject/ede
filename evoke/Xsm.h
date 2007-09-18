/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __XSM_H__
#define __XSM_H__

#include <X11/Xlib.h> // XEvent

/*
 * This is manager class for XSETTINGS protocol. XSETTINGS provides a mechanism
 * for applications to share simple configuration settings like background
 * colors no matter what toolkit is used. For now only gtk fully supports it
 * and support for ede apps is going to be added.
 *
 * The protocol (0.5 version) is described at:
 * http://standards.freedesktop.org/xsettings-spec/xsettings-spec-0.5.html
 *
 * This code is greatly based on xsettings referent implementation I found on
 * freedesktop.org cvs, since specs are very unclear about the details.
 * Author of referent implementation is Owen Taylor.
 */

enum XsettingsType {
	XS_TYPE_INT = 0,
	XS_TYPE_COLOR,
	XS_TYPE_STRING
};

struct XsmData;
struct XsettingsSetting;

class Xsm {
	private:
		XsmData* data;

	public:
		Xsm();
		~Xsm();
		bool is_running(void);
		bool init(void);
		bool should_quit(const XEvent* xev);
};

#endif
