/*
 * $Id$
 *
 * ede-screensaver-conf, a tool to configure screensaver
 * Part of Equinox Desktop Environment (EDE).
 * Based on some Xine code I found somewhere
 * Copyright (c) 2009 Sanel Zukan <karijes@equinox-project.org>
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or later.
 * See COPYING for the details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

#include <edelib/File.h>
#include <edelib/Run.h>
#include <edelib/Directory.h>
#include <edelib/TiXml.h>

#include "XScreenSaver.h"

EDELIB_NS_USING(String)
EDELIB_NS_USING(file_path)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(file_rename)
EDELIB_NS_USING(run_program)
EDELIB_NS_USING(dir_home)
EDELIB_NS_USING(dir_exists)
EDELIB_NS_USING(dir_empty)

static Atom XA_SCREENSAVER;
static Atom XA_SCREENSAVER_VERSION;
static Atom XA_DEMO;
static Atom XA_SELECT;

static XErrorHandler old_handler = 0;
static Bool          got_bad_window = False;
static int           atoms_loaded = 0;

static pid_t       xscr_preview_pid = 0;
static const char* xscr_folder_found = 0;

extern char** environ;

static const char* xscr_hacks_dirs[] = {
	PREFIX"/lib/xscreensaver/",
	"/usr/libexec/xscreensaver/",
	"/usr/lib/xscreensaver/",
	"/usr/X11R6/lib/xscreensaver/",
	"/usr/local/lib/xscreensaver/",
	"/lib/xscreensaver/",
	0
};

static const char* xscr_hacks_config_dirs[] = {
	PREFIX"/share/xscreensaver/config/",
	"/usr/share/xscreensaver/config/",
	"/usr/local/share/xscreensaver/config/",
	0
};

#define EAT_SPACES(c)                           \
do {                                            \
	while(*c && (*c == ' ' || *c == '\t')) c++; \
} while(0)

static int bad_window_handler(Display *dpy, XErrorEvent *xevent) {
	if(xevent->error_code == BadWindow) {
		got_bad_window = True;
		return 0;
	}

	if(!old_handler)
		return 0;

	return (*old_handler)(dpy, xevent);
}

/* 
 * convert "xx:xx:xx" to minutes
 * TODO: a better code would be great
 */
static int time_to_min(const char *t) {
	char nb[64];
	const char *p = t;
	unsigned int i = 0;
	int ret = 0;

	for(; *p && *p != ':' && i < sizeof(nb); p++, i++)
		nb[i] = *p;
	p++;
	nb[i] = '\0';

	ret = atoi(nb) * 60;

	i = 0;
	if(*p) {
		for(i = 0; *p && *p != ':' && i < sizeof(nb); p++, i++)
			nb[i] = *p;
		p++;
	}

	nb[i] = '\0';
	ret += atoi(nb);

	return ret;
}

static void min_to_time(unsigned int t, String& ret) {
	ret.clear(); /* so we can re-use it again */

	unsigned int hour = 0, min = 0;
	char buf[3];
	char* fmt;

	/* assure we do not get values that exceedes a day (24 * 60 = 1440) */
	if(t >= 1440) {
		min = 2;
	} else if(t >= 60) {
		hour = t / 60;
		min = t % 60;
	} else {
		min = t;
	}

	/* construct 'xx:yy:zz' time format */
	if(hour >= 10)
		fmt = "%i";
	else
		fmt = "0%i";
	snprintf(buf, sizeof(buf), fmt, hour);

	ret = buf;
	ret += ":";

	if(min >= 10)
		fmt = "%i";
	else
		fmt = "0%i";
	snprintf(buf, sizeof(buf), fmt, min);

	ret += buf;
	ret += ":00";
}

static void xscreensaver_init_atoms_once(Display *dpy) {
	if(atoms_loaded)
		return;

	XA_SCREENSAVER = XInternAtom(dpy, "SCREENSAVER", False);
	XA_SCREENSAVER_VERSION = XInternAtom(dpy, "_SCREENSAVER_VERSION", False);
	XA_DEMO = XInternAtom(dpy, "DEMO", False);
	XA_SELECT = XInternAtom(dpy, "SELECT", False);
	XSync(dpy, 0);

	atoms_loaded = 1;
}

static Window xscreensaver_find_own_window(Display *dpy) {
	Window root = RootWindow(dpy, DefaultScreen(dpy));
	Window root2, parent, *childs;
	unsigned int nchilds;

	if(!XQueryTree(dpy, root, &root2, &parent, &childs, &nchilds))
		return 0;
	if(root != root2)
		return 0;

	for(unsigned int i = 0; i < nchilds; i++) {
		Atom type;
		int format;
		unsigned long nitems, bytesafter;
		char *v;
		int status;

		XSync(dpy, False);
		got_bad_window = False;
		old_handler = XSetErrorHandler(bad_window_handler);

		status = XGetWindowProperty(dpy, childs[i], XA_SCREENSAVER_VERSION, 0, 200, False, XA_STRING,
				&type, &format, &nitems, &bytesafter,
				(unsigned char**)&v);

		XSync(dpy, False);
		XSetErrorHandler(old_handler);
		old_handler = 0;

		if(got_bad_window) {
			status = BadWindow;
			got_bad_window = False;
		}

		if(status == Success && type != None) {
			/* TODO: check if XFree(v) is needed */
			Window ret = childs[i];
			XFree(childs);
			return ret;
		}
	}

	XFree(childs);
	return 0;
}

#if 0
static void xscreensaver_run_hack(Display *dpy, Window id, long hack) {
	XEvent ev;

	ev.xany.type = ClientMessage;
	ev.xclient.display = dpy;
	ev.xclient.window = id;
	ev.xclient.message_type = XA_SCREENSAVER;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XA_SELECT;
	ev.xclient.data.l[1] = hack;
	ev.xclient.data.l[2] = 0;

	/*
	ev.xclient.data.l[0] = XA_DEMO;
	ev.xclient.data.l[1] = 5000;  // XA_DEMO protocol version
	ev.xclient.data.l[2] = hack;
	*/

	if(XSendEvent(dpy, id, False, 0, &ev) == 0)
		puts("XSendEvent() failed");
	else
		XSync(dpy, 0);
}
#endif

static const char *find_hacks_dir_once(void) {
	if(xscr_folder_found)
		return xscr_folder_found;

	for(int i = 0; xscr_hacks_dirs[i]; i++) {
		if(dir_exists(xscr_hacks_dirs[i])) {
			xscr_folder_found = xscr_hacks_dirs[i];
			break;
		}
	}

	return xscr_folder_found;
}

static SaverHack *hack_from_config(const char* path) {
	TiXmlDocument doc(path);

	if(!doc.LoadFile())
		return NULL;

	TiXmlNode* el = doc.FirstChild("screensaver");
	if(!el) 
		return NULL;

	SaverHack* h = new SaverHack;
	h->exec = el->ToElement()->Attribute("name");
	h->name = el->ToElement()->Attribute("_label");

	/* 
	 * now try to find all '<command>' tags and merge values
	 * to form full command line
	 */
	for(el = el->FirstChildElement(); el; el = el->NextSibling()) {
		if(strcmp(el->Value(), "command") == 0) {
			h->exec += " ";
			h->exec += el->ToElement()->Attribute("arg");
		}
	}

	return h;
}

static SaverPrefs *guess_config(void) {
	const char* config_dir = NULL, *hacks_dir;

	for(int i = 0; xscr_hacks_config_dirs[i]; i++) {
		if(dir_exists(xscr_hacks_config_dirs[i]) && !dir_empty(xscr_hacks_config_dirs[i])) {
			config_dir = xscr_hacks_config_dirs[i];
			break;
		}
	}

	E_RETURN_VAL_IF_FAIL(config_dir, NULL);

	hacks_dir = find_hacks_dir_once();
	E_RETURN_VAL_IF_FAIL(hacks_dir, NULL);

	/* 
	 * now do xscreensaver way: try to find xscreensaver hack programs and their matching 
	 * .xml files for name and command args
	 */
	DIR* dfd = opendir(hacks_dir);
	if(!dfd)
		return NULL;

	char config_path[256];
	struct dirent* entry;

	SaverPrefs* sp = new SaverPrefs;
	/* some default values */
	sp->curr_hack = 0;
	sp->timeout = 2;
	sp->mode = SAVER_ONE;
	sp->dpms_enabled = false;
	sp->dpms_standby = 20;
	sp->dpms_suspend = 40;
	sp->dpms_off     = 60;

	unsigned int ind = 0;

	while(1) {
		entry = readdir(dfd);
		if(!entry)
			break;

		/* skip '.' and '..' */
		if(entry->d_name[0] == '.' && 
			(entry->d_name[1] == '\0' || entry->d_name[1] == '.' && entry->d_name[2] == '\0'))
		{
			continue;
		}

		snprintf(config_path, sizeof(config_path), "%s%s.xml", config_dir, entry->d_name);
		SaverHack* h = hack_from_config(config_path);
		if(h) {
			h->sindex = ind++;
			sp->hacks.push_back(h);
		}
	}

	return sp;
}

bool xscreensaver_run_daemon(Display *dpy) {
	xscreensaver_init_atoms_once(dpy);

	Window id = xscreensaver_find_own_window(dpy);

	/* if not running, try to manualy start it */
	if(id == 0) { 
		E_DEBUG(E_STRLOC ": xscreensaver daemon not running, starting it...\n");

		String p = file_path("xscreensaver");
		if(p.empty())
			return false;
 
		/* run 'xscreensaver -nosplash' */
		p += " -nosplash";
		run_program(p.c_str(), false);

		usleep(250000);

		/* check again */
		id = xscreensaver_find_own_window(dpy);
		if(id == 0)
			return false;
	}

	return true;
}

SaverPrefs *xscreensaver_read_config(void) {
	XrmDatabase db;
	XrmValue    xrmv;
	char        *type;

	/*
	 * Luckily, xscreensaver uses X resource for storage
	 * which saves me from parsing... jwz thanx !!!
	 */
	String path = dir_home();
	path += "/.xscreensaver";

	XrmInitialize();

	db = XrmGetFileDatabase(path.c_str());
	if(!db) {
		E_WARNING(E_STRLOC ": Unable to open xscreensaver config file, trying to guess...\n");
		return guess_config();
	}

	SaverPrefs *ret = new SaverPrefs;
	ret->curr_hack = 0;
	ret->timeout = 2;  // in minutes
	ret->mode = SAVER_ONE;
	ret->dpms_enabled = false;
	ret->dpms_standby = ret->dpms_suspend = ret->dpms_off = 30; // in minutes

	if(XrmGetResource(db, "selected", "*", &type, &xrmv) == True && xrmv.addr != NULL) {
		/* 
		 * safe without checks since 0 (if atoi() fails) is first hack
		 * in the list
		 */
		ret->curr_hack = atoi(xrmv.addr);
	}

	if(XrmGetResource(db, "mode", "*", &type, &xrmv) == True && xrmv.addr != NULL) {
		const char *v = xrmv.addr;
		if(!strcasecmp(v, "false") || !strcasecmp(v, "off") || !strcasecmp(v, "no"))
			ret->mode = SAVER_OFF;
		else if(!strcasecmp(v, "blank"))
			ret->mode = SAVER_BLANK;
		else if(!strcasecmp(v, "random"))
			ret->mode = SAVER_RANDOM;
		else
			ret->mode = SAVER_ONE;
	}

	if(XrmGetResource(db, "timeout", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->timeout = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsEnabled", "*", &type, &xrmv) == True && xrmv.addr != NULL) {
		const char *v = xrmv.addr;

		if(!strcasecmp(v, "true") || !strcasecmp(v, "on") || !strcasecmp(v, "yes"))
			ret->dpms_enabled = true;
	}

	if(XrmGetResource(db, "dpmsStandby", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_standby = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsSuspend", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_suspend = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsOff", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_off = time_to_min(xrmv.addr);

	/*
	 * Parse hacks (screensavers), skipping those that starts with '-'. Also, check if hack
	 * contains name beside command (e.g. '"Qix (solid)" qix -root') and use it; otherwise use
	 * capitalized command name.
	 *
	 * Note that to the each hack will be given index number; in the final GUI list they
	 * will be ordered, but xscreensaver keeps own order for selecting that must be preserved.
	 */
	int nhacks = 0;

	if(XrmGetResource(db, "programs", "*", &type, &xrmv) == True) {
		char *programs = strdup(xrmv.addr);
		char *c = NULL;
		char *p = NULL;
		char buf[256];
		unsigned int i;

		for(c = strtok(programs, "\n"); c; c = strtok(NULL, "\n"), nhacks++) {
			/* skip those marked for skipping */
			if(c[0] == '-')
				continue;

			if((p = strstr(c, "GL:")) != NULL) {
				p += 3;
				c = p;
			} 

			EAT_SPACES(c);

			if(*c == '"') {
				/* extract name from '"' */
				c++;

				for(i = 0; i < sizeof(buf) && *c && *c != '"'; c++, i++)
					buf[i] = *c;

				buf[i] = '\0';

				/* skip ending '"' */
				c++;
			} else {
				/* store it so we could back for command line */
				char *tc = c;

				/* or read command and capitalize it */
				for(i = 0; i < sizeof(buf) && *tc && *tc != ' '; tc++, i++)
					buf[i] = *tc;

				buf[i] = '\0';
				buf[0] = toupper(buf[0]);
			}

			SaverHack *h = new SaverHack;
			h->name = buf;
			h->sindex = nhacks;

			/* remove remainig gap */
			EAT_SPACES(c);

			/* 
			 * now go for exec command
			 *
			 * FIXME: it will miss options spread in two or more lines (in Xresource sense)
			 * so when read via XrmGetResource(), it will be merged in a long line with a bunch
			 * of spaces and tabs. See 'crackberg' command
			 */
			for(i = 0; i < sizeof(buf) && *c && *c != '\t'; c++, i++)
				buf[i] = *c;

			buf[i] = '\0';
			h->exec = buf;

			ret->hacks.push_back(h);
		}

		free(programs);
	}

	XrmDestroyDatabase(db);
	return ret;
}

void xscreensaver_save_config(SaverPrefs *sp) {
	String tmp_path = dir_home();
	String path = tmp_path;

	tmp_path += "/.xscreensaver.tmp";
	path +="/.xscreensaver";

	FILE *fd = fopen(tmp_path.c_str(), "w");
	if(!fd) {
		E_WARNING(E_STRLOC ": Unable to write temporary in %s! Nothing will be saved\n", tmp_path.c_str());
		return;
	}

	fprintf(fd, "# XScreenSaver Preferences File\n");
	fprintf(fd, "# Written by ede-screensaver-conf\n\n");

	const char* val;

	switch(sp->mode) {
		case SAVER_OFF:
			val = "off";
			break;
		case SAVER_BLANK:
			val = "blank";
			break;
		case SAVER_RANDOM:
			val = "random";
			break;
		default:
			val = "one";
			break;
	}

	String tm;

	fprintf(fd, "mode: %s\n", val);
	fprintf(fd, "selected: %i\n", sp->curr_hack);

	min_to_time(sp->timeout, tm);
	fprintf(fd, "timeout: %s\n", tm.c_str());

	if(sp->dpms_enabled)
		val = "yes";
	else
		val = "no";

	fprintf(fd, "dpmsEnabled: %s\n", val);

	min_to_time(sp->dpms_standby, tm);
	fprintf(fd, "dpmsStandby: %s\n", tm.c_str());

	min_to_time(sp->dpms_suspend, tm);
	fprintf(fd, "dpmsSuspend: %s\n", tm.c_str());

	min_to_time(sp->dpms_off, tm);
	fprintf(fd, "dpmsOff: %s\n", tm.c_str());
	fprintf(fd, "programs:\t\t\\\n");

	HackListIter it = sp->hacks.begin(), it_end = sp->hacks.end();
	for(; it != it_end; ++it)
		fprintf(fd, "\t\t \"%s\"  %s\t\t \\n\\\n", (*it)->name.c_str(), (*it)->exec.c_str());

	fprintf(fd, "\n\n");

	/* some defaults */
	fprintf(fd, "sgiSaverExtension:	True\n");
	fprintf(fd, "xidleExtension: True\n");
	fprintf(fd, "procInterrupts: True\n");
	fprintf(fd, "GetViewPortIsFullOfLies: False\n");
	fprintf(fd, "demoCommand: xscreensaver-demo\n");
	fprintf(fd, "prefsCommand: xscreensaver-demo -prefs\n");

	fclose(fd);

	/* 
	 * Now open it as Xresource database and merge with the real ~/.xscreensaver
	 * file, so other values we didn't wrote/used are preserved. If it does not
	 * exists, save it as ordinary file
	 */
	XrmInitialize();

	XrmDatabase db = XrmGetFileDatabase(path.c_str());
	if(db) {
		XrmCombineFileDatabase(tmp_path.c_str(), &db, True);
		/* and store it as ~/.xscreensaver */
		XrmPutFileDatabase(db, path.c_str());
		XrmDestroyDatabase(db);
		file_remove(tmp_path.c_str());
	} else {
		file_rename(tmp_path.c_str(), path.c_str());
	}
}

/* run screensaver in in FLTK window */
void xscreensaver_preview(int id, const char* name) {
	const char* hacks_folder = find_hacks_dir_once();
	E_RETURN_IF_FAIL(hacks_folder);

	if(xscr_preview_pid)
		xscreensaver_kill_preview();

	String cmd;
	cmd.printf("%s%s -window-id 0x%X", hacks_folder, name, id);

	pid_t f = fork();
	switch((int)f) {
		case -1:
			E_WARNING(E_STRLOC ": Unable to fork screensaver process\n");
			break;
		case 0: {
			usleep(250000);
			char* argv[4];
			argv[0] = "sh";
			argv[1] = "-c";
			argv[2] = (char*)cmd.c_str();
			argv[3] = NULL;
			execve("/bin/sh", argv, environ);
			/* never reached */
			exit(1);
			break;
		}
		default:
			xscr_preview_pid = f;
			break;
	}
}

void xscreensaver_kill_preview(void) {
	if(xscr_preview_pid) {
		kill(xscr_preview_pid, SIGTERM);
		xscr_preview_pid = 0;
	}
}
