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
#include "Logout.h"
#include "EvokeService.h"
#include "Splash.h"
#include "Spawn.h"
#include "Crash.h"
#include "Autostart.h"

#include <edelib/File.h>
#include <edelib/Regex.h>
#include <edelib/Config.h>
#include <edelib/DesktopFile.h>
#include <edelib/Directory.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/Debug.h>
#include <edelib/MessageBox.h>
#include <edelib/Nls.h>

#include <FL/fl_ask.h>

#include <sys/types.h> // getpid
#include <unistd.h>    // 
#include <stdlib.h>    // free
#include <string.h>    // strdup, memset
#include <errno.h>     // error codes

void resolve_path(const edelib::String& datadir, edelib::String& item, bool have_datadir) {
	if(item.empty())
		return;

	const char* i = item.c_str();

	if(!edelib::file_exists(i) && have_datadir) {
		item = edelib::build_filename("/", datadir.c_str(), i);
		i = item.c_str();
		if(!edelib::file_exists(i)) {
			// no file, send then empty
			item.clear();
		}
	}
}

char* get_basename(const char* path) {
	char* p = strrchr(path, '/');
	if(p)
		return (p + 1);

	return (char*)path;
}

/*
 * 'Remove' duplicate entries by looking at their basename
 * (aka. filename, but ignoring directory path). Item is not actually removed from
 * the list (will mess up list pointers, but this can be resolved), but data it points
 * to is cleared, which is a sort of marker to caller to skip it. Dumb yes, but very simple.
 *
 * It will use brute force for lookup and 'removal' and (hopfully) it should not have 
 * a large impact on startup since, afaik, no one keeps hundreds of files in autostart 
 * directories (if keeps them, then that issue is not up to this program :-P).
 *
 * Alternative would be to sort items (by their basename) and apply consecutive unique on 
 * them, but... is it worth ?
 */
void basename_unique(StringList& lst) {
	if(lst.empty())
		return;

	StringListIter first, last, first1, last1;
	first = lst.begin();
	last  = lst.end();

	if(first == last)
		return;

	const char* p1, *p2;
	for(; first != last; ++first) {
		for(first1 = lst.begin(), last1 = lst.end(); first1 != last1; ++first1) {
			p1 = (*first).c_str();
			p2 = (*first1).c_str();

			if(first != first1 && strcmp(get_basename(p1), get_basename(p2)) == 0)
				(*first1).clear();
		}
	}
}

int get_int_property_value(Atom at) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen),
			at, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra,
			(unsigned char**)&prop);
	int ret = -1;
	if(status != Success && !prop)
		return ret;
	ret = int(*(long*)prop);
	XFree(prop);
	return ret;
}

int get_string_property_value(Atom at, char* txt, int txt_len) {
	XTextProperty names;
	XGetTextProperty(fl_display, RootWindow(fl_display, fl_screen), &names, at);
	if(!names.nitems || !names.value)
		return 0;

	char** vnames;
	int nsz = 0;
	if(!XTextPropertyToStringList(&names, &vnames, &nsz)) {
		XFree(names.value);
		return 0;
	}

	strncpy(txt, vnames[0], txt_len);
	txt[txt_len] = 0;
	XFreeStringList(vnames);
	return 1;
}

void service_watcher_cb(int pid, int signum) {
	EvokeService::instance()->service_watcher(pid, signum);
}

EvokeService::EvokeService() : is_running(0), logfile(NULL), pidfile(NULL), lockfile(NULL) { 
	top_win = NULL;
}

EvokeService::~EvokeService() {
	if(logfile)
		delete logfile;

	if(lockfile) {
		edelib::file_remove(lockfile);
		free(lockfile);
	}

	if(pidfile) {
		edelib::file_remove(pidfile);
		free(pidfile);
	}

	processes.clear();
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

bool EvokeService::setup_pid(const char* file, const char* lock) {
	if(!file)
		return false;

	if(edelib::file_exists(lock))
		return false;

	FILE* l = fopen(lock, "w");
	if(!l)
		return false;
	fprintf(l, " ");
	fclose(l);
	lockfile = strdup(lock);

	FILE* f = fopen(file, "w");
	if(!f)
		return false;

	fprintf(f, "%i", getpid());
	fclose(f);

	pidfile = strdup(file);
	return true;
}

bool EvokeService::init_splash(const char* config, bool no_splash, bool dry_run) {
	edelib::Config c;
	if(!c.load(config))
		return false;

	char buff[1024];
	bool have_datadir = false;

	c.get("evoke", "DataDirectory", buff, sizeof(buff));

	// no evoke section
	if(c.error() == edelib::CONF_ERR_SECTION)
		return false;

	edelib::String datadir;
	if(c.error() == edelib::CONF_SUCCESS) {
		datadir = buff;
		have_datadir = true;
	}

	edelib::String splashimg;
	if(c.get("evoke", "Splash", buff, sizeof(buff)))
		splashimg = buff;

	edelib::String sound;
	if(c.get("evoke", "Sound", buff, sizeof(buff)))
		sound = buff;

	// Startup key must exists
	if(!c.get("evoke", "Startup", buff, sizeof(buff)))
		return false;

	StringList vs;
	edelib::stringtok(vs, buff, ",");

	// nothing, fine, do nothing
	unsigned int sz = vs.size();
	if(sz == 0)
		return true;

	EvokeClient ec;
	const char* key_name;

	for(StringListIter it = vs.begin(); it != vs.end(); ++it) {
		key_name = (*it).c_str();
		edelib::str_trim((char*)key_name);

		// probably listed but not the same key; also Exec value must exists
		if(!c.get(key_name, "Exec", buff, sizeof(buff)))
			continue;
		else
			ec.exec = buff;

		if(c.get(key_name, "Description", buff, sizeof(buff)))
			ec.desc = buff;

		if(c.get(key_name, "Icon", buff, sizeof(buff)))
			ec.icon = buff;

		clients.push_back(ec);
	}

	/*
	 * Now, before data is send to Splash, resolve directories
	 * since Splash expects that.
	 */
	resolve_path(datadir, splashimg, have_datadir);
	resolve_path(datadir, sound, have_datadir);

	ClientListIter it, it_end;
	for(it = clients.begin(), it_end = clients.end(); it != it_end; ++it)
		resolve_path(datadir, (*it).icon, have_datadir);

	Splash sp(no_splash, dry_run);
	sp.set_clients(&clients);
	sp.set_background(&splashimg);
	sp.set_sound(&sound);

	sp.run();

	return true;
}

/*
 * This is implementation of Autostart Spec (http://standards.freedesktop.org/autostart-spec/autostart-spec-0.5.html).
 * The Autostart Directories are $XDG_CONFIG_DIRS/autostart.
 * If the same filename is located under multiple Autostart Directories 
 * only the file under the most important directory should be used.
 *  Example: If $XDG_CONFIG_HOME is not set the Autostart Directory in the user's home 
 *  directory is ~/.config/autostart/
 *  Example: If $XDG_CONFIG_DIRS is not set the system wide Autostart Directory 
 *  is /etc/xdg/autostart/
 *  Example: If $XDG_CONFIG_HOME and $XDG_CONFIG_DIRS are not set and the two 
 *  files /etc/xdg/autostart/foo.desktop and ~/.config/autostart/foo.desktop exist 
 *  then only the file ~/.config/autostart/foo.desktop will be used because ~/.config/autostart/ 
 *  is more important than /etc/xdg/autostart/
 * If Hidden key is set true in .desktop file, file MUST be ignored.
 * OnlyShowIn and NotShowIn (list of strings identifying desktop environments) if (or if not)
 * contains environment name, MUST not be started/not started.
 * TryExec is same as for .desktop spec.
 */
void EvokeService::init_autostart(bool safe) {
	const char* autostart_dirname = "/autostart/";

	edelib::String adir = edelib::user_config_dir();
	adir += autostart_dirname;

	StringList dfiles, sysdirs;
	StringListIter it, it_end;

	edelib::dir_list(adir.c_str(), dfiles, true);

	edelib::system_config_dirs(sysdirs);
	if(!sysdirs.empty()) {
		for(it = sysdirs.begin(), it_end = sysdirs.end(); it != it_end; ++it) {
			*it += autostart_dirname;
			// append content
			edelib::dir_list((*it).c_str(), dfiles, true, false, false);
		}
	}

	if(dfiles.empty())
		return;

	/*
	 * Remove duplicates where first one seen have priority to be keept. 
	 * This way is required by spec. 
	 *
	 * Also handle this case (noted in spec): 
	 * if $XDG_CONFIG_HOME/autostart/foo.desktop and $XDG_CONFIG_DIRS/autostart/foo.desktop
	 * exists, but $XDG_CONFIG_HOME/autostart/foo.desktop have 'Hidden = true',
	 * $XDG_CONFIG_DIRS/autostart/foo.autostart is ignored too.
	 *
	 * Latter is implied via basename_unique().
	 */
	basename_unique(dfiles);

	const char* name;
	char buff[1024];
	edelib::DesktopFile df;
	edelib::String item_name;

	AstartDialog dlg(dfiles.size(), safe);

	for(it = dfiles.begin(), it_end = dfiles.end(); it != it_end; ++it) {
		if((*it).empty())
			continue;

		name = (*it).c_str();
		if(!edelib::str_ends(name, ".desktop"))
			continue;

		if(!df.load(name)) {
			EVOKE_LOG("Can't load %s. Skipping...\n", name);
			continue;
		}

 		// If Hidden key is set true in .desktop file, file MUST be ignored.
		if(df.hidden())
			continue;

		if(df.name(buff, sizeof(buff)))
			item_name = buff;
		else
			item_name = name;

		if(df.try_exec(buff, sizeof(buff)) || df.exec(buff, sizeof(buff)))
			dlg.add_item(item_name, buff);
	}

	if(dlg.list_size() > 0)
		dlg.run();
}

void EvokeService::setup_atoms(Display* d) {
	// with them must be send '1' or property will be ignored (except _EDE_EVOKE_SPAWN)
	_ede_shutdown_all    = XInternAtom(d, "_EDE_EVOKE_SHUTDOWN_ALL", False);
	_ede_evoke_quit      = XInternAtom(d, "_EDE_EVOKE_QUIT", False);

	_ede_spawn           = XInternAtom(d, "_EDE_EVOKE_SPAWN", False);
}

void EvokeService::quit_x11(void) {
	int ret = edelib::ask(_("Nice quitting is not implemented yet and this will forcefully kill\nall running applications. Make sure to save what needs to be saved.\nSo, would you like to continue ?"));
	if(ret)
		stop();
#if 0
	/*
	 * This code is working, but not as I would like to see.
	 * It will (mostly) call XKillClient() for _every_ window
	 * (including i.e. buttons in some app), and that is not
	 * nice way to say good bye. This must be implemented in
	 * wm since it only knows what is actuall window and what not. 
	 * For now quit_x11() will simply quit itself, quitting X11
	 * session (if it holds it), which will in turn forcefully kill
	 * all windows.
	 */
	Window dummy, *wins;
	Window root = RootWindow(fl_display, fl_screen);
	unsigned int n;

	if(!XQueryTree(fl_display, root, &dummy, &dummy, &wins, &n))
		return;

	Atom _wm_protocols     = XInternAtom(fl_display, "WM_PROTOCOLS", False);
	Atom _wm_delete_window = XInternAtom(fl_display, "WM_DELETE_WINDOW", False);
	Atom* protocols;
	int np;
	XEvent ev;
	bool have_quit = 0;

	for(unsigned int i = 0; i < n; i++) {
		if(wins[i] == root)
			continue;

		have_quit = 0;
		if(XGetWMProtocols(fl_display, wins[i], &protocols, &np)) {
			for(int j = 0; j < np; j++) {
				if(protocols[j] == _wm_delete_window) {
					have_quit = 1;
					break;
				}
			}
		}

		if(have_quit) {
			memset(&ev, 0, sizeof(ev));
			ev.xclient.type = ClientMessage;
			ev.xclient.window = wins[i];
			ev.xclient.message_type = _wm_protocols;
			ev.xclient.format = 32;
			ev.xclient.data.l[0] = (long)_wm_delete_window;
			ev.xclient.data.l[1] = (long)fl_event_time;
			XSendEvent(fl_display, wins[i], False, 0L, &ev);
		} else {
			XKillClient(fl_display, wins[i]);
		}
	}

	XFree((void*)wins);
	stop();
#endif
}

/*
 * Monitor starting service and report if staring failed. Also if one of 
 * runned services crashed attach gdb on it pid and run backtrace.
 */
void EvokeService::service_watcher(int pid, int signum) {
	printf("got %i\n", signum);
	Mutex mutex;

	if(signum == SPAWN_CHILD_CRASHED) {
		EvokeProcess pc;

		mutex.lock();
		bool ret = find_and_unregister_process(pid, pc);
		mutex.unlock();

		if(ret) {
			printf("%s crashed with core dump\n", pc.cmd.c_str());

			CrashDialog cdialog;
			cdialog.set_data(pc.cmd.c_str());
			cdialog.run();
		}
		return;
	} 

	mutex.lock();
	unregister_process(pid);
	mutex.unlock();
	
	if(signum == SPAWN_CHILD_KILLED) {
		printf("child %i killed\n", pid);
	} else if(signum == 127) {
		edelib::alert(_("Program not found"));
	} else if(signum == 126) {
		edelib::alert(_("Program not executable"));
	} else {
		printf("child %i exited with %i\n", pid, signum);
	}
}

/*
 * Execute program. It's return status
 * will be reported via service_watcher()
 */
void EvokeService::run_program(const char* cmd, bool enable_vars) {
	EASSERT(cmd != NULL);

	edelib::String scmd(cmd);
	pid_t child;

	if(enable_vars && scmd.length() > 6) {
		if(scmd.substr(0, 6) == "$TERM ") {
			char* term = getenv("TERM");
			if(!term)
				term = "xterm";

			edelib::String tmp(term);
			tmp += " -e ";
			tmp += scmd.substr(6, scmd.length());

			scmd = tmp;
		}
	}

	int r = spawn_program_with_core(scmd.c_str(), service_watcher_cb, &child);

	if(r != 0)
		edelib::alert("Unable to start %s. Got code %i", cmd, r);
	else {
		Mutex mutex;

		mutex.lock();
		register_process(scmd.c_str(), child);
		mutex.unlock();
	}
}

#if 0
void EvokeService::heuristic_run_program(const char* cmd) {
	if(strncmp(cmd, "$TERM ", 6) == 0) {
		run_program(cmd);
		return;
	}

	edelib::String ldd = edelib::file_path("ldd");
	if(ldd.empty()) {
		run_program(cmd);
		return;
	}

	ldd += " ";
	ldd += edelib::file_path(cmd);

	int r = spawn_program(ldd.c_str(), 0, 0, "/tmp/eshrun");
	printf("%s\n", ldd.c_str());
	if(r != 0) {
		printf("spawn %i\n", r);
		return;
	}

	sleep(1);

	//edelib::File f;
	FILE* f = fopen("/tmp/eshrun", "r");
	if(!f) {
		puts("File::open");
		return;
	} else {
		puts("opened /tmp/eshrun");
	}

	edelib::Regex rx;
	rx.compile("^\\s*libX11");
	char buff[1024];
	bool is_gui = 0;

	while(fgets(buff, sizeof(buff)-1, f)) {
		printf("checking %s\n", buff);

		if(rx.match(buff)) {
			printf("found libX11\n");
			is_gui = 1;
			//break;
		}
	}

	fclose(f);

	edelib::String fcmd;
	if(!is_gui) {
		fcmd = "$TERM ";
		fcmd += cmd;
	} else {
		fcmd = cmd;
	}

	run_program(fcmd.c_str());
}
#endif

void EvokeService::register_process(const char* cmd, pid_t pid) {
	EvokeProcess pc;
	pc.cmd = cmd;
	pc.pid = pid;
	printf("registering %s with %i\n", cmd, pid);
	processes.push_back(pc);
}

void EvokeService::unregister_process(pid_t pid) {
	if(processes.empty())
		return;

	ProcessListIter it = processes.begin();
	ProcessListIter it_end = processes.end();

	while(it != it_end) {
		if((*it).pid == pid) {
			printf("Found %s with pid %i, cleaning...\n", (*it).cmd.c_str(), pid);
			processes.erase(it);
			return;
		}
		++it;
	}
}

bool EvokeService::find_and_unregister_process(pid_t pid, EvokeProcess& pc) {
	if(processes.empty())
		return 0;

	ProcessListIter it = processes.begin();
	ProcessListIter it_end = processes.end();

	while(it != it_end) {
		if((*it).pid == pid) {
			printf("Found %s with pid %i, cleaning...\n", (*it).cmd.c_str(), pid);
			pc.cmd = (*it).cmd;
			pc.pid = pid;

			processes.erase(it);
			return 1;
		}
		++it;
	}

	return 0;
}

int EvokeService::handle(const XEvent* ev) {
	logfile->printf("Got event %i\n", ev->type);

	if(ev->type == MapNotify) {
		if(top_win) {
			// for splash to keep it at top (working in old edewm)
			XRaiseWindow(fl_display, fl_xid(top_win));
			return 1;
		}
	} else if(ev->type == ConfigureNotify) {
	 	if(ev->xconfigure.event == DefaultRootWindow(fl_display) && top_win) {
			// splash too, but keep window under other wm's
			XRaiseWindow(fl_display, fl_xid(top_win));
			return 1;
		}
	} else if(ev->type == PropertyNotify) {
		if(ev->xproperty.atom == _ede_spawn) {
			char buff[1024];
			if(get_string_property_value(_ede_spawn, buff, sizeof(buff))) {
				logfile->printf("Got _EDE_EVOKE_SPAWN with %s. Starting client...\n", buff);
				run_program(buff);
				//heuristic_run_program(buff);
			} else {
				logfile->printf("Got _EDE_EVOKE_SPAWN with malformed data. Ignoring...\n");
			}
			return 1;
		}

		if(ev->xproperty.atom == _ede_evoke_quit) {
			int val = get_int_property_value(_ede_evoke_quit);
			if(val == 1) {
				logfile->printf("Got accepted _EDE_EVOKE_QUIT\n");
				stop();
			} else
				logfile->printf("Got _EDE_EVOKE_QUIT with bad code (%i). Ignoring...\n", val);
			return 1;
		}

		if(ev->xproperty.atom == _ede_shutdown_all) {
			int val = get_int_property_value(_ede_shutdown_all);
			if(val == 1) {
				logfile->printf("Got accepted _EDE_EVOKE_SHUTDOWN_ALL\n");

				int dw = DisplayWidth(fl_display, fl_screen);
				int dh = DisplayHeight(fl_display, fl_screen);

				printf("got %i\n", logout_dialog(dw, dh));
				// quit_x11();
			} else	
				logfile->printf("Got _EDE_EVOKE_SHUTDOWN_ALL with bad code (%i). Ignoring...\n", val);
			return 1;
		}
	}

	return 0;
}
