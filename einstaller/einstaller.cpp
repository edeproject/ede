/*
 * $Id$
 *
 * Package manager for Equinox Desktop Environment
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "einstaller.h"
#include "einstall.h"

#include <fltk/SharedImage.h>
#include <fltk/xpmImage.h>
#include <fltk/run.h>
#include <stdlib.h>

#include "icons/install.xpm"
#include "../edelib2/NLS.h"

#include "../edeconf.h"

using namespace fltk;


static TextBuffer *out_buffer;
static Window* install_window;

static xpmImage datas_install((const char **)install);

Group* steps_group;
Group* step1_group;
Input* package_name_input;
CheckButton* nodeps_button;
Group* step2_group;
TextEditor* result_output;
ProgressBar* install_progress;
Button* prev_button;
Button* next_button;

static void cb_package_name_input(Button*, void*) {
	next_button->activate();
}

static void cb_Browse(Button*, void*) {
//	char *file_types = _("Packages (*.rpm; *.tgz; *.deb), *.{rpm|tgz|deb}, All files (*.*), *");
	const char *f = file_chooser(_("Package selection"), "*.{rpm|tgz|deb|gz|bz2}", package_name_input->value());
	if (f) {
		package_name_input->value(f);
		next_button->activate();
	}
}

static void cb_prev_button(Button*, void*) {
	step1_group->show();
	step2_group->hide();
	prev_button->deactivate();
	next_button->activate();
	out_buffer->remove(0,out_buffer->length());
	flush();
}


static void cb_next_button(Button*, void*) {
	step1_group->hide();
	step2_group->show();
	prev_button->activate();
	next_button->deactivate();
	flush();
	install_package(package_name_input->value(), nodeps_button->value());
}

static void cb_Close(Button*, void*) {
	exit(0);
}


int main (int argc, char **argv) {
	//  fl_init_locale_support("einstaller", PREFIX"/share/locale");
	out_buffer = new TextBuffer();
	
	{Window* o = install_window = new Window(505, 315, "Install software package");
	o->begin();
	{
		InvisibleBox* o = new InvisibleBox(5, 5, 135, 270);
		o->set_vertical();
		o->image(datas_install);
		o->box(DOWN_BOX);
		o->color((Color)0x7d8300);
	}
	{
		Group* o = steps_group = new Group(145, 5, 350, 270);
		o->box(FLAT_BOX);
		o->begin();
		{
			Group* o = step1_group = new Group(0, 0, 350, 270);
			o->box(ENGRAVED_BOX);
			o->begin();
			{
				InvisibleBox* o = new InvisibleBox(5, 5, 340, 120, "Welcome. This installation wizard will help you to install new software on your computer.");
				o->labelsize(18);
				o->align(ALIGN_TOP|ALIGN_LEFT|ALIGN_INSIDE|ALIGN_WRAP);
			}
			{
				Input* o = package_name_input = new Input(5, 125, 240, 25, "Enter the name of software package you want to install:");
				o->align(ALIGN_TOP|ALIGN_LEFT|ALIGN_WRAP);
				o->callback((Callback*)cb_package_name_input);
			}
			{
				Button* o = new Button(250, 125, 90, 25, "&Browse...");
				o->callback((Callback*)cb_Browse);
			}
			nodeps_button = new CheckButton(5, 160, 338, 25, "Ignore dependencies");
			o->end();
		}
		{
			Group* o = step2_group = new Group(0, 0, 350, 270);
			o->box(ENGRAVED_BOX);
			o->hide();
			o->begin();
			{
				TextEditor* o = result_output = new TextEditor(5, 20, 335, 155, "Installation results:");
				o->align(ALIGN_TOP|ALIGN_LEFT|ALIGN_WRAP);
				o->buffer(out_buffer);
			}
			{
				ProgressBar* o = install_progress = new ProgressBar(5, 210, 335, 20, "Installation status:");
				o->align(ALIGN_TOP|ALIGN_LEFT);
			}
			o->end();
		}
		o->end();
	}
	{
		Button* o = prev_button = new Button(195, 280, 90, 25, "<< &Previous");
		o->callback((Callback*)cb_prev_button);
		o->deactivate();
	}
	{
		Button* o = next_button = new Button(295, 280, 90, 25, "&Install");
		o->callback((Callback*)cb_next_button);
		o->deactivate();
	}
	{
		Button* o = new Button(405, 280, 90, 25, "&Close");
		o->callback((Callback*)cb_Close);
	}
	o->end();
	;
	} // Window
	install_window->show(argc, argv);
	return run();
}
