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

#include "ElmaService.h"
#include "ElmaWindow.h"

#include <edelib/Config.h>

#include <FL/Fl.h>
#include <FL/x.h>
#include <stdlib.h>

ElmaService::ElmaService() : config(NULL), theme(NULL) {
}

ElmaService::~ElmaService() {
	delete config;
	elma_theme_clear(theme);
}

ElmaService* ElmaService::instance(void) {
	static ElmaService es;
	return &es;
}

void ElmaService::execute(const char* cmd) {
	system(cmd);
}

bool ElmaService::load_config(void) {
	config = new ConfigData;

	edelib::Config cfile;

	if(!cfile.load("elma.conf"))
		return false;

	char buff[256];
	unsigned int buffsz = sizeof(buff);

	if(cfile.get("elma", "xserver", buff, buffsz))
		config->xserver_cmd = buff;

	if(cfile.get("elma", "halt", buff, buffsz))
		config->halt_cmd = buff;

	if(cfile.get("elma", "reboot", buff, buffsz))
		config->reboot_cmd = buff;

	if(cfile.get("elma", "login_cmd", buff, buffsz))
		config->login_cmd = buff;

	if(cfile.get("elma", "xauth", buff, buffsz))
		config->xauth_cmd = buff;

	if(cfile.get("elma", "xauth_file", buff, buffsz))
		config->xauth_file = buff;

	if(cfile.get("elma", "theme", buff, buffsz))
		config->theme = buff;

	cfile.get("elma", "numlock", config->numlock, false);
	cfile.get("elma", "cursor", config->show_cursor, false);

	return true;
}

bool ElmaService::load_theme(void) {
	theme = elma_theme_init("themes/default");

	if(theme == NULL)
		return false;
	return true;
}

void ElmaService::display_window(void) {
	fl_open_display();

	int dx, dy, dw, dh;
	Fl::screen_xywh(dx, dy, dw, dh);

	//numlock_xkb_init(fl_display);

	ElmaWindow* win = new ElmaWindow(dw, dh);

	if(!win->create_window(theme)) {
		delete win;
		return;
	}

	win->clear_border();
	win->show();
	win->cursor(FL_CURSOR_NONE);

	Fl::run();
}
