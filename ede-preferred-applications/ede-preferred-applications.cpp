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
#include <edelib/Resource.h>

#include "PredefApps.h"

EDELIB_NS_USING_AS(Window, EdeWindow)
EDELIB_NS_USING(Resource)

Fl_Choice *browser_choice,
		  *mail_choice,
		  *filemgr_choice,
		  *term_choice;

/* store separate item */
static void check_and_store(Fl_Choice *c, KnownApp *lst, const char *n, Resource &rc) {
	int       sel = c->value();
	KnownApp *app = app_get(lst, sel);
	if(app_is_magic_cmd(*app))
		return;

	rc.set("Preferred", n, app->cmd);
}

/* load separate item */
static void check_and_load(Fl_Choice *c, KnownApp *lst, const char *n, Resource &rc) {
	static char buf[128];
	if(!rc.get("Preferred", n, buf, sizeof(buf)))
		return;

	KnownApp *app = app_find_by_cmd(lst, buf);
	if(!app) return;

	int i = app_get_index(lst, *app);
	if(i < 0) return;

	c->value(i);
}

static void load_settings(void) {
	Resource rc;
	if(!rc.load("ede-launch")) return;

	check_and_load(browser_choice, app_browsers, "browser", rc);
	check_and_load(mail_choice, app_mails, "mail", rc);
	check_and_load(filemgr_choice, app_filemanagers, "file_manager", rc);
	check_and_load(term_choice, app_terminals, "terminal", rc);
}

static void close_cb(Fl_Widget *widget, void *ww) {
	EdeWindow *win = (EdeWindow*)ww;
	win->hide();
}

static void ok_cb(Fl_Widget *widget, void *ww) {
	Resource  rc;

	check_and_store(browser_choice, app_browsers, "browser", rc);
	check_and_store(mail_choice, app_mails, "mail", rc);
	check_and_store(filemgr_choice, app_filemanagers, "file_manager", rc);
	check_and_store(term_choice, app_terminals, "terminal", rc);

	rc.save("ede-launch");
	close_cb(widget, ww);
}

int main(int argc, char** argv) {
	EDE_APPLICATION("ede-preferred-applications");

	EdeWindow *win = new EdeWindow(365, 220, _("Preferred applications"));
	Fl_Tabs *tabs = new Fl_Tabs(10, 10, 345, 165);
	tabs->begin();
		Fl_Group *gint = new Fl_Group(15, 30, 335, 140, _("Internet"));
		gint->begin();
			browser_choice = new Fl_Choice(20, 65, 320, 25, _("Web browser"));
			browser_choice->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_browsers, browser_choice);
			browser_choice->value(0);

			mail_choice = new Fl_Choice(20, 120, 320, 25, _("Mail reader"));
			mail_choice->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_mails, mail_choice);
			mail_choice->value(0);
		gint->end();

		Fl_Group *gutil = new Fl_Group(15, 30, 335, 140, _("Utilities"));
		gutil->hide();
		gutil->begin();
			filemgr_choice = new Fl_Choice(20, 65, 320, 25, _("File manager"));
			filemgr_choice->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_filemanagers, filemgr_choice);
			filemgr_choice->value(0);

			term_choice = new Fl_Choice(20, 120, 320, 25, _("Terminal"));
			term_choice->align(FL_ALIGN_TOP_LEFT);
			app_populate_menu(app_terminals, term_choice);
			term_choice->value(0);
		gutil->end();
	tabs->end();

	Fl_Button *ok = new Fl_Button(170, 185, 90, 25, _("&OK"));
	Fl_Button *cancel = new Fl_Button(265, 185, 90, 25, _("&Cancel"));
	ok->callback(ok_cb, win);
	cancel->callback(close_cb, win);

	win->set_modal();
	win->end();

	/* now load all settings before show */
	load_settings();
	win->show(argc, argv);

	return Fl::run();
}
