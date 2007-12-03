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
#include "Xsm.h"

#include <edelib/List.h>
#include <edelib/String.h>

#include <FL/x.h>
#include <pthread.h>

struct EvokeClient {
	edelib::String desc;      // short program description (used in Starting... message)
	edelib::String icon;      // icon for this client
	edelib::String exec;      // program name/path to run
};

struct EvokeProcess {
	edelib::String cmd;
	pid_t pid;
};

struct QueuedSignal {
	pid_t pid;
	int signum;
};

typedef edelib::list<EvokeClient> ClientList;
typedef edelib::list<EvokeClient>::iterator ClientListIter;

typedef edelib::list<edelib::String> StringList;
typedef edelib::list<edelib::String>::iterator StringListIter;

typedef edelib::list<EvokeProcess> ProcessList;
typedef edelib::list<EvokeProcess>::iterator ProcessListIter;

typedef edelib::list<QueuedSignal> SignalQueue;
typedef edelib::list<QueuedSignal>::iterator SignalQueueIter;

class Fl_Double_Window;

class EvokeService {
	private:
		bool  is_running;
		Log*  logfile;
		Xsm*  xsm;
		char* pidfile;
		char* lockfile;

		Atom _ede_shutdown_all;
		Atom _ede_spawn;
		Atom _ede_evoke_quit;

		ClientList  clients;
		ProcessList processes;
		int wake_up_pipe[2];

	public:
		EvokeService();
		~EvokeService();
		static EvokeService* instance(void);

		void start(void)   { is_running = true; }
		void stop(void)    { is_running = false; }
		bool running(void) { return is_running; }

		bool setup_channels(void);
		bool setup_logging(const char* file);
		bool setup_pid(const char* file, const char* lock);
		void setup_atoms(Display* d);
		bool init_splash(const char* config, bool no_splash, bool dry_run);
		void init_autostart(bool safe);

		void init_xsettings_manager(void);
		void stop_xsettings_manager(bool serialize);

		int handle(const XEvent* ev);

		Log* log(void) { return logfile; }

		void service_watcher(int pid, int signum);
		void wake_up(int fd);

		void run_program(const char* cmd, bool enable_vars = 1);
		void register_process(const char* cmd, pid_t pid);
		void unregister_process(pid_t pid);
		bool find_and_unregister_process(pid_t pid, EvokeProcess& pc);

		void quit_x11(void);
};

#define EVOKE_LOG EvokeService::instance()->log()->printf

/*
 * FIXME: This should be probably declared somewhere outside
 * or in edelib as separate class
 */
class Mutex {
	private:
		pthread_mutex_t mutex;
		Mutex(const Mutex&);
		Mutex& operator=(const Mutex&);
	public:
		Mutex() { pthread_mutex_init(&mutex, 0); }
		~Mutex() { pthread_mutex_destroy(&mutex); }
		void lock(void) { pthread_mutex_lock(&mutex); }
		void unlock(void) { pthread_mutex_unlock(&mutex); }
};

#endif
