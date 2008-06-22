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


// Open with.. dialog window


#ifndef _OpenWith_h_
#define _OpenWith_h_

#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/String.h>
#include <edelib/List.h>

#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>

class OpenWith : public edelib::Window {
private:
	const char* _file, * _type;
	edelib::list<edelib::String> programs;
	Fl_Input *inpt;
	Fl_Check_Button *always_use;
	Fl_Box *txt;
public:
	// Create and initialize OpenWith dialog
	OpenWith();

	// Show openwith dialog for file
	// - pfile - full path to file
	// - ptype - mimetype string e.g. "text/plain"
	// - pcomment - mimetype description e.g. "Plain text document"
	void show(const char* pfile, const char* ptype, const char* pcomment);

	// This function is friend of OpenWith because it needs to access
	// _type and _file
	friend void openwith_ok_cb(Fl_Widget*w, void*i);
};


#endif
