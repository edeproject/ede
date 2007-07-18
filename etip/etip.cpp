/*
 * $Id$
 *
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <fltk/Window.h>
#include <fltk/Button.h>
#include <fltk/CheckButton.h>
#include <fltk/InvisibleBox.h>
#include <fltk/xpmImage.h>
#include <fltk/run.h>

#include <stdlib.h>
#include <time.h>

#include "icons/hint.xpm"

// TODO: should be replaced with NLS from edelib
#define _(s) s

#define TIPS_NUM 7
#define TITLE_TIPS_NUM 9

using namespace fltk;

unsigned int curr_tip = 0;
const char* tiplist[TIPS_NUM] = 
{
_("To start any application is simple. Press on the button with your user name, go\
 to the Programs menu, select category and click on the wished program."),

_("To exit the EDE, press button with your user name and then Logout."),

_("To lock the computer, press button with your user name and then choose Lock."),

_("To setup things on the computer, press button with your user name, Panel menu and then the Control panel."),

_("To add a program that is not in the Programs menu, click on the button with your user,\
 Panel menu, and then Menu editor."),

_("Notice that this is still development version, so please send your bug reports or\
 comments on EDE forum, EDE bug reporting system (on project's page), or check mails of current\
 maintainers located in AUTHORS file."),

_("You can download latest release on http://sourceforge.net/projects/ede.")
};

const char* title_tips[TITLE_TIPS_NUM] =
{
_("Boring \"Did you know...\""),
_("How about this..."),
_("Smart idea..."),
_("Really smart idea..."),
_("Really really smart idea..."),
_("Uf..."),
_("Something new..."),
_("Or maybe this..."),
_("...")
};

const char* random_txt(const char** lst, unsigned int max)
{
	unsigned int val = rand() % max;
	curr_tip = val;
	return lst[val];
}

void close_cb(Widget*, void* w)
{
	Window* win = (Window*)w;
	win->hide();
}

void next_cb(Widget*, void* tb)
{
	InvisibleBox* tipbox = (InvisibleBox*)tb;
	curr_tip++;
	if(curr_tip >= TIPS_NUM)
		curr_tip = 0;
	tipbox->label(tiplist[curr_tip]);	
	tipbox->redraw_label();
}

void prev_cb(Widget*, void* tb)
{
	InvisibleBox* tipbox = (InvisibleBox*)tb;
	if(curr_tip == 0)
		curr_tip = TIPS_NUM - 1;
	else
		curr_tip--;
	tipbox->label(tiplist[curr_tip]);	
	tipbox->redraw_label();
}

int main(int argc, char** argv)
{
	srand(time(NULL));

	Window* win = new Window(460, 200, _("Tips..."));
	win->begin();

	InvisibleBox* img = new InvisibleBox(10, 10, 70, 65);
	xpmImage pix(hint_xpm);
	img->image(pix);
	img->box(FLAT_BOX);

	InvisibleBox* title = new InvisibleBox(85, 15, 365, 25, random_txt(title_tips, TITLE_TIPS_NUM));
	title->box(fltk::FLAT_BOX);
	title->labelfont(fltk::HELVETICA_BOLD);
	title->align(fltk::ALIGN_LEFT|fltk::ALIGN_INSIDE);

	InvisibleBox* tiptxt = new InvisibleBox(85, 45, 365, 110, random_txt(tiplist, TIPS_NUM));
	tiptxt->box(fltk::FLAT_BOX);
	tiptxt->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT|fltk::ALIGN_INSIDE|fltk::ALIGN_WRAP);

    new fltk::CheckButton(10, 165, 155, 25, _("Do not bother me"));

    Button* prev = new Button(170, 165, 90, 25, _("@< Previous"));
	prev->callback(prev_cb, tiptxt);
    Button* next = new Button(265, 165, 90, 25, _("Next @>"));
	next->callback(next_cb, tiptxt);

    Button* close = new Button(360, 165, 90, 25, _("&Close"));
	close->callback(close_cb, win);
	win->end();

	win->set_modal();
	win->show();
	return run();
}
