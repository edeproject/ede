#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>

#include <edelib/Nls.h>
#include <edelib/Debug.h>

#include "XScreenSaver.h"
#include "icons/energy.xpm"

static Fl_Pixmap image_energy(energy_star_xpm);

static Fl_Spinner* standby_val;
static Fl_Spinner* suspend_val;
static Fl_Spinner* off_val;

static void dpms_enable_cb(Fl_Widget* w, void*) {
	Fl_Check_Button* o = (Fl_Check_Button*)w;
	if(o->value()) {
		standby_val->activate();
		suspend_val->activate();
		off_val->activate();
	} else {
		standby_val->deactivate();
		suspend_val->deactivate();
		off_val->deactivate();
	}
}

static void close_cb(Fl_Widget*, void* w) {
	Fl_Double_Window* win = (Fl_Double_Window*)w;
	win->hide();
}

int main(int argc, char **argv) {
	SaverPrefs* sp = xscreensaver_read_config();
	E_ASSERT(sp != NULL);

	Fl_Double_Window* win = new Fl_Double_Window(340, 445, _("Screensaver options"));
	win->begin();
		/* monitor */
		Fl_Box* b1 = new Fl_Box(120, 163, 100, 15);
		b1->box(FL_BORDER_BOX);
		Fl_Box* b2 = new Fl_Box(65, 10, 210, 158);
		b2->box(FL_THIN_UP_BOX);
		/* box size is intentionaly odd so preserve aspect ratio */
		Fl_Box* b3 = new Fl_Box(78, 20, 184, 138);
		b3->box(FL_DOWN_BOX);
		b3->color(FL_BLACK);
		Fl_Box* b4 = new Fl_Box(95, 173, 146, 14);
		b4->box(FL_THIN_UP_BOX);
		
		Fl_Group* g1 = new Fl_Group(10, 215, 320, 45, _("Screensaver"));
		g1->box(FL_ENGRAVED_BOX);
		g1->align(FL_ALIGN_TOP_LEFT);
		g1->begin();
			Fl_Choice* saver_list = new Fl_Choice(19, 225, 180, 25);
			saver_list->down_box(FL_BORDER_BOX);

			HackListIter it = sp->hacks.begin(), it_end = sp->hacks.end();
			for(; it != it_end; ++it) {
				saver_list->add((*it)->name.c_str(), 0, 0);
				delete *it;
			}

			Fl_Spinner* timeout = new Fl_Spinner(275, 226, 45, 25, _("Timeout:"));
			timeout->tooltip(_("Idle time in minutes after screensaver is started"));
			timeout->value(sp->timeout);
		g1->end();

		Fl_Group* g2 = new Fl_Group(10, 290, 320, 110, _("DPMS"));
		g2->box(FL_ENGRAVED_BOX);
		g2->align(FL_ALIGN_TOP_LEFT);
		g2->begin();
			Fl_Check_Button* denabled = new Fl_Check_Button(20, 299, 180, 26, _("Enabled"));
			denabled->down_box(FL_DOWN_BOX);
			denabled->tooltip(_("Enable or disable Display Power Management Signaling support"));
			denabled->callback((Fl_Callback*)dpms_enable_cb);
			denabled->value(sp->dpms_enabled);

			Fl_Box* energy_image = new Fl_Box(20, 341, 75, 49);
			energy_image->image(image_energy);

			standby_val = new Fl_Spinner(275, 301, 45, 24, _("Standby:"));
			standby_val->tooltip(_("Minutes for standby"));
			standby_val->value(sp->dpms_standby);

			suspend_val = new Fl_Spinner(275, 331, 45, 24, _("Suspend:"));
			suspend_val->tooltip(_("Minutes for suspend"));
			suspend_val->value(sp->dpms_suspend);

			off_val = new Fl_Spinner(275, 360, 45, 24, _("Off:"));
			off_val->tooltip(_("Minutes to turn off the screen"));
			off_val->value(sp->dpms_off);

			/* execute callback to apply changes before window is shown */
			denabled->do_callback();
		g2->end();

		Fl_Button* ok_button = new Fl_Button(145, 410, 90, 25, _("&OK"));

		Fl_Button* close_button = new Fl_Button(240, 410, 90, 25, _("&Cancel"));
		close_button->callback(close_cb, win);
	win->end();
	delete sp;

	win->show(argc, argv);
	return Fl::run();
}
