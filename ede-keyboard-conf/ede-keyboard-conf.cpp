/*
 * $Id$
 *
 * ede-keyboard-conf, a tool to configure keyboard
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <edelib/MessageBox.h>
#include <edelib/Nls.h>
EDELIB_NS_USING(alert)

#ifdef HAVE_XKBRULES
#include <stdio.h>
#include <string.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h> 

#include <edelib/Window.h>
#include <edelib/Debug.h>
#include <edelib/String.h>
#include <edelib/Resource.h>
#include <edelib/File.h>
#include <edelib/Run.h>
#include <edelib/MessageBox.h>
#include <edelib/ForeignCallback.h>

#define CONFIG_NAME      "ede-keyboard"
#define PANEL_APPLET_ID  "ede-keyboard"
#define DEFAULT_X_LAYOUT "us"

EDELIB_NS_USING_AS(Window, AppWindow)
EDELIB_NS_USING(String)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(file_path)
EDELIB_NS_USING(run_sync)
EDELIB_NS_USING(foreign_callback_call)

static AppWindow       *win;
static Fl_Hold_Browser *layout_browser;
static Fl_Check_Button *repeat_press;
static Fl_Check_Button *show_flag;
static bool				dialog_canceled;

static const char *x11_dirs[] = {
	"/etc/X11/",
	"/usr/share/X11/",
	"/usr/local/share/X11/",
	"/usr/X11R6/lib/X11/",
	"/usr/X11R6/lib64/X11/",
	"/usr/local/X11R6/lib/X11/",
	"/usr/local/X11R6/lib64/X11/",
	"/usr/lib/X11/",
	"/usr/lib64/X11/",
	"/usr/local/lib/X11/",
	"/usr/local/lib64/X11/",
	"/usr/pkg/share/X11/",
	"/usr/pkg/xorg/lib/X11/",
	0
};

static const char *x11_rules[] = {
	"xkb/rules/xorg",
	"xkb/rules/xfree86",
	0
}; 

static void cancel_cb(Fl_Widget*, void*) {
	win->hide();
	dialog_canceled = true;
}

static void ok_cb(Fl_Widget*, void*) {
	win->hide();
	dialog_canceled = false;
}

static void fetch_current_layout(String &current) {
	char             *rules_file;
	XkbRF_VarDefsRec vd;

	/* get the layout straight from X since the layout can be changed during X session */
	if(!XkbRF_GetNamesProp(fl_display, &rules_file, &vd)) {
		/* use some values */
		current = DEFAULT_X_LAYOUT;
	} else {
		current = vd.layout;
	}

	/* free everything */
	XFree(rules_file);
	XFree(vd.layout);
	XFree(vd.model);
	XFree(vd.options);
	XFree(vd.variant);
}

static XkbRF_RulesPtr fetch_all_layouts(const String &current) {
	char		   buf[256];
	XkbRF_RulesPtr xkb_rules = NULL;

	/* try to locate rules file */
	for(int i = 0; x11_dirs[i]; i++) {
		for(int j = 0; x11_rules[j]; j++) {
			snprintf(buf, sizeof(buf), "%s%s", x11_dirs[i], x11_rules[j]);

			xkb_rules = XkbRF_Load(buf, (char*)"", True, True);
			if(xkb_rules) 
				goto done;
		}
	}

	if(!xkb_rules) {
		E_WARNING(E_STRLOC ": Unable to load keyboard rules file\n");
		return NULL;
	}

done:
	for(int i = 0; i < xkb_rules->layouts.num_desc; i++) {
		snprintf(buf, sizeof(buf), "%s\t%s", xkb_rules->layouts.desc[i].name, xkb_rules->layouts.desc[i].desc);
		layout_browser->add(buf);

		if(current == xkb_rules->layouts.desc[i].name) {
			/* Fl_Browser counts items from 1 */
			layout_browser->select(i + 1);
		}
	}

	return xkb_rules;
}

static void read_config(void) {
	Resource r;
	if(!r.load(CONFIG_NAME))
		return;

	/* 
	 * NOTE: keyboard layout is not read from config file to set applet flag/name;
	 * this is done using X call. Keyboard layout is only read when '--apply' was given
	 * to this program, so it can be set when EDE is started
	 */
	bool state;
	r.get("Keyboard", "show_country_flag", state, true);
	show_flag->value((int)state);

	r.get("Keyboard", "repeat_key_press", state, true);
	repeat_press->value((int)state);
}

static void save_config(const String &layout) {
	Resource r;

	r.set("Keyboard", "show_country_flag", (bool)show_flag->value());
	r.set("Keyboard", "repeat_key_press", (bool)repeat_press->value());
	r.set("Keyboard", "layout", layout.c_str());

	r.save(CONFIG_NAME);
}

static void apply_changes_on_x(const char *current, const String* previous) {
	if(repeat_press->value())
		XAutoRepeatOn(fl_display);
	else
		XAutoRepeatOff(fl_display);

	/* do not do anything if selected layout is the same as previous */
	if(!previous || *previous != current) {
		/* 
		 * believe me, it is easier to call this command than to reimplmement a mess for 
		 * uploading keyboard layout on X server! 
		 */
		String setxkbmap = file_path("setxkbmap");

		if(setxkbmap.empty()) {
			alert(_("Unable to find 'setxkbmap' tool.\n\nThis tool is used as helper tool for "
					"easier keyboard setup and is standard tool shipped with every X package. "
					"Please install it first and run this program again."));
		} else {
			int ret = run_sync("%s %s", setxkbmap.c_str(), current);
			/* do not show dialog since we can fail if config has bad entry when called from apply_chages_from_config() */
			if(ret != 0) 
				E_WARNING(E_STRLOC ": 'setxkbmap %s' failed with %i\n", current, ret);
		}
	}

	/* force panel applet to re-read config file to see if flag should be displayed or not */
	foreign_callback_call(PANEL_APPLET_ID);
}

static void apply_chages_from_config(void) {
	Resource r;
	if(!r.load(CONFIG_NAME))
		return;

	char buf[32];
	if(!r.get("Keyboard", "layout", buf, sizeof(buf)))
		return;

	/* TODO: this should be validated somehow */
	apply_changes_on_x(buf, NULL);
}
#endif /* HAVE_XKBRULES */

int main(int argc, char **argv) {
#ifdef HAVE_XKBRULES
	/* must be opened first */
	fl_open_display();

	/* only apply what was written in config */
	if(argc > 1 && strcmp(argv[1], "--apply") == 0) {
		apply_chages_from_config();
		return 0;
	}

	String cl;
	dialog_canceled = false;

	/* get layout X holds */
	fetch_current_layout(cl);

	/* construct GUI */
	win = new AppWindow(340, 320, _("Keyboard configuration tool"));
	win->begin();
		Fl_Tabs *tabs = new Fl_Tabs(10, 10, 320, 265);
		tabs->begin();
			Fl_Group *layout_group = new Fl_Group(15, 30, 310, 240, _("Layout"));
			layout_group->begin();
				layout_browser = new Fl_Hold_Browser(20, 40, 300, 190);

				/* so things can be nicely aligned per column */
				int cwidths [] = {100, 0};
				layout_browser->column_widths(cwidths);
				layout_browser->column_char('\t');

				layout_group->resizable(layout_browser);

				show_flag = new Fl_Check_Button(20, 240, 300, 25, _("Show country flag"));
				show_flag->down_box(FL_DOWN_BOX);
				show_flag->value(1);
			layout_group->end();
			Fl_Group::current()->resizable(layout_group);

			Fl_Group *details_group = new Fl_Group(15, 30, 310, 240, _("Details"));
			details_group->hide();
			details_group->begin();
				repeat_press = new Fl_Check_Button(20, 45, 300, 25, _("Repeat pressed key"));
				repeat_press->down_box(FL_DOWN_BOX);
				repeat_press->value(1);
			details_group->end();
		tabs->end();

		/* resizable box */
		Fl_Box *rbox = new Fl_Box(30, 163, 65, 52);
		Fl_Group::current()->resizable(rbox);

		Fl_Button *ok_button = new Fl_Button(145, 285, 90, 25, _("&OK"));
		ok_button->callback(ok_cb);

		Fl_Button *cancel_button = new Fl_Button(240, 285, 90, 25, _("&Cancel"));
		cancel_button->callback(cancel_cb);
	win->end();

	/* read all XKB layouts */
	XkbRF_RulesPtr xkb_rules = fetch_all_layouts(cl);

	/* read configuration */
	read_config();

	win->show(argc, argv);
	Fl::run();

	/* do not save configuration if was canceled */
	if(!dialog_canceled && xkb_rules && layout_browser->value()) {
		int  i = layout_browser->value();
		/* get the layout that matches browser row */
		char *n  = xkb_rules->layouts.desc[i - 1].name;

		save_config(n);
		apply_changes_on_x(n, &cl);
	}

	if(xkb_rules) 
		XkbRF_Free(xkb_rules, True);

#else
	alert(_("ede-keyboard-conf was compiled without XKB extension. Please recompile it again"));
#endif /* HAVE_XKBRULES */

	return 0;
}
