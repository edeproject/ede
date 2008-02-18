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

#include "Theme.h"
#include <edelib/Config.h>
#include <edelib/File.h>
#include <edelib/Debug.h>

#define DEFAULT_THEME_FILE "elma.theme"

ElmaTheme* elma_theme_init(const char* directory) {
	edelib::String path;
	path.printf("%s/%s", directory, DEFAULT_THEME_FILE);

	edelib::Config conf;

	if(!conf.load(path.c_str())) {
		EWARNING(ESTRLOC ": Can' load %s\n", path.c_str());
		return NULL;
	}

	ElmaTheme* et = new ElmaTheme;
	char buff[1024];

	conf.get("theme", "background", buff, sizeof(buff));
	et->background.printf("%s/%s", directory, buff);

	conf.get("theme", "panel", buff, sizeof(buff));
	et->panel.printf("%s/%s", directory, buff);

	conf.get("theme", "panel_x", et->panel_x, 5);
	conf.get("theme", "panel_y", et->panel_y, 5);

	ThemeBox* info = new ThemeBox;

	conf.get("theme", "info_x", info->x, 5);
	conf.get("theme", "info_y", info->y, 5);
	conf.get("theme", "info_width", info->w, 5);
	conf.get("theme", "info_height", info->h, 5);
	conf.get("theme", "info_color", info->font_color, 255); // FL_WHITE default
	conf.get("theme", "info_font_size", info->font_size, 12);
	if(conf.get("theme", "info_text", buff, sizeof(buff)))
		info->label = new edelib::String(buff);
	else
		info->label = NULL;

	et->info = info;

	ThemeBox* user = new ThemeBox;

	conf.get("theme", "username_x", user->x, 10);
	conf.get("theme", "username_y", user->y, 10);
	conf.get("theme", "username_width", user->w, 95);
	conf.get("theme", "username_height", user->h, 25);
	conf.get("theme", "username_color", user->font_color, 255); // FL_WHITE default
	conf.get("theme", "username_font_size", user->font_size, 12);
	if(conf.get("theme", "username_text", buff, sizeof(buff))) {
		user->label = new edelib::String(buff);
		// append few spaces
		*(user->label) += "   ";
	} else
		user->label = NULL;

	et->user = user;

	ThemeBox* pass = new ThemeBox;

	conf.get("theme", "password_x", pass->x, 50);
	conf.get("theme", "password_y", pass->y, 50);
	conf.get("theme", "password_width", pass->w, 95);
	conf.get("theme", "password_height", pass->h, 25);
	conf.get("theme", "password_color", pass->font_color, 255); // FL_WHITE default
	conf.get("theme", "password_font_size", pass->font_size, 12);
	if(conf.get("theme", "password_text", buff, sizeof(buff))) {
		pass->label = new edelib::String(buff);
		// append few spaces
		*(pass->label) += "   ";
	} else
		pass->label = NULL;

	et->pass = pass;

	ThemeBox* error = new ThemeBox;

	conf.get("theme", "error_x", error->x, 30);
	conf.get("theme", "error_y", error->y, 30);
	conf.get("theme", "error_width", error->w, 100);
	conf.get("theme", "error_height", error->h, 25);
	conf.get("theme", "error_color", error->font_color, 255); // FL_WHITE default
	conf.get("theme", "error_font_size", error->font_size, 12); // FL_WHITE default
	error->label = NULL;

	et->error = error;

	return et;
}

void elma_theme_clear(ElmaTheme* et) {
	if(!et)
		return;

	if(et->info) {
		delete et->info->label;
		delete et->info;
	}

	if(et->user) {
		delete et->user->label;
		delete et->user;
	}

	if(et->pass) {
		delete et->pass->label;
		delete et->pass;
	}

	delete et->error;
	delete et;
}
