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

#ifndef __SPLASH_H__
#define __SPLASH_H__

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Box.h>

#include "EvokeService.h"

class Splash : public Fl_Double_Window {
	private:
		ClientList* clist;
		const edelib::String* bkg;
		const edelib::String* sound;
		unsigned int counter;
		bool no_splash;
		bool dry_run;

		ClientListIter clist_it;

		Fl_Box*  msgbox;
		Fl_Box** icons;

	public:
		Splash(bool sp, bool dr);
		~Splash();

		/*
		 * NOTE: clients() and set_background() uses address of passed data,
		 * so make sure passed data does not destroys.
		 */
		void set_clients(ClientList* cl) { clist = cl; }
		void set_background(const edelib::String* s) { bkg = s; }
		void set_sound(const edelib::String* s) { sound = s; }

		const ClientList* get_clients(void) const { return clist; }
		bool next_client(void);
		bool next_client_nosplash(void);

		void run(void);
		//virtual void show(void);
};

#endif
