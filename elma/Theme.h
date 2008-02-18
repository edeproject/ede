/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __THEME_H__
#define __THEME_H__

#include <edelib/String.h>

struct ThemeBox {
	int x, y, w, h;
	int font_color;
	int font_size;
	edelib::String* label;
};

struct ElmaTheme {
	ThemeBox* info;
	ThemeBox* user;
	ThemeBox* pass;
	ThemeBox* error;

	int panel_x;
	int panel_y;
	edelib::String panel;

	edelib::String background;
};

ElmaTheme* elma_theme_init(const char* directory);
void       elma_theme_clear(ElmaTheme* et);

#endif
