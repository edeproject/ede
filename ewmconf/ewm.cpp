/*
 * $Id$
 *
 * edewm (EDE Window Manager) settings
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "ewm.h"

#include <stdlib.h>
#include <string.h>

#include <fltk/run.h>
#include <fltk/ask.h>
#include <fltk/ColorChooser.h>
#include <fltk/x.h>

#include "../edelib2/Config.h"


using namespace fltk;
using namespace edelib;




Color title_active_color, title_active_color_text;
Color title_normal_color, title_normal_color_text;

int title_height;
int title_align;

bool opaque_resize;
int title_draw_grad;

bool animate;
int animate_speed;

bool use_frame;
Color theme_frame_color;
bool use_theme;
char* theme_path;

static void sendClientMessage(XWindow w, Atom a, long x)
{
  XEvent ev;
  long mask;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = CurrentTime;
  mask = 0L;
  if (w == RootWindow(xdisplay, xscreen))
    mask = SubstructureRedirectMask;	   
  XSendEvent(xdisplay, w, False, mask, &ev);
}

void applyConfiguration()
{
    extern Atom FLTKChangeSettings;
//    sendClientMessage(RootWindow(xdisplay, xscreen), FLTKChangeSettings, 0);
    XFlush(xdisplay);
}

void readConfiguration()
{
    Config wmconf(Config::find_file("wmanager.conf", 0));

    wmconf.set_section("TitleBar");
    wmconf.read("Active color", title_active_color, color(0,0,128));
    wmconf.read("Active color text", title_active_color_text, color(255,255,255));
    wmconf.read("Normal color", title_normal_color, color(0,0,130));
    wmconf.read("Normal color text", title_normal_color_text, color(255,255,255));

    wmconf.read("Box type", title_draw_grad, 0);
    wmconf.read("Height", title_height, 20);
    wmconf.read("Text align", title_align, 0);

    wmconf.set_section("Resize");
    wmconf.read("Opaque resize", opaque_resize, false);
    wmconf.read("Animate", animate, true);
    wmconf.read("Animate Speed", animate_speed, 14);
    
    wmconf.set_section("Misc");
    wmconf.read("Use frame", use_frame, false);
    wmconf.read("Frame color", theme_frame_color, color(0,0,128));
    wmconf.read("Use theme", use_theme, false);
    wmconf.read("Theme path", theme_path, 0);
}

void writeConfiguration()
{
    if(title_height > 50) title_height=50;
    else if(title_height < 10) title_height=10;

    Config wmconf(Config::find_file("wmanager.conf", 1));

    wmconf.set_section("TitleBar");
    wmconf.write("Active color", title_active_color);
    wmconf.write("Active color text", title_active_color_text);
    wmconf.write("Normal color", title_normal_color);
    wmconf.write("Normal color text", title_normal_color_text);
    
    wmconf.write("Box type", title_draw_grad);
    wmconf.write("Height", title_height);
    wmconf.write("Text align", title_align);

    wmconf.set_section("Resize");
    wmconf.write("Opaque resize", opaque_resize);
    wmconf.write("Animate", animate);
    wmconf.write("Animate Speed", animate_speed);
    
    wmconf.set_section("Misc");
    wmconf.write("Use frame", use_frame);
    wmconf.write("Frame color", theme_frame_color);
    wmconf.write("Use theme", use_theme);
    if (use_theme)
    	wmconf.write("Theme path", theme_path);
    else
    	wmconf.write("Theme path", "");    
}

