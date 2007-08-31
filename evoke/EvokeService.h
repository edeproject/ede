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
#include <edelib/List.h>
#include <edelib/String.h>
#include <FL/x.h>

struct EvokeClient {
	edelib::String desc;      // short program description (used in Starting... message)
	edelib::String icon;      // icon for this client
	edelib::String exec;      // program name/path to run
};

struct EvokeProcess {
	edelib::String cmd;
	pid_t pid;
};

typedef edelib::list<EvokeClient>           ClientList;
typedef edelib::list<EvokeClient>::iterator ClientListIter;

typedef edelib::list<edelib::String>           StringList;
typedef edelib::list<edelib::String>::iterator StringListIter;

typedef edelib::list<EvokeProcess>           ProcessList;
typedef edelib::list<EvokeProcess>::iterator ProcessListIter;

class Fl_Double_Window;

class EvokeService {
	private:
		bool  is_running;
		Log*  logfile;
		char* pidfile;
		char* lockfile;

		Fl_Double_Window* top_win;

		Atom _ede_shutdown_all;
		Atom _ede_spawn;
		Atom _ede_evoke_quit;

		ClientList  clients;
		ProcessList processes;

	public:
		EvokeService();
		~EvokeService();
		static EvokeService* instance(void);

		void start(void)   { is_running = true; }
		void stop(void)    { is_running = false; }
		bool running(void) { return is_running; }

		bool setup_logging(const char* file);
		bool setup_pid(const char* file, const char* lock);
		void setup_atoms(Display* d);
		bool init_splash(const char* config, bool no_splash, bool dry_run);
		void init_autostart(void);

		int handle(const XEvent* ev);

		Log* log(void) { return logfile; }

		void register_top(Fl_Double_Window* win) { top_win = win; }
		void unregister_top(void) { top_win = NULL; }

		void service_watcher(int pid, int signum);
		void run_program(const char* cmd, bool enable_vars = 1);
		void heuristic_run_program(const char* cmd);
		void register_process(const char* cmd, pid_t pid);
		void unregister_process(pid_t pid);
		bool find_and_unregister_process(pid_t pid, EvokeProcess& pc);

		void quit_x11(void);
};

#define EVOKE_LOG EvokeService::instance()->log()->printf

#endif
