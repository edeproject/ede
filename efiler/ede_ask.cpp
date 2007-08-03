/*
 * $Id$
 *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

// This is a replacement for fl_ask that uses system icons.
// ede_choice_alert() is just ede_choice() that uses alert icon.
// TODO before adding to edelib:
//   * return -1 in fl_choice() when user press Escape key or closes the window using [X]
//   * un-reverse order of buttons (I know how to order buttons myself thank you :) )
//   * add support for EDE sound events instead of the ugly beep
//   * ability to set messagebox title
//   * add ede_critical() (ede_alert() with icon messagebox_critical)
//   * fix for STR #1745


#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Widget.H>
#include <edelib/IconTheme.h>
#include <stdarg.h>


Fl_Image* img;

void ede_alert(const char*fmt, ...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("messagebox_warning",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,fmt);
	fl_alert(fmt);
	va_end(ap);
}

void ede_message(const char*fmt, ...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("messagebox_info",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,fmt);
	fl_message(fmt);
	va_end(ap);
}

int ede_ask(const char*fmt, ...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("help",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,fmt);
	int c=fl_ask(fmt);
	va_end(ap);
	return c;
}

int ede_choice(const char*fmt,const char *b0,const char *b1,const char *b2,...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("help",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,b2);
	int c=fl_choice(fmt,b0,b1,b2);
	va_end(ap);
	return c;
}

int ede_choice_alert(const char*fmt,const char *b0,const char *b1,const char *b2,...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("messagebox_warning",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,b2);
	int c=fl_choice(fmt,b0,b1,b2);
	va_end(ap);
	return c;
}

const char* ede_password(const char*fmt,const char *defstr,...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("password",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,defstr);
	const char* c = fl_password(fmt,defstr);
	va_end(ap);
	return c;
}


const char* ede_input(const char*fmt,const char *defstr,...) {
	va_list ap;

	img = Fl_Shared_Image::get(edelib::IconTheme::get("help",edelib::ICON_SIZE_MEDIUM).c_str());
	Fl_Widget* w = fl_message_icon(); 
	w->image(img);
	w->label("");
	w->align(FL_ALIGN_TOP| FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
	w->box(FL_NO_BOX);

	va_start(ap,defstr);
	const char* c = fl_input(fmt,defstr);
	va_end(ap);
	return c;
}
