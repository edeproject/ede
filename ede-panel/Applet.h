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

#ifndef __APPLET_H__
#define __APPLET_H__

class Fl_Widget;

/* stored version in each applet shared library in case interface get changed */
#define EDE_PANEL_APPLET_INTERFACE_VERSION 0x01

/*
 * Options assigned to each applet: how it will be resizable (horizontally or vertically)
 * and how it will be aligned. Each applet is by default aligned left without resizing ability.
 */
enum {
	EDE_PANEL_APPLET_OPTION_RESIZABLE_H = (1 << 1),
	EDE_PANEL_APPLET_OPTION_RESIZABLE_V = (1 << 2),
	EDE_PANEL_APPLET_OPTION_ALIGN_LEFT  = (1 << 3),
	EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT = (1 << 4)
};

struct AppletInfo {
	const char    *name;
	const char    *klass_name;
	const char    *version;
	const char    *icon;
	const char    *author;
	unsigned long  options;
};

typedef Fl_Widget*  (*applet_create_t)(void);
typedef void        (*applet_destroy_t)(Fl_Widget *);

typedef AppletInfo* (*applet_get_info_t)(void);
typedef void        (*applet_destroy_info_t)(AppletInfo *a);

typedef float       (*applet_version_t)(void);

/*
 * The main macro each applet library must implement. It will assign apropriate values
 * so applet loader can use them to load applet class with some common metadata.
 */
#define EDE_PANEL_APPLET_EXPORT(klass, aoptions, aname, aversion, aicon, aauthor) \
extern "C" Fl_Widget *ede_panel_applet_create(void) {                             \
    return new klass;                                                             \
}                                                                                 \
                                                                                  \
extern "C" void ede_panel_applet_destroy(Fl_Widget *w) {                          \
	klass *k = (klass*)w;                                                         \
	delete k;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" AppletInfo *ede_panel_applet_get_info(void) {                          \
	AppletInfo *a = new AppletInfo;                                               \
	a->name = aname;                                                              \
	a->klass_name = #klass;                                                       \
	a->version = aversion;                                                        \
	a->icon = aicon;                                                              \
	a->author = aauthor;                                                          \
	a->options = aoptions;                                                        \
	return a;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" void ede_panel_applet_destroy_info(AppletInfo *a) {                    \
	delete a;                                                                     \
}                                                                                 \
                                                                                  \
extern "C" int ede_panel_applet_get_iface_version(void) {                         \
	return EDE_PANEL_APPLET_INTERFACE_VERSION;                                    \
}

#endif
