#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_File_Chooser.H>
#include <edelib/Window.h>
#include <edelib/Nls.h>
#include <edelib/Ede.h>
#include <edelib/Resource.h>
#include <edelib/Debug.h>
#include <edelib/WindowUtils.h>

#define KNOWN_APP_PREDEFINED 1
#include "PredefApps.h"
#include "AppChoice.h"

#define EMPTY_STR(s) (s[0] == '\0' || (strlen(s) == 0))

EDELIB_NS_USING_AS(Window, EdeWindow)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(window_center_on_screen)

AppChoice *browser_choice,
		  *mail_choice,
		  *filemgr_choice,
		  *term_choice;

/* store separate item */
static void check_and_store(AppChoice *c, const char *n, Resource &rc) {
	const char *val = c->selected();
	if(val) rc.set("Preferred", n, val);
}

/* load separate item */
static void check_and_load(AppChoice *c, const char *n, Resource &rc) {
	static char buf[128];
	if(rc.get("Preferred", n, buf, sizeof(buf)) && !EMPTY_STR(buf)) {
		c->add_if_user_program(buf);
		c->select_by_cmd(buf);
	}
}

static void load_settings(void) {
	Resource rc;
	if(!rc.load("ede-launch")) return;

	check_and_load(browser_choice, "browser", rc);
	check_and_load(mail_choice, "mail", rc);
	check_and_load(filemgr_choice, "file_manager", rc);
	check_and_load(term_choice, "terminal", rc);
}

static void close_cb(Fl_Widget *widget, void *ww) {
	EdeWindow *win = (EdeWindow*)ww;
	win->hide();
}

static void ok_cb(Fl_Widget *widget, void *ww) {
	Resource  rc;
	/* 
	 * if Resource does not have any items, it will refuse to save anything, keeping
	 * previous content of conf file. This happens if all items are set to be None
	 */
	rc.set("Preferred", "dummy", 0);

	check_and_store(browser_choice, "browser", rc);
	check_and_store(mail_choice, "mail", rc);
	check_and_store(filemgr_choice, "file_manager", rc);
	check_and_store(term_choice, "terminal", rc);

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
			browser_choice = new AppChoice(20, 65, 320, 25, _("Web browser"));
			browser_choice->add_programs(app_browsers);
			browser_choice->value(0);

			mail_choice = new AppChoice(20, 120, 320, 25, _("Mail reader"));
			mail_choice->add_programs(app_mails);
			mail_choice->value(0);
		gint->end();

		Fl_Group *gutil = new Fl_Group(15, 30, 335, 140, _("Utilities"));
		gutil->hide();
		gutil->begin();
			filemgr_choice = new AppChoice(20, 65, 320, 25, _("File manager"));
			filemgr_choice->add_programs(app_filemanagers);
			filemgr_choice->value(0);

			term_choice = new AppChoice(20, 120, 320, 25, _("Terminal"));
			term_choice->add_programs(app_terminals);
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
	window_center_on_screen(win);
	win->show(argc, argv);

	return Fl::run();
}
