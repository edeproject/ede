/*
 * $Id$
 *
 * edewm (EDE Window Manager) settings
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef ewmconf_h
#define ewmconf_h

#include <fltk/Window.h>
#include <fltk/TabGroup.h>
#include <fltk/Group.h>
#include <fltk/Choice.h>
#include <fltk/Item.h>
#include <fltk/ValueInput.h>
#include <fltk/Button.h>
#include <fltk/CheckButton.h>
#include <fltk/Input.h>
#include <fltk/Divider.h>
#include <fltk/ValueSlider.h>

extern fltk::Button* titlebarLabelColorButton;
extern fltk::Button* titlebarColorButton;
extern fltk::Button* titlebarActiveLabelColorButton;
extern fltk::Button* titlebarActiveColorButton;
extern fltk::Choice* titlebarDrawGrad;
extern fltk::CheckButton* useThemeButton;
extern fltk::Input* themePathInput;
extern fltk::Button* browse_btn;
extern fltk::CheckButton* animateButton;
extern fltk::ValueSlider* animateSlider;
extern fltk::CheckButton* opaqueResize;

void changeBoxColor(fltk::Button *box);

#endif
