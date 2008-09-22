/*
 * $Id: etip.cpp 1664 2006-06-14 00:21:44Z karijes $
 *
 * Etip, show some tips!
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for the details.
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pixmap.H>

#include <edelib/Nls.h>
#include <edelib/Config.h>
#include <edelib/DesktopFile.h>
#include <edelib/File.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/MessageBox.h>
#include <edelib/Directory.h>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "icons/hint.xpm"
#include "Fortune.h"

static Fl_Pixmap image_hint(hint_xpm);

Fl_Window* win;
Fl_Check_Button* check_button;
Fl_Box* title_box;
Fl_Box* tip_box;
int curr_tip = 0;

FortuneFile* ffile = 0;
edelib::String fstring;

#define TITLE_TIPS_NUM 4
const char* title_tips[TITLE_TIPS_NUM] = {
	_("Did you know ?"),
	_("Tip of the Day"),
	_("A tip..."),
	_("Boring \"Did you know ?\"")
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
	edelib::Config conf;
	if(!conf.load("etip.conf"))
		return NULL;

	char path[1024];
	if(!conf.get("etip", "Path", path, sizeof(path)))
		return NULL;

	// check if file exists and at the same place we have .dat file
	if(!edelib::file_exists(path))
		return NULL;

	edelib::String dat = path;
	dat += ".dat";

	if(!edelib::file_exists(dat.c_str()))
		return NULL;

	return fortune_open(path, dat.c_str());
}

bool create_directory_if_needed(const edelib::String& path) {
	if(!edelib::dir_exists(path.c_str()) && !edelib::dir_create(path.c_str())) {
		edelib::alert(_("I'm not able to create %s. This error is probably since the path is too long or I don't have permission to do so"));
		return false;
	}

	return true;
}

/* 
 * Save/create etip.desktop inside autostart directory.
 * Autostart resides in ~/.config/autostart and directory does not
 * exists, it will be created (only /autostart/, not full path).
 *
 * TODO: edelib should have some dir_create function that should
 * create full path
 *
 * Saving/creating inside /etc/xdg/autostart is not done since 
 * ~/.config/autostart is prefered (see autostart specs) and all 
 * application will be looked there first.
 */
void write_autostart_stuff(void) {
	edelib::String path = edelib::user_config_dir();

	if(!create_directory_if_needed(path))
		return;

	// now try with autostart part
	path += "/autostart";
	if(!create_directory_if_needed(path))
		return;

	// now see if etip.desktop exists, and update it if do
	path += "/etip.desktop";
	edelib::DesktopFile conf;

	bool show_at_startup = check_button->value();

	if(!conf.load(path.c_str()))
		conf.create_new(edelib::DESK_FILE_TYPE_APPLICATION);

	// always write these values so someone does not try to play with us
	conf.set_hidden(show_at_startup);
	conf.set_name("Etip");
	conf.set_exec("etip");

	if(conf.save(path.c_str()) == false)
		edelib::alert(_("I'm not able to save %s. Probably I don't have enough permissions to do that ?"), path.c_str());
}

void read_autostart_stuff(void) {
	edelib::String path = edelib::user_config_dir();
	path += "/autostart/etip.desktop";

	edelib::DesktopFile conf;
	if(!conf.load(path.c_str())) {
		check_button->value(0);
		return;
	}

	if(conf.hidden())
		check_button->value(1);
	else
		check_button->value(0);
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
	ffile = load_fortune_file();

	// initialize random number only if we loaded tips
	if(ffile)
		srand(time(0));

	win = new Fl_Window(535, 260, _("EDE Tips"));
	win->begin();
		Fl_Group* main_group = new Fl_Group(10, 10, 515, 205);
		main_group->box(FL_DOWN_BOX);
		main_group->color(FL_BACKGROUND2_COLOR);
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
	win->show(argc, argv);

	Fl::run();

	if(ffile)
		fortune_close(ffile);

	return 0;
}
