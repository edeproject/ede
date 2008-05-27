/*
 * $Id$
 *
 * Library of useful functions
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#ifndef edelib_Util_h
#define edelib_Util_h

//#include "../edeconf.h"


//namespace edelib {


// Constants
//#define SYSTEM_PATH PREFIX"/share/ede"
#define SYSTEM_PATH "/usr/share/ede"
#define DOC_PATH SYSTEM_PATH"/doc"


// Cross-platform test if path is absolute or relative
int is_path_rooted(const char *fn);

// Recursively create a path in the file system
bool make_path( const char *path );

// Create the path needed for file using make_path
bool make_path_for_file( const char *path );

// Cross-platform function for system files location
char* get_sys_dir();

// strcat() that also does realloc if needed. Useful if
// e.g. you have a loop which grows string in each pass
//  -- Note: due to use of realloc *always* use strdupcat return value:
//            dest = strdupcat(dest,src);
// and *never* use it like:
//            strdupcat(dest,src);

// NOTE this function is not used! Its use is not recommended

char* strdupcat(char *dest, const char *src);

// Whitespace trim (both left and right)
char* wstrim(char *string);

// Version with temporary results (static char[])
const char* twstrim(const char *string);

// Finds in haystack any of strings contained in string "needles". The substrings
// are divided with separator.
// Not actually used...
char* strstrmulti(const char *haystack, const char *needles, const char *separator);

/*! \fn const char* edelib::nice_size(double size)

Converts the given number into a human-readable file size. Number is double since
it can store larger numbers than long (e.g. >4GB). Function returns a pointer to
a local static char array, which means that you need to use it (e.g. display on 
screen) before the next call to nice_size(). E.g. if you use it for a fltk label,
be sure to use copy_label() method. Example:
	struct stat buf;
	stat("somefile.txt",&buf);
	size_box->copy_label(nice_size(buf.st_size));
*/

const char* nice_size(double size);

// Returns nicely formatted string for date and time given in seconds since
// Epoch. This should be in config
const char* nice_time(long int epoch);

// Create vector from string using separator
//std::vector<char*> vec_from_string(const char *str, const char *separator);


/*! \fn const char* edelib::tsprintf(char* format, ...)
\fn const char* edelib::tasprintf(char* format, ...)

"Temporary sprintf" - does the same as sprintf() except that the value is stored in
a temporary char array (actually a static variable). This means that the value will
be valid until you call tsprintf the next time. This is useful for data that is
temporary in nature e.g. user interface messages, debugging messages etc.

Example:
	run_program(tsprintf(PREFIX"/bin/eiconsconf %s",param));

When setting text labels of fltk objects, use method copy_label() which creates a 
local copy, otherwise labels will change misteriously :)
	window->copy_label(tsprintf("Welcome to %s, version %s",appname,appversion));

tasprintf(...) also does allocation ("Temporary Allocate sprintf"), it's just a 
shortcut for strdup(tsprintf(...)) - with tasprintf you have to explicitely call
free() on created strings, with tsprintf you don't.
*/

const char* tsprintf(const char* format, ...);


char* tasprintf(const char* format, ...);

//}

#endif
