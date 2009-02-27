/*
 * $Id$
 *
 * evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <stdio.h> // snprintf
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl.H>
#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/Util.h>
#include <edelib/Directory.h>
#include <edelib/Resource.h>
#include <edelib/Run.h>

#include "Splash.h"

#define TIMEOUT_START    0.5  /* timeout when splash is first time shown (also for first client) */
#define TIMEOUT_CONTINUE 2.0  /* timeout between starting rest of the cliens */

EDELIB_NS_USING(String)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(build_filename)
EDELIB_NS_USING(dir_exists)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(RES_SYS_ONLY)

#ifndef EDEWM_HAVE_NET_SPLASH
static Splash* global_splash = NULL;

static int splash_xmessage_handler(int e) {
	if(fl_xevent->type == MapNotify) {
		XRaiseWindow(fl_display, fl_xid(global_splash));
		return 1;
	}

	if(fl_xevent->type == ConfigureNotify) {
	 	if(fl_xevent->xconfigure.event == DefaultRootWindow(fl_display)) {
			XRaiseWindow(fl_display, fl_xid(global_splash));
			return 1;
		}
	}

	return 0;
}
#endif

/*
 * repeatedly call runner() until all clients are 
 * started then hide splash window
 */
static void runner_cb(void* s) {
	Splash* sp = (Splash*)s;

	if(sp->next_client())
		Fl::repeat_timeout(TIMEOUT_CONTINUE, runner_cb, sp);
	else
		sp->hide();
}

Splash::Splash(StartupItemList& s, String& theme, bool show_it, bool dr) : Fl_Double_Window(480, 365) {
	slist = &s;
	splash_theme = &theme;
	show_splash = show_it;
	dryrun = dr;
	icons = NULL;
	counter = 0;

	box(FL_BORDER_BOX);
}
	

Splash::~Splash() {
	E_DEBUG(E_STRLOC ": Cleaning splash data\n");
	/* elements of icons cleans Fl_Group */
	delete [] icons;
	Fl::remove_timeout(runner_cb);
}


/* after edewm got _NET_WM_WINDOW_TYPE_SPLASH support */
#if EDEWM_HAVE_NET_SPLASH
void Splash::show(void) {
	if(shown())
		return;

	Fl_X::make_xid(this);
	/* 
	 * Edewm does not implement this for now. Alternative, working solution
	 * is used via register_top()/unregister_top(); also looks like later 
	 * is working on othe wm's too.
	 */
	Atom win_type   = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);
	Atom win_splash = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
	XChangeProperty(fl_display, fl_xid(this), win_type, XA_ATOM, 32, PropModeReplace,
			(unsigned char*)&win_splash, sizeof(Atom));
}
#endif

void Splash::run(void) {
	E_ASSERT(slist != NULL);

	if(!show_splash) {
		while(next_client_nosplash()) 
			;
		return;
	}

	fl_register_images();

	String path, splash_theme_path;

#ifdef USE_LOCAL_CONFIG
	splash_theme_path = "splash-themes/";
#else
	splash_theme_path = Resource::find_data("themes/splash-themes", RES_SYS_ONLY);
	if(splash_theme_path.empty()) {
		E_WARNING(E_STRLOC ": Unable to locate splash themes in $XDG_DATA_DIRS directories\n");
		return;
	}
#endif
	splash_theme_path += E_DIR_SEPARATOR;
	splash_theme_path += *splash_theme;

	if(!dir_exists(splash_theme_path.c_str())) {
		E_WARNING(E_STRLOC ": Unable to locate '%s' in '%s' theme directory\n", 
				splash_theme->c_str(), splash_theme_path.c_str());
		return;
	}

	/* setup widgets */
	begin();
		Fl_Box* bimg = new Fl_Box(0, 0, w(), h());
		Fl_Image* splash_img = 0;

		path = build_filename(splash_theme_path.c_str(), "background.png");
		splash_img = Fl_Shared_Image::get(path.c_str());

		if(splash_img) {
			int W = splash_img->w();
			int H = splash_img->h();
			/* update window and Box sizes */
			size(W, H);
			bimg->size(W, H);

			bimg->image(splash_img);
		}

		/*
		 * place message box at the bottom with
		 * nice offset (10 px) from window borders
		 */
		msgbox = new Fl_Box(10, h() - 25 - 10, w() - 20, 25);

		/*
		 * Setup icons positions, based on position of msgbox assuming someone 
		 * will not abuse splash to start hundrets of programs, since icons will 
		 * be placed only in horizontal order, one line, so in case their large
		 * number, some of them will go out of window borders.
		 *
		 * Icon box will be 64x64 so larger icons can fit too.
		 *
		 * This code will use Fl_Group (moving group, later, will move all icons
		 * saving me from code mess), but will not update it's w() for two reasons:
		 * icons does not use it, and will be drawn correctly, and second, setting
		 * width will initiate fltk layout engine which will mess everything up.
		 */
		Fl_Group* icon_group = new Fl_Group(10, msgbox->y() - 10 - 64, 0, 64);
		int X = icon_group->x();
		int Y = icon_group->y();

		/* offset between icons */
		int ioffset = 5;

		/* FIXME: use malloc/something instead this */
		icons = new Fl_Box*[slist->size()];

		icon_group->begin();
			int         i = 0;
			const char* imgpath;
			Fl_Image*   iconimg = 0;

			for(StartupItemListIter it = slist->begin(); it != slist->end(); ++it, ++i) {
				Fl_Box* bb = new Fl_Box(X, Y, 64, 64);

				path = build_filename(splash_theme_path.c_str(), (*it)->icon.c_str());
				imgpath = path.c_str();
				iconimg = Fl_Shared_Image::get(imgpath);

				if(!iconimg) {
					bb->label(_("No image"));
					bb->align(FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
				} else 
					bb->image(iconimg);

				bb->hide();

				X += bb->w() + ioffset;
				icons[i] = bb;
			}
		icon_group->end();

		/* see X as width of all icons */
		int gx = w()/2 - X/2;
		/* gx can be negative */
		gx = (gx > 10) ? gx : 10;
		icon_group->position(gx, Y);
	end();

	clear_border();

	/*
	 * If set_override() is used, message boxes will be
	 * popped behind splash. Using it or not ???
	 */
	set_override();

	// make sure window is centered
	int sw = DisplayWidth(fl_display, fl_screen);
	int sh = DisplayHeight(fl_display, fl_screen);
	position(sw/2 - w()/2, sh/2 - h()/2);

	show();
	Fl::add_timeout(TIMEOUT_START, runner_cb, this);

	// to keep splash at the top
#ifndef EDEWM_HAVE_NET_SPLASH
	global_splash = this;
	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), SubstructureNotifyMask);
	Fl::add_handler(splash_xmessage_handler);
#endif
	
	while(shown())
		Fl::wait();

#ifndef EDEWM_HAVE_NET_SPLASH
	Fl::remove_handler(splash_xmessage_handler);
#endif
}

/* called when splash option is on */
bool Splash::next_client(void) {
	if(slist->empty())
		return false;

	if(counter == 0)
		slist_it = slist->begin();

	if(slist_it == slist->end()) {
		counter = 0;
		return false;
	}


	E_ASSERT(counter < slist->size() && "Internal error; 'counter' out of bounds");

	char buff[1024];
	const char* msg = (*slist_it)->description.c_str();
	const char* cmd = (*slist_it)->exec.c_str();
	snprintf(buff, sizeof(buff), _("Starting %s..."), msg);

	icons[counter]->show();
	msgbox->copy_label(buff);
	redraw();

	/* run command */
	if(!dryrun)
		run_async(cmd);

	++slist_it;
	++counter;
	return true;
}

/* called when splash option is off */
bool Splash::next_client_nosplash(void) {
	if(slist->empty())
		return false;

	if(counter == 0)
		slist_it = slist->begin();

	if(slist_it == slist->end()) {
		counter = 0;
		return false;
	}

	E_ASSERT(counter < slist->size() && "Internal error; 'counter' out of bounds");

	char buff[1024];
	const char* msg = (*slist_it)->description.c_str();
	const char* cmd = (*slist_it)->exec.c_str();
	snprintf(buff, sizeof(buff), _("Starting %s..."), msg);

	printf("%s\n", buff);

	/* run command */
	if(!dryrun)
		run_async(cmd);

	++slist_it;
	++counter;
	return true;
}
