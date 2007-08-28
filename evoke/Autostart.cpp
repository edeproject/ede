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

#include <edelib/Nls.h>
#include <FL/Fl_Pixmap.h>
#include <FL/Fl.h>

Fl_Pixmap warnpix(warning_xpm);

void closeit_cb(Fl_Widget*, void* w) {
	AstartDialog* win = (AstartDialog*)w;
	win->hide();
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
		run_sel = new Fl_Button(45, 270, 125, 25, _("Run &selected"));
		run_all = new Fl_Button(175, 270, 90, 25, _("&Run all"));
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
