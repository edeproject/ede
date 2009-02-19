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

#ifndef __SPLASH_H__
#define __SPLASH_H__

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>

#include "EvokeService.h"

class Splash : public Fl_Double_Window {
	private:
		StartupItemList*     slist;
		StartupItemListIter  slist_it;
		edelib::String*      splash_theme;      

		unsigned int counter;
		bool         show_splash;
		bool         dryrun;

		Fl_Box*      msgbox;
		Fl_Box**     icons;

	public:
		Splash(StartupItemList& s, edelib::String& theme, bool show_it, bool dr);
		~Splash();

		bool next_client(void);
		bool next_client_nosplash(void);

		void run(void);

#if EDEWM_HAVE_NET_SPLASH
		virtual void show(void);
#endif
};

#endif
