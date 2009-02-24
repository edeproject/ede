/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */


#ifndef __EVOKESERVICE_H__
#define __EVOKESERVICE_H__

#include <FL/x.H>
#include <edelib/List.h>
#include <edelib/String.h>

struct StartupItem {
	edelib::String exec;
	edelib::String icon;
	edelib::String description;
};

typedef edelib::list<StartupItem*>           StartupItemList;
typedef edelib::list<StartupItem*>::iterator StartupItemListIter;
class Xsm;

class EvokeService {
private:
	char*           lock_name;
	Xsm*            xsm;
	bool            is_running;
	StartupItemList startup_items;
	edelib::String  splash_theme;

	void clear_startup_items(void);
public:
	EvokeService();
	~EvokeService();
	static EvokeService* instance(void);

	bool setup_lock(const char* name);
	void remove_lock(void);

	void start(void)   { is_running = true; }
	void stop(void)    { is_running = false; }
	bool running(void) { return is_running; }

	void read_startup(void);
	void run_startup(bool splash, bool dryrun);
	int handle(const XEvent* xev);

	void start_xsettings_manager(void);
	void stop_xsettings_manager(bool serialize);
};

#endif
