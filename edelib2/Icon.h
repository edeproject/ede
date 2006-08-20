/*
 * $Id$
 *
 * edelib::Icon - General icon library with support for Icon themes
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


/*! \class edelib::Icon

This class gives support for KDE-compatible icon themes.
Icons are identified with their standard names, and the class
will fetch corresponding image from its standard location.
User can easily change all icons at once by setting the icon
theme.

There should be a freedesktop.org standard for icon names,
but I'm not aware of one so we will implement a sort of KDE
compatibility. Please refer to Icon theme documentation for
details.

*/

#ifndef _edelib_Icon_h_
#define _edelib_Icon_h_

#include <fltk/SharedImage.h>

using namespace fltk;

namespace edelib {

// This class builds on pngImage, which is a subclass of SharedImage.
// SharedImage.h code suggests that this will be dropped in favor of
// Image class. 

class Icon : public pngImage 
{
public:
	/*! Icon sizes:
	TINY - 16x16
	SMALL - 32x32
	MEDIUM - 48x48
	LARGE - 64x64
	HUGE - 128x128
	At the moment we only support and use TINY and MEDIUM sizes.*/
	enum IconSize {
		TINY = 16,
		SMALL = 32,
		MEDIUM = 48,
		LARGE = 64,
		HUGE = 128,
	};

	// Silence compiler warning
	virtual ~Icon();

	/*! Return edelib::Icon with given standard name and size. Adding 
	path or	.png extension is not necessary - just name is fine. 
	Example:  
	o->image(edelib::Icon::get("fileopen", edelib::Icon::TINY)); */
	static Icon* get(const char *name, int size = MEDIUM);

	/*! Give currently active theme and its directory. */
	static const char* get_theme() { if (!theme) read_theme_config(); return theme; }

	/*! Set theme as currently active. */
	static void set_theme(const char *t);
	
	int get_size() { return my_size; }

// In future, we might add methods for retrieving theme metadata 
// (name, author, URL, copyright and such)

private:
	static void read_theme_config();
	static const char* iconspath;
	static const char* defaulttheme;
	static char* theme;
	int my_size;
	char *my_theme;
};

}

#endif
