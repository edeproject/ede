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
#include "Autostart.h"

#include <edelib/File.h>
#include <edelib/Config.h>
#include <edelib/DesktopFile.h>
#include <edelib/Directory.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/Debug.h>
#include <edelib/MessageBox.h>
#include <edelib/Nls.h>

#include <sys/types.h> // getpid
#include <unistd.h>    // pipe
#include <fcntl.h>     // fcntl
#include <stdlib.h>    // free
#include <string.h>    // strdup, memset
#include <errno.h>
#include <signal.h>

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

/*
 * This is re-implementation of XmuClientWindow() so I don't have to link code with libXmu. 
 * XmuClientWindow() will return parent window of given window; this is used so we don't 
 * send delete message to some button or else, but it's parent.
 */
Window mu_try_children(Display* dpy, Window win, Atom wm_state) {
	Atom real;
	Window root, parent;
	Window* children = 0;
	Window ret = 0;
	unsigned int nchildren;
	unsigned char* prop;
	unsigned long n, extra;
	int format;

	if(!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
		return 0;

	for(unsigned int i = 0; (i < nchildren) && !ret; i++) {
		prop = NULL;
		XGetWindowProperty(dpy, children[i], wm_state, 0, 0, False, AnyPropertyType,
				&real, &format, &n, &extra, (unsigned char**)&prop);
		if(prop)
			XFree(prop);
		if(real)
			ret = children[i];
	}

	for(unsigned int i = 0; (i < nchildren) && !ret; i++)
		ret = mu_try_children(dpy, win, wm_state);

	if(children)
		XFree(children);
	return ret;
}

Window mu_client_window(Display* dpy, Window win, Atom wm_state) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;
	int status = XGetWindowProperty(dpy, win, wm_state, 0, 0, False, AnyPropertyType,
			&real, &format, &n, &extra, (unsigned char**)&prop);
	if(prop)
		XFree(prop);

	if(status != Success)
		return win;

	if(real)
		return win;

	Window w = mu_try_children(dpy, win, wm_state);
	if(!w) w = win;

	return w;
}

void service_watcher_cb(int pid, int signum) {
	EvokeService::instance()->service_watcher(pid, signum);
}

void wake_up_cb(int fd, void* v) {
	EvokeService::instance()->wake_up(fd);
}

EvokeService::EvokeService() : 
	is_running(0), logfile(NULL), xsm(NULL), composite(NULL), pidfile(NULL), lockfile(NULL) { 

	wake_up_pipe[0] = wake_up_pipe[1] = -1;
}

EvokeService::~EvokeService() {
	if(logfile)
		delete logfile;

	delete composite;

	stop_xsettings_manager(true);

	if(lockfile) {
		edelib::file_remove(lockfile);
		free(lockfile);
	}

	if(pidfile) {
		edelib::file_remove(pidfile);
		free(pidfile);
	}

	processes.clear();

	if(wake_up_pipe[0] != -1)
		close(wake_up_pipe[0]);
	if(wake_up_pipe[1] != -1)
		close(wake_up_pipe[1]);
}

EvokeService* EvokeService::instance(void) {
	static EvokeService es;
	return &es;
}

bool EvokeService::setup_channels(void) {
	if(pipe(wake_up_pipe) != 0)
		return false;

	fcntl(wake_up_pipe[1], F_SETFL, fcntl(wake_up_pipe[1], F_GETFL) | O_NONBLOCK);
	Fl::add_fd(wake_up_pipe[0], FL_READ, wake_up_cb);
	return true;
}

bool EvokeService::setup_logging(const char* file) {
	if(!file)
		logfile = new DummyLog();
	else
		logfile = new RealLog();

	if(!logfile->open(file)) {
		delete logfile;
		logfile = NULL;
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

	StringList dfiles, sysdirs, tmp;
	StringListIter it, it_end, tmp_it, tmp_it_end;

	edelib::dir_list(adir.c_str(), dfiles, true);

	edelib::system_config_dirs(sysdirs);
	if(!sysdirs.empty()) {
		for(it = sysdirs.begin(), it_end = sysdirs.end(); it != it_end; ++it) {
			*it += autostart_dirname;

			/*
			 * append content
			 * FIXME: too much of copying. There should be some way to merge list items
			 * probably via merge() member
			 */
			edelib::dir_list((*it).c_str(), tmp, true);
			for(tmp_it = tmp.begin(), tmp_it_end = tmp.end(); tmp_it != tmp_it_end; ++tmp_it)
				dfiles.push_back(*tmp_it);
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

void EvokeService::init_xsettings_manager(void) {
	xsm = new Xsm;

	if(Xsm::manager_running(fl_display, fl_screen)) {
		int ret = edelib::ask(_("XSETTINGS manager already running on this screen. Would you like to replace it?"));
		if(ret < 1) {
			stop_xsettings_manager(false);
			return;
		} else
			goto do_it;
	}

do_it:
	if(!xsm->init(fl_display, fl_screen)) {
		edelib::alert(_("Unable to load XSETTINGS manager properly"));
		stop_xsettings_manager(false);
	}

	if(!xsm) return;

	if(xsm->load_serialized("ede-settings.xml"))
		xsm->notify();
}

void EvokeService::stop_xsettings_manager(bool serialize) {
	if(!xsm)
		return;

	if(serialize)
		xsm->save_serialized("ede-settings.xml");

	delete xsm;
	xsm = NULL;
}

void EvokeService::init_composite(void) {
	composite = new Composite();

	if(!composite->init()) {
		delete composite;
		composite = NULL;
	}
}

void EvokeService::setup_atoms(Display* d) {
	// with them must be send '1' or property will be ignored (except _EDE_EVOKE_SPAWN)
	_ede_shutdown_all    = XInternAtom(d, "_EDE_EVOKE_SHUTDOWN_ALL", False);
	_ede_evoke_quit      = XInternAtom(d, "_EDE_EVOKE_QUIT", False);

	_ede_spawn           = XInternAtom(d, "_EDE_EVOKE_SPAWN", False);
}

void EvokeService::quit_x11(void) {
	Window dummy, *wins;
	Window root = RootWindow(fl_display, fl_screen);
	unsigned int n;

	if(!XQueryTree(fl_display, root, &dummy, &dummy, &wins, &n))
		return;

	Atom _wm_protocols     = XInternAtom(fl_display, "WM_PROTOCOLS", False);
	Atom _wm_delete_window = XInternAtom(fl_display, "WM_DELETE_WINDOW", False);
	Atom _wm_state         = XInternAtom(fl_display, "WM_STATE", False);
	XWindowAttributes attr;
	XEvent ev;

	for(unsigned int i = 0; i < n; i++) {
		if(XGetWindowAttributes(fl_display, wins[i], &attr) && (attr.map_state == IsViewable))
			wins[i] = mu_client_window(fl_display, wins[i], _wm_state);
		else
			wins[i] = 0;
	}

	/*
	 * Hm... probably we should first quit known processes started by us
	 * then rest of the X familly
	 */
	for(unsigned int i = 0; i < n; i++) {
		if(wins[i]) {
			EVOKE_LOG("closing %i window\n", i);

			// FIXME: check WM_PROTOCOLS before sending WM_DELETE_WINDOW ???
			memset(&ev, 0, sizeof(ev));
			ev.xclient.type = ClientMessage;
			ev.xclient.window = wins[i];
			ev.xclient.message_type = _wm_protocols;
			ev.xclient.format = 32;
			ev.xclient.data.l[0] = (long)_wm_delete_window;
			ev.xclient.data.l[1] = CurrentTime;
			XSendEvent(fl_display, wins[i], False, 0L, &ev);

			EVOKE_LOG("%i window closed\n", i);
		}
	}

	XSync(fl_display, False);
	sleep(1);

	// kill remaining windows
	for(unsigned int i = 0; i < n; i++) {
		if(wins[i]) { 
			EVOKE_LOG("killing remaining %i window\n", i);
			XKillClient(fl_display, wins[i]);
		}
	}

	XSync(fl_display, False);

	XFree(wins);
	EVOKE_LOG("now close myself\n");
	stop();
}

/*
 * This is run each time when some of the managed childs quits.
 * Instead directly running wake_up(), it will be notified wia
 * wake_up_pipe[] channel, via add_fd() monitor.
 */
void EvokeService::service_watcher(int pid, int ret) {
	write(wake_up_pipe[1], &pid, sizeof(int));
	write(wake_up_pipe[1], &ret, sizeof(int));
}

#include <FL/fl_ask.h>
void EvokeService::wake_up(int fd) {
	puts("=== wake_up() ===");

	int child_pid = -1;
	// child can return anything so there is no default value
	int child_ret;
	if(read(wake_up_pipe[0], &child_pid, sizeof(int)) == -1 || child_pid == -1) {
		puts("unable to read from channel");
		return;
	}

	if(read(wake_up_pipe[0], &child_ret, sizeof(int)) == -1) {
		puts("unable to read from channel");
		return;
	}

	Mutex mutex;

	if(child_ret == SPAWN_CHILD_CRASHED) {
		EvokeProcess pc;
		bool ret;

		mutex.lock();
		ret = find_and_unregister_process(child_pid, pc);
		mutex.unlock();

		if(ret) {
			printf("%s crashed with core dump\n", pc.cmd.c_str());
		}
	} else { 
		mutex.lock();
		unregister_process(child_pid);
		mutex.unlock();

		switch(child_ret) {
			case SPAWN_CHILD_KILLED:
				printf("child %i killed\n", child_pid);
				break;
			case 127:
				edelib::alert(_("Program not found"));
				//fl_alert(_("Program not found"));
				break;
			case 126:
				edelib::alert(_("Program not executable"));
				break;
			default:
				printf("child %i exited with %i\n", child_pid, child_ret);
				break;
		}
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

int EvokeService::composite_handle(const XEvent* xev) {
	if(composite)
		return composite->handle_xevents(xev);
	return 1;
}

/*
 * Main loop for processing got X events.
 *
 * Great care must be taken to route this events to FLTK too (via fl_handle()), since 
 * add_fd() (in evoke.cpp) will not do that. If events are not routed to fltk, popped 
 * dialogs, especially from service_watcher() will not be correctly drawn and will hang 
 * whole program.
 */
int EvokeService::handle(const XEvent* xev) {
	if(xsm && xsm->should_terminate(xev)) {
		EVOKE_LOG("XSETTINGS manager shutdown\n");
		stop_xsettings_manager(true);
		return 1;
	}

	if(xev->type == PropertyNotify) {
		if(xev->xproperty.atom == _ede_spawn) {
			char buff[1024];
			if(get_string_property_value(_ede_spawn, buff, sizeof(buff))) {
				EVOKE_LOG("Got _EDE_EVOKE_SPAWN with %s. Starting client...\n", buff);
				run_program(buff);
			} else {
				EVOKE_LOG("Got _EDE_EVOKE_SPAWN with malformed data. Ignoring...\n");
			}
		}

		if(xev->xproperty.atom == _ede_evoke_quit) {
			int val = get_int_property_value(_ede_evoke_quit);
			if(val == 1) {
				EVOKE_LOG("Got accepted _EDE_EVOKE_QUIT\n");
				stop();
			} else
				EVOKE_LOG("Got _EDE_EVOKE_QUIT with bad code (%i). Ignoring...\n", val);
		}

		if(xev->xproperty.atom == _ede_shutdown_all) {
			int val = get_int_property_value(_ede_shutdown_all);
			if(val == 1) {
				EVOKE_LOG("Got accepted _EDE_EVOKE_SHUTDOWN_ALL\n");

				int dw = DisplayWidth(fl_display, fl_screen);
				int dh = DisplayHeight(fl_display, fl_screen);

				// TODO: add XDM service permissions
				printf("got %i\n", logout_dialog(dw, dh, 1, 1));
				//quit_x11();
			} else	
				EVOKE_LOG("Got _EDE_EVOKE_SHUTDOWN_ALL with bad code (%i). Ignoring...\n", val);
		}
	}

#ifdef USE_FLTK_LOOP_EMULATION
	// let FLTK handle the rest
	fl_handle(*xev);
	return 0;
#else
	// let composite manager do the rest
	return composite_handle(xev);
#endif
}
