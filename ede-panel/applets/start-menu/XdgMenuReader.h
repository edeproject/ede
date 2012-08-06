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

#ifndef __XDGMENUREADER_H__
#define __XDGMENUREADER_H__

#include <edelib/MenuItem.h>
#include <edelib/List.h>
#include <edelib/String.h>

EDELIB_NS_USING(MenuItem)
EDELIB_NS_USING(String)
EDELIB_NS_USING(list)

typedef list<String> StrList;
typedef list<String>::iterator StrListIt;

void xdg_menu_dump_for_test_suite(void);
/* all locations where menu files are stored */
void xdg_menu_applications_location(StrList &lst);

struct XdgMenuContent;

XdgMenuContent *xdg_menu_load(void);
void            xdg_menu_delete(XdgMenuContent *c);
MenuItem       *xdg_menu_to_fltk_menu(XdgMenuContent *c);

#endif
