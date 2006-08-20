/*
 * $Id$
 *
 * Volume control application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef prefs_h
#define prefs_h
//#include <efltk/Fl.h>
#include "../edelib2/NLS.h" //#include <efltk/Fl_Locale.h>
void choice_items(char *path);
#include <fltk/Window.h>//#include <efltk/Fl_Window.h>
extern fltk::Window* preferencesWindow;
#include <fltk/TabGroup.h>//#include <efltk/Fl_Tabs.h>
#include <fltk/Group.h>//#include <efltk/Fl_Group.h>
#include <fltk/InputBrowser.h>//#include <efltk/Fl_Input_Browser.h>
extern fltk::InputBrowser* deviceNameInput;
#include <fltk/Button.h>//#include <efltk/Fl_Button.h>
#include <fcntl.h>
extern void update_info();
void PreferencesDialog(fltk::Widget *, void *);
#endif
