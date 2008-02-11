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

#include <FL/Fl.h>
#include <edelib/Debug.h>

#include <sys/types.h>
#include <pwd.h>

#include "ElmaWindow.h"
#include "Background.h"
#include "TextArea.h"

static void timeout_cb(void* e) {
	ElmaWindow* win = (ElmaWindow*)e;
	win->allow_input();
}

ElmaWindow::ElmaWindow(int W, int H) : Fl_Double_Window(0, 0, W, H), 
	bkg(0), user_in(0), pass_in(0), error_display(0), info_display(0) {

	deny_mode = false;
}

ElmaWindow::~ElmaWindow() {
}

bool ElmaWindow::load_everything(void) {
	begin();
		bkg = new Background(0, 0, w(), h());
		if(!bkg->load_images("themes/default/background.jpg", "themes/default/panel.png"))
			return false;

		//bkg->panel_pos(35, 189);
		bkg->panel_pos(235, 489);

		//user_in = new TextArea(428, 351, 184, 28, "Username:  ");
		user_in = new TextArea(240, 489, 184, 30, "Username:  ");
		user_in->labelcolor(FL_WHITE);
		user_in->textfont(FL_HELVETICA_BOLD);
		user_in->textsize(14);

		//pass_in = new TextArea(428, 351, 184, 28, "Password:  ");
		pass_in = new TextArea(240, 489, 184, 30, "Password:  ");
		pass_in->labelcolor(FL_WHITE);
		pass_in->textfont(FL_HELVETICA_BOLD);
		pass_in->textsize(14);
		pass_in->type(FL_SECRET_INPUT);
		pass_in->hide();

		//error_display = new Fl_Box(418, 390, 184, 28);
		error_display = new Fl_Box(240, 520, 184, 28);
		error_display->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);
		error_display->labelcolor(FL_WHITE);
		error_display->labelsize(14);
		error_display->hide();

		info_display = new Fl_Box(10, 10, 184, 28);
		info_display->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);
		info_display->labelcolor(FL_GRAY);
		info_display->label("EDE version 2.0");
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

			return 1;
		}
	}

	return Fl_Double_Window::handle(event);
}
