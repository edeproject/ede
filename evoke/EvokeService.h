/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __EVOKESERVICE_H__
#define __EVOKESERVICE_H__

#include "Log.h"
#include <X11/Xlib.h>

#include <edelib/Vector.h>
#include <edelib/String.h>

struct EvokeClient {
	edelib::String message;   // message during startup
	edelib::String icon;      // icon for this client
	edelib::String exec;      // program name/path to run
	bool core;                // does understaind _EDE_SHUTDOWN_SELF (only ede apps)
};

class EvokeService {
	private:
		Log*  logfile;
		char* pidfile;

		Atom _ede_shutdown_all;
		Atom _ede_shutdown_client;
		Atom _ede_spawn;

		edelib::vector<EvokeClient> clients;

	public:
		EvokeService();
		~EvokeService();
		static EvokeService* instance(void);

		bool setup_logging(const char* file);
		bool setup_pid(const char* file);
		bool setup_config(const char* config, bool do_startup);
		void setup_atoms(Display* d);

		int handle(int event);

		Log* log(void) { return logfile; }
};

#define EVOKE_LOG EvokeService::instance()->log()->printf

#endif
