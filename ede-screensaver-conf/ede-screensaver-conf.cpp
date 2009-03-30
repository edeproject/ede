/*
 * $Id$
 *
 * ede-screensaver-conf, a tool to configure screensaver
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 Sanel Zukan <karijes@equinox-project.org>
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or later.
 * See COPYING for the details.
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>
#include <FL/x.H>

#include <edelib/Nls.h>
#include <edelib/Debug.h>

#include "XScreenSaver.h"
#include "icons/energy.xpm"

static Fl_Pixmap image_energy((const char**)energy_star_xpm);

static Fl_Spinner*       standby_val;
static Fl_Spinner*       suspend_val;
static Fl_Spinner*       off_val;
static Fl_Spinner*       timeout_val;
static Fl_Double_Window* main_win;
static Fl_Double_Window* preview_win;

static void dpms_enable_cb(Fl_Widget* w, void* s) {
	Fl_Check_Button* o = (Fl_Check_Button*)w;
	SaverPrefs* sp = (SaverPrefs*)s;

	if(o->value()) {
		standby_val->activate();
		suspend_val->activate();
		off_val->activate();
	} else {
		standby_val->deactivate();
		suspend_val->deactivate();
		off_val->deactivate();
	}

	sp->dpms_enabled = o->value();
}

static void choice_cb(Fl_Widget* w, void* s) {
	Fl_Choice* c = (Fl_Choice*)w;
	SaverPrefs* sp = (SaverPrefs*)s;

	const char* saver_name = c->text();
	if(!saver_name)
		return;

	/* when we want to disable it */
	if(strcmp(saver_name, "(None)") == 0) {
		xscreensaver_kill_preview();
		/* just draw an empty box */
		Fl_Box* b = new Fl_Box(0, 0, 180, 134);
		preview_win->add(b);
		preview_win->redraw();

		sp->mode = SAVER_OFF;
		return;
	}

	/* FIXME: for now only one is allowed */
	sp->mode = SAVER_ONE;

	/* find the name matches in our list and run it's command */
	HackListIter it = sp->hacks.begin(), it_end = sp->hacks.end();
	for(; it != it_end; ++it) {
		if((*it)->name == saver_name) {
			xscreensaver_preview(fl_xid(preview_win), (*it)->exec.c_str());
			sp->curr_hack = (*it)->sindex;
			break;
		}
	}
}

static void close_cb(Fl_Widget*, void*) {
	xscreensaver_kill_preview();
	preview_win->hide();
	main_win->hide();
}

static void ok_cb(Fl_Widget*, void* s) {
	SaverPrefs* sp = (SaverPrefs*)s;
	if(sp) {
		sp->timeout = (int)timeout_val->value();
		sp->dpms_standby = (int)standby_val->value();
		sp->dpms_suspend = (int)suspend_val->value();
		sp->dpms_off = (int)off_val->value();

		xscreensaver_save_config(sp);
	}
	close_cb(0, 0);
}

int main(int argc, char **argv) {
	fl_open_display();
	/* start daemon if not started */
	xscreensaver_run_daemon(fl_display);

	SaverPrefs* sp = xscreensaver_read_config();

	main_win = new Fl_Double_Window(340, 445, _("Screensaver options"));
	/* so ESC does not interrupt running process */
	main_win->callback(close_cb);

	main_win->begin();
		/* monitor */
		Fl_Box* b1 = new Fl_Box(120, 163, 100, 15);
		b1->box(FL_BORDER_BOX);
		Fl_Box* b2 = new Fl_Box(65, 10, 210, 158);
		b2->box(FL_THIN_UP_BOX);

		Fl_Box* b3 = new Fl_Box(78, 20, 184, 138);
		b3->color(FL_BLACK);
		b3->box(FL_DOWN_BOX);

		/* preview window in 'b3' box */
		preview_win = new Fl_Double_Window(80, 22, 180, 134);
		preview_win->color(FL_BLACK);
		preview_win->begin();

		/* if failed to load any screensaver */
		if(!sp) {
			Fl_Box* b4 = new Fl_Box(0, 0, 180, 134, _("No screensavers found"));
			b4->labelcolor(FL_GRAY);
			b4->align(FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
		}

		preview_win->end();

		Fl_Box* b4 = new Fl_Box(95, 173, 146, 14);
		b4->box(FL_THIN_UP_BOX);
		
		Fl_Group* g1 = new Fl_Group(10, 215, 320, 45, _("Screensaver"));
		g1->box(FL_ENGRAVED_BOX);
		g1->align(FL_ALIGN_TOP_LEFT);
		g1->begin();
			Fl_Choice* saver_list = new Fl_Choice(19, 225, 180, 25);
			saver_list->down_box(FL_BORDER_BOX);
			saver_list->add("(None)", 0, 0);

			if(sp) {
				int sel = 0;
				saver_list->callback((Fl_Callback*)choice_cb, sp);

				/* fix possible error */
				if(sp->curr_hack >= sp->hacks.size())
					sp->curr_hack = 0;

				HackListIter it = sp->hacks.begin(), it_end = sp->hacks.end();
				for(int i = 1; it != it_end; ++it, i++) {
					saver_list->add((*it)->name.c_str(), 0, 0);

					/*
					 * Check real hack index number against current one and let it match 
					 * position in our Fl_Choice list. Note that first item is '(None)'
					 * so 'i' starts from 1
					 */
					if(sp->mode != SAVER_OFF && (*it)->sindex == sp->curr_hack)
						sel = i;
				}

				saver_list->value(sel);
			}

			timeout_val = new Fl_Spinner(275, 226, 45, 25, _("Timeout:"));
			timeout_val->tooltip(_("Idle time in minutes after screensaver is started"));
			timeout_val->range(1, 500);
			if(sp)
				timeout_val->value(sp->timeout);
			else
				timeout_val->value(1);

		g1->end();

		Fl_Group* g2 = new Fl_Group(10, 290, 320, 110, _("Power management"));
		g2->box(FL_ENGRAVED_BOX);
		g2->align(FL_ALIGN_TOP_LEFT);
		g2->begin();
			Fl_Check_Button* denabled = new Fl_Check_Button(20, 299, 180, 26, _("Enabled"));
			denabled->down_box(FL_DOWN_BOX);
			denabled->tooltip(_("Enable or disable Display Power Management Signaling support"));
			denabled->callback((Fl_Callback*)dpms_enable_cb, sp);
			if(sp)
				denabled->value(sp->dpms_enabled);
			else
				denabled->value(1);

			Fl_Box* energy_image = new Fl_Box(20, 341, 75, 49);
			energy_image->image(image_energy);

			standby_val = new Fl_Spinner(275, 301, 45, 24, _("Standby:"));
			standby_val->tooltip(_("Minutes for standby"));
			standby_val->range(1, 500);
			if(sp)
				standby_val->value(sp->dpms_standby);
			else
				standby_val->value(1);

			suspend_val = new Fl_Spinner(275, 331, 45, 24, _("Suspend:"));
			suspend_val->tooltip(_("Minutes for suspend"));
			suspend_val->range(1, 500);
			if(sp)
				suspend_val->value(sp->dpms_suspend);
			else
				suspend_val->value(1);

			off_val = new Fl_Spinner(275, 360, 45, 24, _("Off:"));
			off_val->tooltip(_("Minutes to turn off the screen"));
			off_val->range(1, 500);
			if(sp)
				off_val->value(sp->dpms_off);
			else
				off_val->value(1);

			/* execute callback to apply changes before main_window is shown */
			denabled->do_callback();
		g2->end();

		Fl_Button* ok_button = new Fl_Button(145, 410, 90, 25, _("&OK"));
		ok_button->callback(ok_cb, sp);

		Fl_Button* close_button = new Fl_Button(240, 410, 90, 25, _("&Cancel"));
		close_button->callback(close_cb);
	main_win->end();

	main_win->show(argc, argv);
	/* run preview immediately */
	saver_list->do_callback();
	int ret = Fl::run();

	if(sp) {
		HackListIter it = sp->hacks.begin(), it_end = sp->hacks.end();
		for(; it != it_end; ++it)
			delete *it;
		delete sp;
	}

	return ret;
}
