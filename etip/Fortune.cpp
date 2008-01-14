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

#include "Fortune.h"
#include <netinet/in.h>
#include <ctype.h>

FortuneFile* fortune_open(const char* str_path, const char* dat_path) {
	FILE* sp = fopen(str_path, "r");
	if(!sp)
		return NULL;

	FILE* dp = fopen(dat_path, "r");
	if(!dp) {
		fclose(sp);
		return NULL;
	}

	FortuneFile* f = new FortuneFile;
	f->str_file = sp;
	f->dat_file = dp;

	fread((char*)&f->data, sizeof(StrFile), 1, f->dat_file);
	f->data.str_version = ntohl(f->data.str_version);
	f->data.str_numstr = ntohl(f->data.str_numstr);
	f->data.str_longlen = ntohl(f->data.str_longlen);
	f->data.str_shortlen = ntohl(f->data.str_shortlen);
	f->data.str_flags = ntohl(f->data.str_flags);

	return f;
}

void fortune_close(FortuneFile* f) {
	fclose(f->str_file);
	fclose(f->dat_file);

	delete f;
}

unsigned int fortune_num_items(FortuneFile* f) {
	return f->data.str_numstr; 
}

bool fortune_get(FortuneFile* f, unsigned int num, edelib::String& ret) {
	if(num >= fortune_num_items(f))
		return false;

	// read position from .dat file
	off_t seek_pts[2];

	fseek(f->dat_file, (long)(sizeof(StrFile) + num * sizeof(seek_pts[0])), 0);
	fread(seek_pts, sizeof(seek_pts), 1, f->dat_file);
	seek_pts[0] = ntohl(seek_pts[0]);
	seek_pts[1] = ntohl(seek_pts[1]);

	// now jump to that position in string file
	fseek(f->str_file, (long)seek_pts[0], 0);

	char buff[1024];
	char* p;
	char ch;

	ret.clear();

	while(fgets((char*)buff, sizeof(buff), f->str_file) != NULL && !STR_ENDSTRING(buff, f->data)) {
		if(f->data.str_flags & STR_ROTATED) {
			for(p = buff; (ch = *p); p++) {
				if(isupper(ch))
					*p = 'A' + (ch - 'A' + 13) % 26;
				else if(islower(ch))
					*p = 'a' + (ch - 'a' + 13) % 26;
			}
		}

		ret += buff;
	}

	return true;
}
