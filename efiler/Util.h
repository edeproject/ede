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

// Returns nicely formatted string for byte sizes e.g. "1.2 kB" for size=1284
const char* nice_size(double size);

// Returns nicely formatted string for date and time given in seconds since
// Epoch. This should be in config
const char* nice_time(long int epoch);

// Create vector from string using separator
//std::vector<char*> vec_from_string(const char *str, const char *separator);


/*! \fn const char* edelib::tsprintf(char* format, ...)

A useful function which executes sprintf() on a static char[] variable big enough to
hold short temporary strings. The variable remains valid until next call.

Use:
	run_program(tsprintf(PREFIX"/bin/eiconsconf %s",param));

When setting text values of fltk objects, instead use tasprintf which executes a strdup.
Example:
	window->label(tasprintf("%s, version %s",appname,appversion));
*/

const char* tsprintf(char* format, ...);

char* tasprintf(char* format, ...);

//}

#endif
