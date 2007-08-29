/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Autostart.h"
#include "icons/warning.xpm"
#include "EvokeService.h"

#include <edelib/Nls.h>
#include <FL/Fl_Pixmap.h>
#include <FL/Fl.h>

Fl_Pixmap warnpix(warning_xpm);

void closeit_cb(Fl_Widget*, void* w) {
	AstartDialog* win = (AstartDialog*)w;
	win->hide();
}

void run_sel_cb(Fl_Widget*, void* w) {
	AstartDialog* win = (AstartDialog*)w;
	win->run_selected();
}

void run_all_cb(Fl_Widget*, void* w) {
	AstartDialog* win = (AstartDialog*)w;
	win->run_all();
}

AstartDialog::AstartDialog(unsigned int sz) : Fl_Window(370, 305, _("Autostart warning")), 
	curr(0), lst_sz(sz), lst(0) {

	if(lst_sz)
		lst = new AstartItem[lst_sz];

	begin();
		img = new Fl_Box(10, 10, 65, 60);
		img->image(warnpix);
		txt = new Fl_Box(80, 10, 280, 60, _("The following applications are registered for starting. Please choose what to do next"));
		txt->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_WRAP);
		cbrowser = new Fl_Check_Browser(10, 75, 350, 185);
		rsel = new Fl_Button(45, 270, 125, 25, _("Run &selected"));
		rsel->callback(run_sel_cb, this);
		rall = new Fl_Button(175, 270, 90, 25, _("&Run all"));
		rall->callback(run_all_cb, this);
		cancel  = new Fl_Button(270, 270, 90, 25, _("&Cancel"));
		cancel->callback(closeit_cb, this);
		cancel->take_focus();
	end();
}

AstartDialog::~AstartDialog() { 
	if(lst_sz)
		delete [] lst;
}

void AstartDialog::add_item(const edelib::String& n, const edelib::String& e) {
	if(!lst_sz)
		return;

	if(e.empty())
		return;

	AstartItem it;
	it.name = n;
	it.exec = e;

	lst[curr++] = it;
}

void AstartDialog::run(void) {
	for(unsigned int i = 0; i < curr; i++)
		cbrowser->add(lst[i].name.c_str());

	if(!shown())
		show();

	while(shown())
		Fl::wait();
}

void AstartDialog::run_selected(void) {
	int it = cbrowser->nchecked();
	if(!it)
		return;

	for(unsigned int i = 0; i < curr; i++) {
		if(cbrowser->checked(i+1))
			EvokeService::instance()->run_program(lst[i].exec.c_str());
	}

	hide();
}

void AstartDialog::run_all(void) {
	if(!curr)
		return;

	for(unsigned int i = 0; i < curr; i++)
		EvokeService::instance()->run_program(lst[i].exec.c_str());

	hide();
}
