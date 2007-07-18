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

#ifndef ewm_h
#define ewm_h

#include <fltk/Color.h>


extern fltk::Color title_active_color, title_active_color_text;
extern fltk::Color title_normal_color, title_normal_color_text;
extern bool opaque_resize;
extern int title_draw_grad;
extern bool animate;
extern int animate_speed;
extern bool use_frame;
extern fltk::Color theme_frame_color;
extern bool use_theme;
extern char* theme_path;
extern int title_height;
extern int title_align;

void readConfiguration();
void applyConfiguration();
void writeConfiguration();

#endif

