/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <FL/Fl.H>
#include <edelib/Debug.h>

#include <sys/types.h>
#include <pwd.h>

#include "ElmaWindow.h"
#include "Background.h"
#include "TextArea.h"
#include "Theme.h"

#define USER_AND_PASS_BOX_VISIBLE 1
#define USER_OR_PASS_BOX_VISIBLE  2

static void timeout_cb(void* e) {
	ElmaWindow* win = (ElmaWindow*)e;
	win->allow_input();
}

ElmaWindow::ElmaWindow(int W, int H) : Fl_Double_Window(0, 0, W, H), 
	bkg(0), user_in(0), pass_in(0), error_display(0), info_display(0) {

	deny_mode = false;
	box_mode = USER_AND_PASS_BOX_VISIBLE;
}

ElmaWindow::~ElmaWindow() {
}

bool ElmaWindow::create_window(ElmaTheme* et) {
	EASSERT(et != NULL);

	begin();
		bkg = new Background(0, 0, w(), h());
		if(!bkg->load_images(et->background.c_str(), et->panel.c_str()))
			return false;

		bkg->panel_pos(et->panel_x, et->panel_y);

		ThemeBox* tb;

		tb = et->user;
		user_in = new TextArea(tb->x, tb->y, tb->w, tb->h);
		if(tb->label)
			user_in->label(tb->label->c_str());

		user_in->labelcolor(FL_WHITE);
		user_in->textfont(FL_HELVETICA_BOLD);
		user_in->textsize(tb->font_size);

		tb = et->pass;
		pass_in = new TextArea(tb->x, tb->y, tb->w, tb->h);
		if(tb->label)
			pass_in->label(tb->label->c_str());

		pass_in->type(FL_SECRET_INPUT);
		pass_in->labelcolor(FL_WHITE);
		pass_in->textfont(FL_HELVETICA_BOLD);
		pass_in->textsize(tb->font_size);

		/*
		 * If username box and password box are at the same place, hide
		 * password box. With this, password box will be shown when user
		 * press enter on username box and reverse.
		 */
		if(pass_in->x() == user_in->x() && pass_in->y() == user_in->y()) {
			box_mode = USER_OR_PASS_BOX_VISIBLE;
			pass_in->hide();
		}

		tb = et->error;
		error_display = new Fl_Box(tb->x, tb->y, tb->w, tb->h);
		error_display->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);
		error_display->labelcolor(FL_WHITE);
		error_display->labelsize(tb->font_size);
		error_display->hide();

		tb = et->info;
		info_display = new Fl_Box(tb->x, tb->y, tb->w, tb->h);
		info_display->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);
		info_display->labelcolor(FL_GRAY);
		if(tb->label)
			info_display->label(tb->label->c_str());
	end();

	return true;
}

void ElmaWindow::validate_user(void) {
	EDEBUG("username: %s\n", user_in->value() ? user_in->value() : "none");
	EDEBUG("password: %s\n", pass_in->value() ? pass_in->value() : "none");

	error_display->show();
	error_display->label("Bad username or password");

	deny_input();

	Fl::add_timeout(3.0, timeout_cb, this);
}

void ElmaWindow::allow_input(void) {
	deny_mode = false;

	if(error_display->visible()) {
		error_display->hide();
		error_display->label(0);
	}

	if(user_in->visible())
		user_in->activate();
	if(pass_in->visible())
		pass_in->activate();
}

void ElmaWindow::deny_input(void) {
	deny_mode = true;

	if(user_in->visible())
		user_in->deactivate();
	if(pass_in->visible())
		pass_in->deactivate();
}

int ElmaWindow::handle(int event) {
	if(event == FL_KEYBOARD) {
		if(deny_mode)
			return 1;

		if(Fl::event_key() == FL_Enter) {
			if(box_mode == USER_AND_PASS_BOX_VISIBLE) {
				if(Fl::focus() == user_in)
					Fl::focus(pass_in);
				else {
					validate_user();
					// don't remember password
					pass_in->value(0);
					Fl::focus(user_in);
				}
			} else {
				if(user_in->visible()) {
					user_in->hide();
					pass_in->show();
					// don't remember password
					pass_in->value(0);
				} else {
					user_in->show();
					pass_in->hide();

					validate_user();
				}
			}

			return 1;
		}
	}

	return Fl_Double_Window::handle(event);
}
