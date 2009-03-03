/*
 * $Id$
 *
 * About dialog
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <efltk/Fl_Widget.h>
#include <efltk/Fl_Util.h>
#include "aboutdialog.h"

void AboutDialog(Fl_Widget*, void*)
{
	fl_start_child_process("ede-launch ede-about", false);
}

#if 0
#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Text_Display.h>
#include <efltk/Fl_Locale.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Text_Buffer.h>

#include "aboutdialog.h"
#include "icons/ede.xpm"


Fl_Image ede_logo_pix((const char**)ede_xpm);

const char* licence =
_("This program is based in part on the work of\n\
FLTK project (www.fltk.org).\n\n\
This program is free software, you can redistribute\n\
it and/or modify it under the terms of GNU General\n\
Public License as published by the Free Software\n\
Foundation, either version 2 of the License, or\n\
(at your option) any later version.\n\n\
This program is distributed in the hope that it will\n\
be useful, but WITHOUT ANY WARRANTY;\n\
without even the implied\n\
warranty of MERCHANTABILITY or FITNESS\n\
FOR A PARTICULAR PURPOSE.\n\n\
See the GNU General Public License for more details.\n\
You should have received a copy of the GNU General\n\
Public Licence along with this program; if not, write\n\
to the Free Software Foundation, Inc., 675 Mass Ave,\n\
Cambridge, MA 02139, USA");

void close_cb(Fl_Widget*, void* w)
{
	Fl_Window* win = (Fl_Window*)w;
	win->hide();
}

void DetailsDialog(void)
{
	Fl_Window* win = new Fl_Window(395, 294, _("Details"));
	win->shortcut(0xff1b);
	win->begin();
	Fl_Button* close = new Fl_Button(310, 265, 80, 25, _("&Close"));
	close->callback(close_cb, win);
	Fl_Text_Display* txt = new Fl_Text_Display(5, 5, 385, 255);
	txt->box(FL_DOWN_BOX);
	Fl_Text_Buffer* buff = new Fl_Text_Buffer();
	txt->buffer(buff);
	txt->insert(licence);
	win->end();
	win->set_modal();
	win->show();
}

void details_cb(Fl_Widget*, void*)
{
	DetailsDialog();
}

void AboutDialog(Fl_Widget*, void*)
{
	Fl_Window* win = new Fl_Window(370, 214, _("About Equinox Desktop Environment"));
    win->shortcut(0xff1b);
	win->begin();
	Fl_Box* imgbox = new Fl_Box(10, 15, 55, 55);
	imgbox->image(ede_logo_pix);

	Fl_Box* title = new Fl_Box(75, 15, 285, 35, "Equinox Desktop Environment");
	title->label_font(fl_fonts+1);
	title->label_size(14);
	title->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Box* vers = new Fl_Box(75, 50, 285, 20, _("version "PACKAGE_VERSION));
	vers->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	Fl_Box* copyright = new Fl_Box(75, 85, 285, 20, "Copyright (c) EDE Authors 2000-2007");
	copyright->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Box* lic = new Fl_Box(75, 110, 285, 50, _("This program is licenced under terms of \
the GNU General Public Licence version 2 or newer.\nSee Details for more."));
	lic->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);

	Fl_Button* details = new Fl_Button(200, 185, 80, 25, _("&Details"));
	details->callback(details_cb);

	Fl_Button* close = new Fl_Button(285, 185, 80, 25, _("&Close"));
	close->callback(close_cb, win);


	win->end();
	win->set_modal();
	win->show();
}
#endif
