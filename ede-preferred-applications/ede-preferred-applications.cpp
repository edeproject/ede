#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tabs.H>
#include <edelib/Window.h>
#include <edelib/Nls.h>
#include <edelib/Ede.h>

#include "PredefApps.h"

EDELIB_NS_USING_AS(Window, EdeWindow)

static void close_cb(Fl_Widget *widget, void *ww) {
	EdeWindow *win = (EdeWindow*)ww;
	win->hide();
}

static void ok_cb(Fl_Widget *widget, void *ww) {
	close_cb(widget, ww);
}

int main(int argc, char** argv) {
	EDE_APPLICATION("ede-preferred-applications");

	EdeWindow *win = new EdeWindow(365, 220, _("Preferred applications"));
	Fl_Tabs *tabs = new Fl_Tabs(10, 10, 345, 165);
	tabs->begin();
		Fl_Group *gint = new Fl_Group(15, 30, 335, 140, _("Internet"));
		gint->begin();
			Fl_Choice *cint = new Fl_Choice(20, 65, 320, 25, _("Web browser"));
			cint->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_browsers, cint);
			cint->value(0);

			Fl_Choice *cmail = new Fl_Choice(20, 120, 320, 25, _("Mail reader"));
			cmail->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_mails, cmail);
			cmail->value(0);
		gint->end();

		Fl_Group *gutil = new Fl_Group(15, 30, 335, 140, _("Utilities"));
		gutil->hide();
		gutil->begin();
			Fl_Choice *ufile = new Fl_Choice(20, 65, 320, 25, _("File manager"));
			ufile->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_filemanagers, ufile);
			ufile->value(0);

			Fl_Choice *uterm = new Fl_Choice(20, 120, 320, 25, _("Terminal"));
			uterm->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_terminals, uterm);
			uterm->value(0);
		gutil->end();
	tabs->end();

	Fl_Button *ok = new Fl_Button(170, 185, 90, 25, _("&OK"));
	Fl_Button *cancel = new Fl_Button(265, 185, 90, 25, _("&Cancel"));
	ok->callback(ok_cb, win);
	cancel->callback(close_cb, win);

	win->set_modal();
	win->end();
	win->show(argc, argv);

	return Fl::run();
}
