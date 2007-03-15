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

#ifndef escrsaverconf_h
#define escrsaverconf_h
#include <fltk/Menu.h>
#include <fltk/Window.h>
extern fltk::Window* mainWindow;
#include <fltk/Group.h>
#include <fltk/InputBrowser.h>
extern fltk::InputBrowser* saversList;
#include <fltk/ValueInput.h>
extern fltk::ValueInput* timeoutSlider;
#include <fltk/Button.h>
extern fltk::Group* dpmsGroup;
extern fltk::ValueInput* standbySlider;
extern fltk::ValueInput* suspendSlider;
extern fltk::ValueInput* offSlider;
#include <fltk/CheckButton.h>
extern fltk::CheckButton* enableDPMSCheck;
#include <fltk/InvisibleBox.h>
extern fltk::Window* saverWindow;
int main(int argc, char **argv);
#endif
