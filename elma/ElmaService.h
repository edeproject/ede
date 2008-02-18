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

#ifndef __ELMASERVICE_H__
#define __ELMASERVICE_H__

#include "Theme.h"

struct ConfigData {
	edelib::String xserver_cmd;
	edelib::String halt_cmd;
	edelib::String reboot_cmd;
	edelib::String login_cmd;
	edelib::String xauth_cmd;

	edelib::String xauth_file;
	edelib::String theme;

	bool numlock;
	bool show_cursor;
};

class ElmaService {
	private:
		ConfigData* config;
		ElmaTheme* theme;

		void execute(const char* cmd);

	public:
		ElmaService();
		~ElmaService();
		static ElmaService* instance(void);

		bool load_config(void);
		bool load_theme(void);

		void display_window(void);
};

#endif
