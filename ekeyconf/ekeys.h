/*
 * $Id$
 *
 * Keyboard shortcuts applet
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2005-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef EICON_H
#define EICON_H

#include <stdlib.h>
#include <fltk/events.h> //#include <efltk/Fl.h>
#include <fltk/ask.h> //#include <efltk/fl_ask.h>
//#include <efltk/Fl_Color_Chooser.h>
#include <fltk/x.h> //#include <efltk/x.h>
#include "../edelib2/Config.h" //#include <efltk/Fl_Config.h>
#include "../edelib2/NLS.h" //#include <efltk/Fl_Locale.h>
#include <fltk/InputBrowser.h> //#include <efltk/Fl_Input_Browser.h>


int getshortcutfor(const char*);
void setshortcutfor(const char*, int);
void readKeysConfiguration();
void writeKeysConfiguration();
void sendUpdateInfo();
void populatelist(fltk::InputBrowser *);
void addShortcut(const char*,const char*);
void removeShortcut(const char*);

#endif

