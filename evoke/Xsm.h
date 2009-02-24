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

/*
 * XSETTINGS manager with serialization capability.
 * Also it will write/undo to xrdb (X Resource database).
 */
class Xsm : public edelib::XSettingsManager {
public:
	Xsm();
	~Xsm();

	bool load_serialized(void);
	bool save_serialized(void);

	/* replace XResource values from one from XSETTINGS */
	void xresource_replace(void);
	/* undo old XResource values */
	void xresource_undo(void);
};

#endif
