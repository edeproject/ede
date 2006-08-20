/*
 * $Id$
 *
 * Configure window for eworkpanel
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "epanelconf.h"
#include "util.h"

#include <fltk/run.h>
#include <stdlib.h>

using namespace fltk;



// Widgets accessed from util.cpp
Input *workspaces[8];

Input* vcProgram;
Input* tdProgram;
Input* browserProgram;
Input* terminalProgram;
CheckButton* autohide_check;
ValueSlider* ws_slider;

Window* panelWindow;


// Callbacks

static void cb_Apply(Button*, void*) {
	write_config();
	send_workspaces();
}

static void cb_Close(Button*, void*) {
	exit(0);
}

static void cb_Browse(Button*, void*) {
//  char *file_types = _("Executables (*.*), *, All files (*.*), *");
//  const char *fileName = fl_select_file(0, file_types, _("File selection..."));    
	const char *fileName = file_chooser(_("Choose volume control program"), "*.*", vcProgram->value());
	if (fileName)
		vcProgram->value(fileName);
}

static void cb_Br(Button*, void*) {
//  char *file_types = _("Executables (*.*), *, All files (*.*), *");
//  const char *fileName = fl_select_file(0, file_types, _("File selection..."));    
	const char *fileName = file_chooser(_("Choose time&date program"), "*.*", tdProgram->value());
	if (fileName)
		tdProgram->value(fileName);
}

static void cb_Browse1(Button*, void*) {
//  char *file_types = _("Executables (*.*), *, All files (*.*), *");
//  const char *fileName = fl_select_file(0, file_types, _("File selection..."));    
	const char *fileName = file_chooser(_("Choose web browser program"), "*.*", browserProgram->value());
	if (fileName)
		browserProgram->value(fileName);
}


static void cb_Br1(Button*, void*) {
//  char *file_types = _("Executables (*.*), *, All files (*.*), *");
//  const char *fileName = fl_select_file(0, file_types, _("File selection..."));    
	const char *fileName = file_chooser(_("Choose file manager program"), "*.*", terminalProgram->value());
	if (fileName)
		terminalProgram->value(fileName);
}

static void cb_ws_slider(ValueSlider*, void*) {
	int val = int(ws_slider->value());
	for(int n=0; n<8; n++) {
		if(n<val)
			workspaces[n]->activate();
		else
			workspaces[n]->deactivate();
	}
}


// Main window

int main (int argc, char **argv) 
{

Window* w;
//fl_init_locale_support("epanelconf", PREFIX"/share/locale");
{
	Window* o = panelWindow = new Window(405, 270, _("Panel settings"));
	w = o;
	o->begin();
	{
		Button* o = new Button(205, 235, 90, 25, _("&Apply"));
		o->callback((Callback*)cb_Apply);
	}
	{
		Button* o = new Button(305, 235, 90, 25, _("&Close"));
		o->callback((Callback*)cb_Close);
	}
	{
	TabGroup* o = new TabGroup(10, 10, 385, 215);
	o->selection_color(o->color());
	o->selection_textcolor(o->textcolor());
	o->begin();
	{
		Group* o = new Group(0, 25, 385, 190, _("Utilities"));
		o->begin();
		{
			Group* o = new Group(10, 20, 365, 100, "Panel utilities");
			o->box(ENGRAVED_BOX);
			o->align(ALIGN_TOP|ALIGN_LEFT);
			o->begin();
			{
				Input* o = vcProgram = new Input(10, 20, 245, 25, _("Volume control program:"));
				o->align(ALIGN_TOP|ALIGN_LEFT);
			}
			{
				Button* o = new Button(265, 20, 90, 25, _("&Browse..."));
				o->callback((Callback*)cb_Browse);
			}
			{
				Input* o = tdProgram = new Input(10, 65, 245, 25, _("Time and date program:"));
				o->align(ALIGN_TOP|ALIGN_LEFT);
			}
			{
				Button* o = new Button(265, 65, 90, 25, _("Br&owse..."));
				o->callback((Callback*)cb_Br);
			}
			o->end();
		}
		{
			Group* o = new Group(10, 140, 365, 35, _("Autohide"));
			o->box(ENGRAVED_BOX);
			o->align(ALIGN_TOP|ALIGN_LEFT);
			o->begin();
			autohide_check = new CheckButton(10, 5, 345, 25, _("Automaticaly hide panel"));
			o->end();
		}
		o->end();
	}
	{
		Group* o = new Group(0, 25, 385, 190, _("Workspaces"));
		o->hide();
		o->begin();
		{
			ValueSlider* o = ws_slider = new ValueSlider(120, 10, 255, 25, _("Number of workspaces: "));
			o->type(ValueSlider::TICK_BELOW);
			o->box(THIN_DOWN_BOX);
			o->buttonbox(THIN_UP_BOX);
			o->step(1);
			o->callback((Callback*)cb_ws_slider);
			o->align(ALIGN_LEFT|ALIGN_WRAP);
			o->step(1); ;
			o->range(1,8);
		}
		{
			Group* o = new Group(10, 60, 370, 120, _("Workspace names:"));
			o->box(ENGRAVED_BOX);
			o->align(ALIGN_TOP|ALIGN_LEFT);
			o->begin();
			{
				Input* o = new Input(50, 5, 115, 20, _("WS 1:"));
				o->deactivate();
				workspaces[0] = o; ;
			}
			{
				Input* o = new Input(50, 35, 115, 20, _("WS 2:"));
				o->deactivate();
				workspaces[1] = o; ;
			}
			{
				Input* o = new Input(50, 65, 115, 20, _("WS 3:"));
				o->deactivate();
				workspaces[2] = o; ;
			}
			{
				Input* o = new Input(50, 95, 115, 20, _("WS 4:"));
				o->deactivate();
				workspaces[3] = o; ;
			}
			{
				Input* o = new Input(250, 5, 115, 20, _("WS 5:"));
				o->deactivate();
				workspaces[4] = o; ;
			}
			{
				Input* o = new Input(250, 35, 115, 20, _("WS 6:"));
				o->deactivate();
				workspaces[5] = o; ;
			}
			{
				Input* o = new Input(250, 65, 115, 20, _("WS 7:"));
				o->deactivate();
				workspaces[6] = o; ;
			}
			{
				Input* o = new Input(250, 95, 115, 20, _("WS 8:"));
				o->deactivate();
				workspaces[7] = o; ;
			}
			o->end();
		}
		o->end();
	}
	{
		Group* o = new Group(0, 25, 385, 190, _("Handlers"));
		o->hide();
		o->begin();
		{
			Group* o = new Group(10, 20, 365, 110, _("Handlers programs"));
			o->box(ENGRAVED_BOX);
			o->align(ALIGN_TOP|ALIGN_LEFT);
			o->begin();
			{
				Input* o = browserProgram = new Input(10, 20, 245, 25, _("Internet browser:"));
				o->align(ALIGN_TOP|ALIGN_LEFT);
			}
			{
				Button* o = new Button(265, 20, 90, 25, _("&Browse..."));
				o->callback((Callback*)cb_Browse1);
			}
			{
				Input* o = terminalProgram = new Input(10, 65, 245, 25, _("Terminal:"));
				o->align(ALIGN_TOP|ALIGN_LEFT);
			}
			{
				Button* o = new Button(265, 65, 90, 25, _("Br&owse..."));
				o->callback((Callback*)cb_Br1);
			}
			o->end();
		}
		o->end();
	}
	o->end();
	} // TabGroup
	o->end();
}

read_config();
update_workspaces();
w->show(argc, argv);
return  run();
}
