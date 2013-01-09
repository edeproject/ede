/*
 * $Id$
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

#ifndef __APPLETMANAGER_H__
#define __APPLETMANAGER_H__

#include <edelib/List.h>
#include <edelib/String.h>
#include "Applet.h"

class  Panel;
class  Fl_Widget;
struct AppletData;

typedef edelib::list<AppletData*> AList;
typedef edelib::list<AppletData*>::iterator AListIter;

class AppletManager {
private:
	AList applet_list;
public:
	bool load(const char *path);
	void clear(Panel *p);
	void fill_group(Panel *p);
	void unfill_group(Panel *p);

	bool get_applet_options(Fl_Widget *o, unsigned long &opts);
	unsigned int napplets(void) const { return applet_list.size(); }
};

#endif
