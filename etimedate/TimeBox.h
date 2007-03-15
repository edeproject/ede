// TimeBox.h
// Class that displays a clock with ability to set time
// Part of Equinox Desktop Environment (EDE)
//
// Based on Fl_Time.h
// Copyright (C) 2000 Softfield Research Ltd.
//
// Changes 02/09/2001 by Martin Pekar
// Ported to FLTK2 and redesigned by Vedran Ljubovic <vljubovic@smartnet.ba>, 2005.
//
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.


#ifndef __TIME_WIDGET_H
#define __TIME_WIDGET_H

#include "sys/time.h"

#include <fltk/Group.h>
#include <fltk/Button.h>
#include <fltk/Input.h>
#include <fltk/Clock.h>

#include "../edelib2/NLS.h"

#define TIMEBOX_24HOUR 0
#define TIMEBOX_12HOUR 1

class TimeBox : public fltk::Group {
  
public:
  TimeBox(int x, int y, int w, int h, char *l=0); 
  ~TimeBox();
  
  /**
   * Gets the hour.
   *
   * @return The hour associated with this widget.
   */  
  int hour();

  /**
   * Sets the hour.
   *
   * @param hour The hour associated with this widget.
   */  
  void hour(int value);
  
  /**
   * Gets the minute.
   *
   * @return The minute associated with this widget.
   */  
  int minute();

  /**
   * Sets the minute.
   *
   * @param minute The minute associated with this widget.
   */  
  void minute(int value);
  
  // Be sure to run this after using hour and min to change the clock value.
  void redisplay();
  void move_hands();
  
  /**
   * Sets the minute and hour at the same time.
   *
   * @param minute The minute associated with this widget.
   * @param hour The hour associated with this widget.
   */  
  void value(int h, int m);

  /**
   * Sets the minute and hour to the system minute and hour.
   */
  void current_time();

  /**
   * Refreshes the widget.
   */
  void refresh();

  /**
   * Sets the size of the label text which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @param size The size of the label font.
   */  
  void labelsize(int size);

  /**
   * Sets the label font which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @param font The label font.
   */  
  void labelfont(fltk::Font* font);

  /**
   * Sets the label color which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @param font The label color.
   */  
  void labelcolor(fltk::Color color);

  /**
   * Sets the size of the text which is used to display
   * the set time.
   *
   * @param size The size of the text font.
   */  
  void textsize(int size);

  /**
   * Sets the font of the text which is used to display
   * the set time.
   *
   * @param font The font of the text font.
   */  
  void textfont(fltk::Font*);

  /**
   * Sets the color of the text which is used to display
   * the set time.
   *
   * @param color The color of the text font.
   */  
  void textcolor(fltk::Color);
  
  /**
   * Gets the size of the label text which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @return The size of the label font.
   */  
  int labelsize();

  /**
   * Gets the label font which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @return The label font.
   */  
  fltk::Font* labelfont();

  /**
   * Gets the label color which is used for the M+,
   * M-, Y+, and Y- labels.
   *
   * @return The label color.
   */  
  fltk::Color labelcolor();
  
  /**
   * Gets the size of the text which is used to display
   * the set time.
   *
   * @return The size of the text font.
   */  
  int textsize();

  /**
   * Gets the font of the text which is used to display
   * the set time.
   *
   * @return The font of the text font.
   */  
  fltk::Font* textfont();

  /**
   * Gets the color of the text which is used to display
   * the set time.
   *
   * @return The color of the text font.
   */
  fltk::Color textcolor();

  /**
   * Determines if the entered time is a recognized format.
   *
   * @return True if it is a valid time format, otherwise false.
   */  
  bool valid();	
  
  int handle(int);
  void settime(); //just for superuser

private:
  fltk::ClockOutput  *clock;
  fltk::Button *button_decrease_hour;
  fltk::Button *button_decrease_minute;
  fltk::Input  *input_time;
  fltk::Button *button_increase_minute;
  fltk::Button *button_increase_hour;
  
  struct timeval *current_tv;
  struct timeval *display_tv;
  char time_string[20];
  bool last_valid;
  
  int look_;
  
  static void input_changed_cb(fltk::Widget* widget, void* data);
  static void button_cb(fltk::Widget* widget, void* data);
};

#endif
