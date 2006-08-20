/*
 * $Id$
 *
 * Screensaver configuration
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "escrsaverconf.h"
#include "escreensaver.h"
#include "../edelib2/NLS.h"

#include <fltk/Symbol.h>
#include <fltk/xpmImage.h>
#include <fltk/run.h>
#include "icons/energy.xpm"

fltk::Window* mainWindow;

static void cb_mainWindow(fltk::Window*, void*) {
  clearOnExit();
}

fltk::InputBrowser* saversList;

static void cb_saversList(fltk::InputBrowser*, void*) {
  startSaverPreview();
}

fltk::ValueInput* timeoutSlider;

static void cb_OK(fltk::Button*, void*) {
  writeConfiguration(); clearOnExit();
}

static void cb_Cancel(fltk::Button*, void*) {
  clearOnExit();
}

fltk::Group* dpmsGroup;
fltk::ValueInput* standbySlider;
fltk::ValueInput* suspendSlider;
fltk::ValueInput* offSlider;
fltk::CheckButton* enableDPMSCheck;

static void cb_enableDPMSCheck(fltk::CheckButton*, void*) {
  if (enableDPMSCheck->value()) dpmsGroup->activate(); else  dpmsGroup->deactivate();
  enableDPMSCheck->redraw();
}


static void cb_Apply(fltk::Button*, void*) {
  writeConfiguration();
}

fltk::Window* saverWindow;

int main(int argc, char **argv) {
  fltk::Window* w;
  //fl_init_locale_support("escrsaverconf", PREFIX"/share/locale");
   {fltk::Window* o = mainWindow = new fltk::Window(300, 420, _("Screensaver settings"));
    w = o;
    o->set_vertical();
    o->callback((fltk::Callback*)cb_mainWindow);
    o->begin();
     {fltk::Group* o = new fltk::Group(10, 185, 280, 45, "Screensaver");
      o->box(fltk::ENGRAVED_BOX);
      o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
      o->begin();
       {fltk::InputBrowser* o = saversList = new fltk::InputBrowser(10, 10, 155, 25);
        o->callback((fltk::Callback*)cb_saversList);
        o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
        //o->type(1); 
	getScreenhacks();
        fillSaversList(o);
      }
       {fltk::Group* o = new fltk::Group(165, 5, 105, 35);
        o->begin();
         {fltk::ValueInput* o = timeoutSlider = new fltk::ValueInput(65, 5, 40, 25, "Timeout:");
          o->maximum(60);
          o->step(1);
          o->value(1);
          o->align(fltk::ALIGN_LEFT|fltk::ALIGN_CLIP|fltk::ALIGN_WRAP);
        }
        o->end();
      }
      o->end();
    }
     {fltk::Group* o = new fltk::Group(10, 255, 280, 115, "DPMS");
      o->box(fltk::ENGRAVED_BOX);
      o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
      o->begin();
       {fltk::Group* o = dpmsGroup = new fltk::Group(70, 0, 205, 108);
        o->deactivate();
        o->begin();
         {fltk::ValueInput* o = standbySlider = new fltk::ValueInput(160, 10, 40, 25, "Standby:");
          o->maximum(60);
          o->step(1);
          o->value(10);
          o->align(fltk::ALIGN_LEFT|fltk::ALIGN_WRAP);
        }
         {fltk::ValueInput* o = suspendSlider = new fltk::ValueInput(160, 45, 40, 25, "Suspend:");
          o->maximum(60);
          o->step(1);
          o->value(15);
          o->align(fltk::ALIGN_LEFT|fltk::ALIGN_WRAP);
        }
         {fltk::ValueInput* o = offSlider = new fltk::ValueInput(160, 80, 40, 25, "Off:");
          o->maximum(60);
          o->step(1);
          o->value(20);
          o->align(fltk::ALIGN_LEFT|fltk::ALIGN_WRAP);
        }
        o->end();
      }
       {fltk::CheckButton* o = enableDPMSCheck = new fltk::CheckButton(10, 45, 145, 25, "Enabled");
        o->callback((fltk::Callback*)cb_enableDPMSCheck);
        o->align(fltk::ALIGN_LEFT|fltk::ALIGN_INSIDE|fltk::ALIGN_WRAP);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(10, 10, 55, 35);
        fltk::xpmImage *img = new fltk::xpmImage((const char**)energy_xpm);
        o->image(img);
      }
      o->end();
    }
//     {fltk::Button* o = new fltk::Button(0, 380, 90, 25, "&OK");
//      o->callback((fltk::Callback*)cb_OK);
//    }
     {fltk::Button* o = new fltk::Button(100, 380, 90, 25, "&Apply");
      o->callback((fltk::Callback*)cb_Apply);
    }
     {fltk::Button* o = new fltk::Button(200, 380, 90, 25, "&Close");
      o->callback((fltk::Callback*)cb_Cancel);
    }
     {fltk::Group* o = new fltk::Group(45, 5, 200, 165);
      o->begin();
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(10, 6, 180, 131);
        o->box(fltk::UP_BOX);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(20, 15, 160, 110);
        o->box(fltk::DOWN_BOX);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(70, 137, 59, 3);
        o->box(fltk::THIN_UP_BOX);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(52, 140, 95, 12);
        o->box(fltk::UP_BOX);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(164, 127, 15, 6);
        o->box(fltk::THIN_UP_BOX);
      }
       {fltk::InvisibleBox* o = new fltk::InvisibleBox(157, 128, 2, 4);
        o->set_vertical();
        o->box(fltk::FLAT_BOX);
        o->color(fltk::GREEN);
      }
       {fltk::Window* o = saverWindow = new fltk::Window(22, 17, 156, 106);
        o->box(fltk::FLAT_BOX);
        o->color(fltk::BLACK);
        o->end();
      }
      o->end();
    }
    o->end();
  }
  readConfiguration(); 
  cb_enableDPMSCheck(enableDPMSCheck, 0); //deactivate controls if it's off
  mainWindow->end(); 
  mainWindow->show(); 
  startSaverPreview(); //preview active saver
  return fltk::run();
}
