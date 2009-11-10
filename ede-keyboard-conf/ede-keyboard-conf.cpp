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
#include <edelib/Nls.h>
#include <edelib/Debug.h>
#include <edelib/String.h>
#include <edelib/Resource.h>

#define CONFIG_NAME "ede-keyboard"

EDELIB_NS_USING_AS(Window, AppWindow)
EDELIB_NS_USING(String)
EDELIB_NS_USING(Resource)

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

static void fetch_current_layout(String &ret, String &rules_file_ret) {
	char			 *tmp = NULL;
	XkbRF_VarDefsRec  vd;

	E_ASSERT(fl_display != NULL);

	/* get the layout straight from X since the layout can be changed during X session */
	if(!XkbRF_GetNamesProp(fl_display, &tmp, &vd) || !vd.layout)
		return;

	ret = vd.layout;

	/* remember rules filename too (on X.org is 'xorg')*/
	rules_file_ret = tmp;

	/* manually free each member */
	XFree(tmp);
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

			xkb_rules = XkbRF_Load(buf, "", True, True);
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

static void apply_layout_on_x(const String &layout, const String &rules_filename) {
	XkbRF_VarDefsRec vd;
	vd.layout  = (char*)layout.c_str();

	/* 
	 * rest fields must be zero-fied to prevent crash
	 * FIXME: will this change other fields???
	 */
	vd.model   = NULL;
	vd.options = NULL;
	vd.variant = NULL;

	if(XkbRF_SetNamesProp(fl_display, (char*)rules_filename.c_str(), &vd) != True)
		E_WARNING(E_STRLOC ": Failed to update XKB rules\n");
	else
		XSync(fl_display, False);
}

int main(int argc, char **argv) {
	String cl, rules_filename;

	/* needed for fetch_current_layout() and '--apply' */
	fl_open_display();
	dialog_canceled = false;

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

	/* read layouts */
	XkbRF_RulesPtr xkb_rules;

	fetch_current_layout(cl, rules_filename);
	xkb_rules = fetch_all_layouts(cl);

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
		apply_layout_on_x(n, rules_filename);
	}

	if(xkb_rules) 
		XkbRF_Free(xkb_rules, True);

	return 0;
}
