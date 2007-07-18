/*
 * $Id$
 *
 * About dialog
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "about_dialog.h"
#include <string.h>
#include <stdlib.h>

#include "../edeconf.h"
#include "NLS.h"
#include "Run.h"
#include "Util.h"


using namespace fltk;
using namespace edelib;

const char* copying_file = "copying.html";


static Window *aboutWindow;

void showCopyingInfo() 
{
	run_program(tsprintf("file:%s/%s", DOC_PATH, copying_file),false,false,true);
}

void cb_Click(Button* b, void*) 
{
	showCopyingInfo();
}

void cb_Close(Button*, void*) 
{
	aboutWindow->hide();
}

void edelib::about_dialog(const char *progname, const char *progversion, const char *addcomment) 
{
	aboutWindow = new Window(275, 190);
	aboutWindow->begin();
	{InvisibleBox* o = new InvisibleBox(5, 5, 265, 44);
		o->labelsize(18);
		o->label(tasprintf("%s %s",progname,progversion)); // tmp will be deallocated by InvisibleBox destructor
		o->box(FLAT_BOX);
	}
	{InvisibleBox* o = new InvisibleBox(5, 50, 265, 20);
		o->label(tasprintf(_("Part of Equinox Desktop Environment %s"),PACKAGE_VERSION)); // tmp will be deallocated by InvisibleBox destructor
		o->box(FLAT_BOX);
	}
	new InvisibleBox(5, 70, 265, 20, _("(C) Copyright 2000-2005 EDE Authors"));
	{InvisibleBox* o = new InvisibleBox(5, 90, 265, 40, _("This program is licenced under terms of the GNU General Public License version 2 or newer."));
		o->labelsize(10);
		o->align(ALIGN_INSIDE|ALIGN_WRAP);
	}
	{Button* o = new Button(65, 124, 145, 20, _("Click here for details."));
		o->box(NO_BOX);
		o->buttonbox(NO_BOX);
		o->labelcolor(BLUE);
		o->highlight_textcolor(RED);
		o->labelsize(10);
		o->callback((Callback*)cb_Click);
		((Window*)(o->parent()))->hotspot(o);
	}
	{Button* o = new Button(95, 152, 80, 25, "&Close");
		o->callback((Callback*)cb_Close);
	}
	aboutWindow->end();

	aboutWindow->label(tasprintf(_("About %s"), progname)); // tmp will be deallocated by Window destructor
	aboutWindow->set_modal();
	aboutWindow->resizable(aboutWindow);

	aboutWindow->end();
	aboutWindow->show();
}
