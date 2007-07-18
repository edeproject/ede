/*
 * $Id$
 *
 * X server properties
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "edisplayconf.h"
#include "conf.h"
#include "exset.h"
#include "../edelib2/Config.h" //#include <efltk/Fl_Config.h>
// if I move Config.h above exset.h, I get "'Font' does not name a type" in Xlib.h

using namespace edelib;


extern edisplayconf *app;
Config config(Config::find_file("ede.conf",1));
Exset xset;

int get_value(const char *key, int def_value)
{
    static int val;
    config.read(key, val, def_value);
    return val;
}

void do_xset()
{
  xset.set_mouse((int)app->slider_accel->value(),(int)app->slider_thresh->value());
  xset.set_bell((int)app->slider_volume->value(),(int)app->slider_pitch->value(),(int)app->slider_duration->value());
  xset.set_keybd((int)app->check_autorepeat->value(), (int)app->slider_click->value());
  xset.set_pattern((int)app->slider_delay->value(), (int)app->slider_pattern->value());
  xset.set_check_blank((int)app->check_blanking->value());
  xset.set_blank((int)app->radio_blank->value());

}


void read_disp_configuration()
{
  config.set_section("Mouse");
  app->slider_accel->value(get_value("Accel",4));		// Default 4
  app->slider_thresh->value(get_value("Thresh",4)); // Default 4
  config.set_section("Bell");
  app->slider_volume->value(get_value("Volume",50)); // default 50
  app->slider_pitch->value(get_value("Pitch",440)); // Default 440
  app->slider_duration->value(get_value("Duration",200)); // Default 200
  config.set_section("Keyboard");
  app->check_autorepeat->value(get_value("Repeat",1)); // Default 1
  app->slider_click->value(get_value("ClickVolume",50)); // Default 50
  app->slider_delay->value(get_value("Delay",15)); // Default 15
  config.set_section("Screen");
  app->check_blanking->value(get_value("CheckBlank",1)); // Default 1
  app->slider_pattern->value(get_value("Pattern",2)); // Default = 2

  int pattern = get_value("RadioPattern",0);
  int blank = get_value("RadioBlank",1);
  app->radio_blank->value(blank); // Default 1
  app->radio_pattern->value(pattern); // Default 1


  if( pattern )
      app->slider_pattern->activate();
  else
      app->slider_pattern->deactivate();
}

void write_configuration()
{
  config.set_section(config.create_section("Mouse"));
  config.write("Accel",(int)app->slider_accel->value());
  config.write("Thresh",(int)app->slider_thresh->value());

  config.set_section(config.create_section("Bell"));
  config.write("Volume",(int)app->slider_volume->value());
  config.write("Pitch",(int)app->slider_pitch->value());
  config.write("Duration",(int)app->slider_duration->value());

  config.set_section(config.create_section("Keyboard"));
  config.write("Repeat",(int)app->check_autorepeat->value());
  config.write("ClickVolume",(int)app->slider_click->value());

  config.set_section(config.create_section("Screen"));
  config.write("Delay",(int)app->slider_delay->value());
  config.write("Pattern",(int)app->slider_pattern->value());
  config.write("CheckBlank",(int)app->check_blanking->value());
  config.write("RadioBlank", (int)app->radio_blank->value());
  config.write("RadioPattern",(int) app->radio_pattern->value());

  config.flush();
  do_xset();
}




void cancelCB()
{
    app->_finish = true;
}

void testbellCB()
{
    xset.test_bell();
}

void TestBlankCB()
{
    xset.test_blank();
}


void applyCB()
{
    write_configuration();
}

void okCB()
{

    write_configuration();
    app->_finish = true;
}

