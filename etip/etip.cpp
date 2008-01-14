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

#include <FL/Fl.h>
#include <FL/Fl_Window.h>
#include <FL/Fl_Check_Button.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Group.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Pixmap.h>

#include <edelib/Nls.h>
#include <edelib/Config.h>
#include <edelib/File.h>
#include <edelib/StrUtil.h>

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

void preformat_tip(edelib::String& str) {
	unsigned int sz = str.length();
	unsigned int j;

	for(unsigned int i = 0; i < sz; i++) {
		if(str[i] == '\n') {
			j = i;
			j++;
			if(j < sz && str[j] != '\n')
				str[i] = ' ';
		}
	}
}

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

void close_cb(Fl_Widget*, void*) {
	win->hide();
}

void next_cb(Fl_Widget*, void*) {
	if(!ffile)
		return;

	curr_tip++;
	if(curr_tip >= (int)fortune_num_items(ffile))
		curr_tip = 0;

	fortune_get(ffile, curr_tip, fstring);
	//preformat_tip(fstring);
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
