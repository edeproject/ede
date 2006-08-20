/*
 * $Id$
 *
 * Tip of the day
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "etip.h"
#include <fltk/SharedImage.h>
#include <fltk/xpmImage.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../edelib2/Config.h"
#include "../edelib2/NLS.h"

#include "../edeconf.h"

// graphics
#include "icons/hint.xpm"

#define	TOTALTIPS	7


static char *tips[TOTALTIPS];
static int activeTip = 0;
static edelib::Config conf("EDE Team", "etip");


fltk::CheckButton* show_check;
fltk::InvisibleBox* tipsBox;

static void cb_Previous(fltk::Button*, void*) {
	if (activeTip>0 && activeTip<=TOTALTIPS-1) {
		activeTip--;
	} else {
		activeTip = TOTALTIPS-1;
	}
	tipsBox->label(tips[activeTip]);
	tipsBox->window()->redraw();
}

static void cb_Next(fltk::Button*, void*) {
	if (activeTip>=0 && activeTip<TOTALTIPS-1) {
		activeTip++;
	} else {
		activeTip = 0;
	}
	tipsBox->label(tips[activeTip]);
	tipsBox->window()->redraw();
}

static void cb_Close(fltk::Button*, void*) {
	conf.set_section("Tips");
	conf.write("Show", !show_check->value()); 
	conf.flush(); 
	exit(0);
}

#include <fltk/run.h>

int main (int argc, char **argv) {

	fltk::Window* w;
	//fl_init_locale_support("etip", PREFIX"/share/locale");
	bool show = true;
	conf.set_section("Tips");
	conf.read("Show", show, true);
	if (!show)
		return 0;
	tips[0]=_("To start any application is simple. Click on the EDE button, go to the Programs menu, select category and click on the name of program that you wish to start.");
	tips[1]=_("To exit the Equinox Desktop Environment, click first on the EDE button then Logout.");
	tips[2]=_("To lock the computer, click first on the EDE button and then Lock.");
	tips[3]=_("To configure your computer, click on the EDE button, Panel menu and then Control panel.");
	tips[4]=_("To add a program that is not in the Programs menu, click on the EDE button, Panel menu, and then Edit panels menu.");
	tips[5]=_("Notice that this is still a development version, so please send your bug reports or comments on EDE forum, EDE bug reporting system (on project's page), or karijes@users.sourceforge.net.");
	tips[6]=_("You can download the latest release on: http://sourceforge.net/projects/ede.");
	
	srand (time(NULL));
	activeTip = rand()%7;

   {fltk::Window* o = new fltk::Window(400, 210, "Useful tips and tricks");
    w = o;
    o->begin();
     {fltk::InvisibleBox* o = new fltk::InvisibleBox(10, 15, 60, 145);
      o->set_vertical();
      o->image(&fltk::xpmImage(hint_xpm));
    }
     {fltk::Group* o = new fltk::Group(80, 15, 310, 125);
      o->box(fltk::BORDER_BOX);
//      o->color((fltk::Color)0xf4da1200);
      o->color(fltk::WHITE);
      o->labelsize(18);
      o->align(fltk::ALIGN_TOP|fltk::ALIGN_INSIDE|fltk::ALIGN_CLIP|fltk::ALIGN_WRAP);
      o->begin();
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(1, 1, 308, 45, _("Welcome to Equinox Desktop Environment"));
        o->box(fltk::FLAT_BOX);
        o->color((fltk::Color)0xf4da1200);
        o->labelcolor((fltk::Color)32);
        o->labelsize(18);
        o->align(fltk::ALIGN_INSIDE|fltk::ALIGN_WRAP);
      }
       {fltk::InvisibleBox* o = tipsBox = new fltk::InvisibleBox(5, 46, 300, 78);
        o->box(fltk::FLAT_BOX);
        o->color(fltk::WHITE);
        o->align(fltk::ALIGN_INSIDE|fltk::ALIGN_WRAP);
        fltk::Group::current()->resizable(o);
        o->label(tips[activeTip]);
        o->window()->redraw();
      }
      o->end();
      fltk::Group::current()->resizable(o);
    }
     {fltk::CheckButton* o = show_check = new fltk::CheckButton(80, 145, 310, 25, _("Do not show this dialog next time"));
      o->align(fltk::ALIGN_LEFT|fltk::ALIGN_INSIDE|fltk::ALIGN_WRAP);
    }
     {fltk::Group* o = new fltk::Group(0, 175, 400, 40);
      o->begin();
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(0, 5, 110, 40);
        fltk::Group::current()->resizable(o);
      }
       {fltk::Button* o = new fltk::Button(110, 0, 90, 25, _("@<  &Previous"));
        o->callback((fltk::Callback*)cb_Previous);
        o->align(fltk::ALIGN_WRAP);
      }
       {fltk::Button* o = new fltk::Button(205, 0, 90, 25, _("&Next  @>"));
        o->callback((fltk::Callback*)cb_Next);
      }
       {fltk::Button* o = new fltk::Button(300, 0, 90, 25, _("&Close"));
        o->callback((fltk::Callback*)cb_Close);
      }
      o->end();
    }
    o->end();
    o->size_range(o->w(), o->h());
  }
  w->show(argc, argv);
  return  fltk::run();
}
