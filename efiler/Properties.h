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


// File properties window


#ifndef _Properties_h_
#define _Properties_h_

#include <edelib/Window.h>
#include <edelib/MimeType.h>

class Properties : public edelib::Window {
private:
	const char* _file;

	// We need our own MIME resolver
	edelib::MimeType mime_resolver;

public:
	// Construct window
	Properties(const char* file);

	// This function is friend of Properties because it needs to access
	// values of various widgets
	friend void properties_ok_cb(Fl_Widget*w, void*i);

};

#endif
