/*
 * $Id$
 *
 * Etip, show some tips!
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for the details.
 */

#ifndef __FORTUNE_H__
#define __FORTUNE_H__

#include <edelib/String.h>

/*
 * This is a reader for fortune plain and their corresponding binary .dat files.
 * Based on code from Amy A. Lewis since I was too lazy to search format specs
 * around the net.
 */

#define DAT_VERSION 2  // we know only for this

#define STR_ENDSTRING(line, tbl) ((line)[0] == tbl.stuff[0] && (line)[1] == '\n')
#define STR_ROTATED 0x3        // rot-13'd text

struct StrFile {
	unsigned int str_version;  // version number
	unsigned int str_numstr;   // number of strings in the file
	unsigned int str_longlen;  // length of the longest string
	unsigned int str_shortlen; // length of the shortest string
	unsigned int str_flags;    // bit field for flags
	char stuff[4];             // long aligned space, stuff[0] is delimiter
};

struct FortuneFile;

FortuneFile* fortune_open(const char* str_path, const char* dat_path);
void         fortune_close(FortuneFile* f);

// returns a number of known strings
unsigned int fortune_num_items(FortuneFile* f);

// gets strings at num; first is at 0 and last at fortune_num_items() - 1
bool         fortune_get(FortuneFile* f, unsigned int num, edelib::String& ret);

#endif
