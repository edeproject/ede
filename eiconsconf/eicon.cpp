/*
 * $Id$
 *
 * Icon properties (for eiconman - the EDE desktop)
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "eicon.h"

using namespace fltk;
using namespace edelib;


int label_background =  46848
;
int label_foreground = WHITE;
int label_fontsize = 12;
int label_maxwidth = 75;
int label_gridspacing = 16
;
bool label_trans = true;
bool label_engage_1click = false;
bool auto_arr = false;

static void sendClientMessage(XWindow w, Atom a, long x)
{
/*  XEvent ev;
  long mask;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = CurrentTime;
  mask = 0L;
  if (w == RootWindow(fl_display, fl_screen))
    mask = SubstructureRedirectMask;	   
  XSendEvent(fl_display, w, False, mask, &ev);*/
}

void sendUpdateInfo() 
{
// no worky
/*    unsigned int i, nrootwins;
    Window dw1, dw2, *rootwins = 0;
    int screen_count = ScreenCount(fl_display);
    extern Atom FLTKChangeSettings;
    for (int s = 0; s < screen_count; s++) {
        Window root = RootWindow(fl_display, s);
        XQueryTree(fl_display, root, &dw1, &dw2, &rootwins, &nrootwins);
        for (i = 0; i < nrootwins; i++) {
            if (rootwins[i]!=RootWindow(fl_display, fl_screen)) {
                sendClientMessage(rootwins[i], FLTKChangeSettings, 0);
            }
        }
    }
    XFlush(fl_display);*/
}

void
 readIconsConfiguration()
{
    Config globalConfig(Config::find_file("ede.conf", 0), true, false);
    globalConfig.set_section("IconManager");

    globalConfig.read("Label Background", label_background, 46848);
    globalConfig.read("Label Transparent", label_trans, false);
    globalConfig.read("Label Foreground", label_foreground, WHITE);
    globalConfig.read("Label Fontsize", label_fontsize, 12);
    globalConfig.read("Label Maxwidth", label_maxwidth, 75);
    globalConfig.read("Gridspacing", label_gridspacing, 16);
    globalConfig.read("OneClickExec", label_engage_1click, false);
    globalConfig.read("AutoArrange", auto_arr, false);
}

void writeIconsConfiguration()
{
    Config globalConfig(Config::find_file("ede.conf", true));
    globalConfig.set_section("IconManager");

    globalConfig.write("Label Background", label_background);
    globalConfig.write("Label Transparent", label_trans);
    globalConfig.write("Label Foreground", label_foreground);
    globalConfig.write("Label Fontsize", label_fontsize);
    globalConfig.write("Label Maxwidth", label_maxwidth);
    globalConfig.write("Gridspacing", label_gridspacing);
    globalConfig.write("OneClickExec", label_engage_1click);
    globalConfig.write("AutoArrange", auto_arr);
}


