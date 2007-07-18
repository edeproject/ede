/*
 * $Id$
 *
 * edelib::MimeType - Detection of file types and handling
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


/*! \class edelib::MimeType

This detects the type of file using "magic" (fingerprint 
usualy found in the first several bytes) and, if that fails,
"extension" (part of filename after last dot). To avoid code
duplication, GNU file is used.

Also this class suggests the command to be used for opening.

*/


#ifndef _edelib_MimeType_h_
#define _edelib_MimeType_h_

// TODO: have configure script detect libmagic and don't use
// it if its not present
#include <magic.h>

#include "Icon.h"

namespace edelib {

class MimeType
{
public:

	/*! Constructor takes filename and all interesting data is
	returned with methods listed below. filename must contain
	full path. Set usefind to false to avoid using GNU/find
	command.

	Note: We advise using empty constructor and set method in loops.*/

	MimeType(const char* filename=0, bool usefind=true);

	// Silence compiler warning
	virtual ~MimeType();

	/*! Scan file with given full path and store the results until
	next call of set() */
	void set(const char* filename, bool usefind=true);

	/*! Returns a string describing file type i.e. "PNG Image" */
	const char* type_string() { if (cur_typestr) return cur_typestr; else return 0;}

	/*! String that can be executed to open the given file
	or perform some default action (e.g. for .desktop files,
	the program that will be launched) */
	const char* command() { if (cur_command) return cur_command; else return 0; }

	/*! Returns edelib::Icon for files of this type. Parameter is
	edelib::Icon::IconSize */
	Icon* icon(int size) { if(cur_iconname) return Icon::get(cur_iconname,size); else return 0; }

	const char* id() { if(cur_id) return cur_id; else return 0; }

private:
	char *cur_id, *cur_typestr, *cur_command, *cur_iconname, *cur_filename;
	void set_found(char *id);
	magic_t magic_cookie; //handle for libmagic
};

}

#endif
