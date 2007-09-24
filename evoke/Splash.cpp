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

#include "Splash.h"
#include "Spawn.h"

#include <edelib/Run.h>
#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/Sound.h>

#include <FL/Fl_Shared_Image.h>
#include <FL/Fl.h>

#include <stdio.h> // snprintf

#define TIMEOUT_START    0.5  // timeout when splash is first time shown (also for first client)
#define TIMEOUT_CONTINUE 2.0  // timeout between starting rest of the cliens


extern void service_watcher_cb(int pid, int signum);
Fl_Double_Window* splash_win = 0;

class AutoSound {
	public:
		AutoSound()  { edelib::SoundSystem::init(); }
		~AutoSound() { if(edelib::SoundSystem::inited()) edelib::SoundSystem::shutdown(); }
		void play(const char* file) { if(edelib::SoundSystem::inited()) edelib::SoundSystem::play(file, 0); }
};

int splash_xmessage_handler(int e) {
	if(fl_xevent->type == MapNotify || fl_xevent->type == ConfigureNotify) {
		if(splash_win) {
			XRaiseWindow(fl_display, fl_xid(splash_win));
			return 1;
		}
	}

	return 0;
}

/*
 * repeatedly call runner() untill all clients are 
 * started then hide splash window
 */
void runner_cb(void* s) {
	Splash* sp = (Splash*)s;

	if(sp->next_client())
		Fl::repeat_timeout(TIMEOUT_CONTINUE, runner_cb, sp);
	else
		sp->hide();
}

Splash::Splash(bool sp, bool dr) : Fl_Double_Window(480, 364), clist(NULL), bkg(NULL), 
	counter(0), no_splash(sp), dry_run(dr)  {
	icons = NULL;
}

Splash::~Splash() {
	EVOKE_LOG("Cleaning splash data\n");
	// elements of icons cleans Fl_Group
	delete [] icons;
	Fl::remove_timeout(runner_cb);
}


#if 0
// after edewm got _NET_WM_WINDOW_TYPE_SPLASH support
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
	EASSERT(clist != NULL);

	if(no_splash) {
		while(next_client_nosplash()) 
			;
		return;
	}

	fl_register_images();

	// setup widgets
	begin();
		Fl_Box* bimg = new Fl_Box(0, 0, w(), h());
		Fl_Image* splash_img = 0;

		if(bkg)
			splash_img = Fl_Shared_Image::get(bkg->c_str());

		if(splash_img) {
			int W = splash_img->w();
			int H = splash_img->h();
			// update window and Box sizes
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
		// offset between icons
		int ioffset = 5;

		// FIXME: use malloc/something instead this
		icons = new Fl_Box*[clist->size()];

		icon_group->begin();
			const char* imgpath;
			Fl_Image* iconimg;
			int i = 0;
			for(ClientListIter it = clist->begin(); it != clist->end(); ++it, ++i) {
				Fl_Box* bb = new Fl_Box(X, Y, 64, 64);

				imgpath = (*it).icon.c_str();
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

		// see X as width of all icons
		int gx = w()/2 - X/2;
		// gx can be negative
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

#if 0
	bool sound_loaded = false;
	if(sound && !sound->empty())
		sound_loaded = edelib::SoundSystem::init();
#endif
	AutoSound sound_play;

	show();

	Fl::add_timeout(TIMEOUT_START, runner_cb, this);

	/*
	 * Force keeping splash window at the top of all. To do this, we will listen
	 * MappingNotify and ConfigureNotify events, and when they be triggered we will
	 * raise splash window.
	 */
	splash_win = this;

	//XSelectInput(fl_display, RootWindow(fl_display, fl_screen), SubstructureNotifyMask);
	//Fl::add_handler(splash_xmessage_handler);
#if 0
	if(sound_loaded)
		edelib::SoundSystem::play(sound->c_str(), 0);
#endif
	sound_play.play(sound->c_str());

	while(shown())
		Fl::wait();
#if 0
	if(sound_loaded) {
		edelib::SoundSystem::stop();
		edelib::SoundSystem::shutdown();
	}
#endif

	splash_win = 0;
}

// called when splash option is on
bool Splash::next_client(void) {
	if(clist->empty())
		return false;

	if(counter == 0)
		clist_it = clist->begin();

	if(clist_it == clist->end()) {
		counter = 0;
		return false;
	}


	EASSERT(counter < clist->size() && "Internal error; 'counter' out of bounds");

	char buff[1024];
	const char* msg = (*clist_it).desc.c_str();
	const char* cmd = (*clist_it).exec.c_str();
	snprintf(buff, sizeof(buff), _("Starting %s..."), msg);

	icons[counter]->show();
	msgbox->copy_label(buff);
	redraw();

	if(!dry_run)
		spawn_program(cmd, service_watcher_cb);
		//spawn_program(cmd);

	++clist_it;
	++counter;
	return true;
}

// called when splash option is off
bool Splash::next_client_nosplash(void) {
	if(clist->empty())
		return false;

	if(counter == 0)
		clist_it = clist->begin();

	if(clist_it == clist->end()) {
		counter = 0;
		return false;
	}

	EASSERT(counter < clist->size() && "Internal error; 'counter' out of bounds");

	char buff[1024];
	const char* msg = (*clist_it).desc.c_str();
	const char* cmd = (*clist_it).exec.c_str();
	snprintf(buff, sizeof(buff), _("Starting %s..."), msg);

	printf("%s\n", buff);

	if(!dry_run)
		spawn_program(cmd, service_watcher_cb);
		//spawn_program(cmd);

	++clist_it;
	++counter;
	return true;
}
