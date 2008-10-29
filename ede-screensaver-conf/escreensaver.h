/*
 * $Id$
 *
 * Screensaver configuration
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef escreensaver_h
#define escreensaver_h

#include <stdio.h>
#include <stdlib.h>

/*#include <efltk/Fl_Window.h>
#include <efltk/Fl_Item.h>
#include <efltk/x.h>
#include <efltk/Fl_Input_Browser.h>
#include <efltk/Fl_Locale.h>*/

#include <fltk/Window.h>
#include <fltk/Item.h>
#include <fltk/InputBrowser.h>
#include "../edelib2/NLS.h"

void fillSaversList(fltk::InputBrowser *);
void startSaverPreview();
void readConfiguration();
void writeConfiguration();
void writeConfigurationSaver();
void getScreenhacks();
void clearOnExit();

#endif



