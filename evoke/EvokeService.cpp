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

#include "Log.h"
#include "EvokeService.h"
#include <edelib/File.h>
#include <edelib/Config.h>
#include <edelib/StrUtil.h>

#include <sys/types.h> // getpid
#include <unistd.h>    // 
#include <stdlib.h>    // free
#include <string.h>    // strdup


EvokeService::EvokeService() : logfile(NULL), pidfile(NULL) { 
}

EvokeService::~EvokeService() {
	if(logfile)
		delete logfile;

	if(pidfile) {
		edelib::file_remove(pidfile);
		free(pidfile);
	}
}

EvokeService* EvokeService::instance(void) {
	static EvokeService es;
	return &es;
}

bool EvokeService::setup_logging(const char* file) {
	if(!file)
		logfile = new DummyLog();
	else
		logfile = new RealLog();

	if(!logfile->open(file)) {
		delete logfile;
		return false;
	}

	return true;
}

bool EvokeService::setup_pid(const char* file) {
	if(!file)
		return false;

	if(edelib::file_exists(file))
		return false;

	FILE* f = fopen(file, "w");
	if(!f)
		return false;

	fprintf(f, "%i", getpid());
	fclose(f);

	pidfile = strdup(file);
	return true;
}

bool EvokeService::setup_config(const char* config, bool do_startup) {
	// for now if is not startup mode, ignore it
	if(!do_startup)
		return true;

	edelib::Config c;
	if(!c.load(config))
		return false;

	char buff[1024];
	if(!c.get("evoke", "Startup", buff, sizeof(buff)))
		return false;

	edelib::vector<edelib::String> vs;
	edelib::stringtok(vs, buff, ",");

	// nothing, fine, do nothing
	unsigned int sz = vs.size();
	if(sz == 0)
		return true;

	EvokeClient ec;
	const char* key_name;
	for(unsigned int i = 0; i < sz; i++) {
		key_name = vs[i].c_str();
		edelib::str_trim((char*)key_name);

		// probably listed but not the same key; also Exec value must exists
		if(!c.get(key_name, "Exec", buff, sizeof(buff)))
			continue;
		else
			ec.exec = buff;

		if(c.get(key_name, "Message", buff, sizeof(buff)))
			ec.message = buff;
		// it is no EDE app untill say so
		c.get(key_name, "Core", ec.core, false);

		if(c.get(key_name, "Icon", buff, sizeof(buff)))
			ec.icon = buff;

		clients.push_back(ec);
	}

	for(unsigned int i = 0; i < clients.size(); i++) {
		printf("Exec: %s\n", clients[i].exec.c_str());
		printf("Message: %s\n", clients[i].message.c_str());
		printf("Icon: %s\n", clients[i].icon.c_str());
		printf("Core: %i\n\n", clients[i].core);
	}

	return true;
}

void EvokeService::setup_atoms(Display* d) {
	_ede_shutdown_all    = XInternAtom(d, "_EDE_EVOKE_SHUTDOWN_ALL", False);
	_ede_spawn           = XInternAtom(d, "_EDE_EVOKE_SPAWN", False);
	_ede_shutdown_client = XInternAtom(d, "_EDE_SHUTDOWN", False);
}

int EvokeService::handle(int event) {
	logfile->printf("Got event %i\n", event);
	return 0;
}
