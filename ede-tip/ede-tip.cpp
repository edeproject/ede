/*
 * $Id: etip.cpp 1664 2006-06-14 00:21:44Z karijes $
 *
 * ede-tip, show some tips!
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for the details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pixmap.H>

#include <edelib/Resource.h>
#include <edelib/DesktopFile.h>
#include <edelib/File.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/MessageBox.h>
#include <edelib/Directory.h>
#include <edelib/WindowUtils.h>
#include <edelib/Ede.h>

#include "icons/hint.xpm"
#include "Fortune.h"

EDELIB_NS_USING(String)
EDELIB_NS_USING(DesktopFile)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(dir_create_with_parents)
EDELIB_NS_USING(user_config_dir)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(window_center_on_screen)
EDELIB_NS_USING(DESK_FILE_TYPE_APPLICATION)

static Fl_Pixmap image_hint((const char**)hint_xpm);

Fl_Window* win;
Fl_Check_Button* check_button;
Fl_Box* title_box;
Fl_Box* tip_box;
int curr_tip = 0;

FortuneFile* ffile = 0;
String fstring;

#define TITLE_TIPS_NUM 3
const char* title_tips[TITLE_TIPS_NUM] = {
	_("Did you know?"),
	_("Tip of the Day"),
	_("Always good to know"),
};

const char* random_title(const char** lst, unsigned int max) {
	unsigned int val = rand() % max;
	return lst[val];
}

const char* random_fortune(void) {
	unsigned int val = rand() % (fortune_num_items(ffile) - 1);
	curr_tip = val;
	fortune_get(ffile, val, fstring);
	return fstring.c_str();
}

FortuneFile* load_fortune_file(void) {
	String path = Resource::find_data("tips");
	if(path.empty())
		return NULL;

	path += "/ede";
	String dat = path;
	dat += ".dat";

	return fortune_open(path.c_str(), dat.c_str());
}

/* 
 * Save/create ede-tip.desktop inside autostart directory.
 * Autostart resides in ~/.config/autostart and directory does not
 * exists, it will be created (only /autostart/, not full path).
 *
 * Saving/creating inside /etc/xdg/autostart is not done since 
 * ~/.config/autostart is prefered (see autostart specs) and all 
 * application will be looked there first.
 */
void write_autostart_stuff(void) {
	String path = user_config_dir();
	path += "/autostart";
	dir_create_with_parents(path.c_str());

	// now see if ede-tip.desktop exists, and update it if do
	path += "/ede-tip.desktop";
	DesktopFile conf;

	bool hidden = check_button->value();

	if(!conf.load(path.c_str()))
		conf.create_new(DESK_FILE_TYPE_APPLICATION);

	// always write these values so someone does not try to play with us
	conf.set_hidden(!hidden);
	conf.set_name("EDE Tips");
	conf.set_exec("ede-tip");

	if(conf.save(path.c_str()) == false)
		alert(_("I'm not able to save %s. Probably I don't have enough permissions to do that ?"), path.c_str());
}

void read_autostart_stuff(void) {
	String path = user_config_dir();
	path += "/autostart/ede-tip.desktop";

	DesktopFile conf;
	if(!conf.load(path.c_str())) {
		check_button->value(0);
		return;
	}

	if(conf.hidden())
		check_button->value(0);
	else
		check_button->value(1);
}

void close_cb(Fl_Widget*, void*) {
	win->hide();
	write_autostart_stuff();
}

void next_cb(Fl_Widget*, void*) {
	if(!ffile)
		return;

	curr_tip++;
	if(curr_tip >= (int)fortune_num_items(ffile))
		curr_tip = 0;

	fortune_get(ffile, curr_tip, fstring);
	tip_box->label(fstring.c_str());
}

void prev_cb(Fl_Widget*, void*) {
	if(!ffile)
		return;

	curr_tip--;
	if(curr_tip < 0) {
		curr_tip = fortune_num_items(ffile);
		curr_tip--;
	}

	fortune_get(ffile, curr_tip, fstring);
	tip_box->label(fstring.c_str());
}

int main(int argc, char **argv) {
	EDE_APPLICATION("ede-tip");

	ffile = load_fortune_file();

	// initialize random number only if we loaded tips
	if(ffile)
		srand(time(0));

	win = new Fl_Window(535, 260, _("EDE Tips"));
	win->begin();
		Fl_Group* main_group = new Fl_Group(10, 10, 515, 205);
		main_group->box(FL_DOWN_BOX);
		main_group->color(FL_WHITE);
		main_group->begin();
			Fl_Box* image_box = new Fl_Box(11, 13, 121, 201);
			image_box->image(image_hint);

			title_box = new Fl_Box(155, 23, 355, 25, random_title(title_tips, TITLE_TIPS_NUM));
			title_box->labelfont(FL_HELVETICA_BOLD);
			title_box->align(196|FL_ALIGN_INSIDE);

      		tip_box = new Fl_Box(155, 60, 355, 140);
			tip_box->align(197|FL_ALIGN_INSIDE);

			if(!ffile)
				tip_box->label(_("I'm unable to correctly load tip files. Please check what went wrong"));
			else
				tip_box->label(random_fortune());

		main_group->end();

		check_button = new Fl_Check_Button(10, 224, 225, 25, _("Show tips on startup"));
		check_button->down_box(FL_DOWN_BOX);

		read_autostart_stuff();

		Fl_Button* prev_button = new Fl_Button(240, 224, 90, 25, _("&Previous"));
		prev_button->callback(prev_cb);

		Fl_Button* next_button = new Fl_Button(335, 224, 90, 25, _("&Next"));
		next_button->callback(next_cb);

		Fl_Button* close_button = new Fl_Button(435, 224, 90, 25, _("&Close"));
		close_button->callback(close_cb);

		// check_button somehow steal focus
		close_button->take_focus();

	win->end();
	window_center_on_screen(win);
	win->show(argc, argv);

	Fl::run();

	if(ffile)
		fortune_close(ffile);

	return 0;
}
