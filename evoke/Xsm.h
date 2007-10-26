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

#include <edelib/XSettingsManager.h>

/*
 * XSETTINGS manager with serialization capability.
 */
class Xsm : public edelib::XSettingsManager {
	public:
		Xsm();
		~Xsm();
		bool load_serialized(const char* file);
		bool save_serialized(const char* file);
};

#endif
