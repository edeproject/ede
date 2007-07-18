/*
 * $Id$
 *
 * Configure window for eworkpanel
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef epanelconf_h
#define epanelconf_h
#include <fltk/Window.h>
#include <fltk/Button.h>
#include <fltk/TabGroup.h>
#include <fltk/Group.h>
#include <fltk/Input.h>
#include <fltk/CheckButton.h>
#include <fltk/ValueSlider.h>
#include <fltk/file_chooser.h>
#include "../edelib2/NLS.h"

// Widgets accessed from util.cpp
extern fltk::Input *workspaces[8];

extern fltk::Input* vcProgram;
extern fltk::Input* tdProgram;
extern fltk::Input* browserProgram;
extern fltk::Input* terminalProgram;
extern fltk::CheckButton* autohide_check;
extern fltk::ValueSlider* ws_slider;

#endif
