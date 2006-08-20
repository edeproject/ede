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

#include "MimeType.h"

#include <fltk/filename.h>

#include "Run.h"
#include "Config.h"
#include "Util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <magic.h>

// I made this icon because on KDE there are stupidly two icons with same name
#define DEFAULT_ICON "misc-vedran"
#define FOLDER_ICON "folder"
#define RECYCLED_ICON "recycled"
#define LOCKED_ICON "lockoverlay"
#define FOLDERLOCKED_ICON "folder_locked"
// to be defined in separate file:
#define FILE_MANAGER "efiler"


using namespace fltk;
using namespace edelib;


// GLOBAL NOTE: asprintf() is a GNU extension - if it's unsupported on some
// platforms, use our tasprintf() instead (in Util.h)



// File format:
//  id|description|handler program|icon|wildcard for filename (extension)|wildcard for file command output|classic mime type
// 
//  - id - short string; to setup subtypes, just use slash (/) as separator in ID
//  - description - what is shown in gui
//  - handler program - filename will be appended, specify any needed parameters for opening - THIS WILL BE MOVED INTO SEPARATE FILE (for handling multiple programs etc.)
//  - icon - just name, don't give extension or path
//  - extension - will be used only if multiple types have same file command match. You don't need to give asterisk (*) i.e. .png. If there are multiple extensions, separate them with slash (/). Actually, "extension" can be any part of filename, but I can't think of use for this
//  - file output - relevant part of output from `file -bLnNp $filename`
//  - classic mime type - what is used for interchange i.e. text/plain - may be used for matching if other methods fail
// 
//  This is how mimetype resolving is supposed to work: if there is exactly one match for `file`
//  output, this is what we use. If there are multiple, the largest match is used. If there are
//  no results or several results with same size we look at extension, then at classic mime type
//  (using -i parameter to `file`).
// NOTE: not sure about this last thing, since -i parameter appears to just be alias



// queue/tree of mimetype data
static struct MimeData {
	char *id, *typestr, *program, *iconname, *extension, *file_output, *classic_mime;
	MimeData *next;
} *mime_first=0;


// This is used instead of strstrmulti to check if filename ends with any of extensions
char *test_extension(const char *file, const char *ext) {
	if (!file || !ext || (strlen(file)==0) || (strlen(ext)==0)) 
		return (char*)file; // this means that empty search returns true
	char *copy = strdup(ext);
	char *token = strtok(copy, "/");
	char *result = 0;
	do {
		int k = strlen(file)-strlen(token);
		if (strcmp(file+k,token) == 0) { return strdup(file+k); break; }
	} while ((token = strtok(NULL, "/"))); // double braces to silence compiler warnings :(
	free (copy);
	if (!result && (ext[strlen(ext)-1] == '/'))
		return (char*)file; // again
	return result;
}



// Read mime data from file

void get_mimedata() {
	// TODO: currently all mimes are on the same level...
	mime_first = (MimeData*) malloc(sizeof(MimeData)+1);
	MimeData *m = mime_first;

	char line[256];
	char* mime_filename = Config::find_file("mimetypes.conf");
	FILE *f = fopen(mime_filename,"r");
	bool first=true;
	while (!feof(f) && (fgets(line,255,f))) {
		if (feof(f)) break;

		// delete comments
		if (char* p=strchr(line,'#')) { *p = '\0'; }
		// delete spaces
		wstrim(line); 
		// if there's nothing left, skip
		if (strlen(line)<1) continue; 
		char *p1,*p2;
		if (!(p1 = strchr(line,'|'))) continue; // likewise

		// Allocate next element if this is not first pass
		if (!first) {
			m->next = (MimeData*) malloc(sizeof(MimeData)+1);
			m = m->next;
		}
		first=false;
		
		// parse line
		m->id = wstrim(strndup(line,p1-line));
		if (p1 && (p2 = strchr(++p1,'|'))) m->typestr = wstrim(strndup(p1,p2-p1)); else m->typestr=0;
		if (p2 && (p1 = strchr(++p2,'|'))) m->program = wstrim(strndup(p2,p1-p2)); else m->program=0;
		if (p1 && (p2 = strchr(++p1,'|'))) m->iconname = wstrim(strndup(p1,p2-p1)); else m->iconname=0;
		if (p2 && (p1 = strchr(++p2,'|'))) m->extension = wstrim(strndup(p2,p1-p2)); else m->extension=0;
		if (p1 && (p2 = strchr(++p1,'|'))) m->file_output = wstrim(strndup(p1,p2-p1)); else m->file_output=0;
		if (p2 && (p1 = strchr(++p2,'|'))) m->classic_mime = wstrim(strndup(p2,p1-p2)); else m->classic_mime=0;
	}
	fclose(f);
}


void free_mimedata() {
	MimeData *m, *n;
	m = mime_first;
	while ((n = m->next)) { free(m); m=n; }
	mime_first=0;
}


void print_mimedata() { // for debugging
	MimeData *m = mime_first;
	while (m != 0) {
		fprintf(stderr, "ID: '%s' Name: '%s' Prog: '%s' Icon: '%s' Ext: '%s' File: '%s' MIME: '%s'\n", m->id, m->typestr, m->program, m->iconname, m->extension, m->file_output, m->classic_mime);
		m = m->next;
	}
}


// declare given MimeData as current
void MimeType::set_found(char *id) {
	if (!id) return;

	// find id
	MimeData *m = mime_first;
	while (m && strcmp(m->id,id)!=0) m = m->next;

	// copy data to cur_*
	cur_id = strdup(id);
	if (m->typestr) cur_typestr = strdup(m->typestr);
	if (m->program && (strlen(twstrim(m->program))>0)) {
		asprintf (&cur_command, "%s \"%s\"", m->program, cur_filename);
	}
	if (m->iconname) cur_iconname = strdup(m->iconname);
	else cur_iconname = strdup(DEFAULT_ICON);
}



MimeType::MimeType(const char* filename, bool usefind) {
	cur_id=cur_typestr=cur_command=cur_iconname=cur_filename=0;
	if (filename && filename_exist(filename)) {
		cur_filename=strdup(filename);

		// Directory
		if (filename_isdir(filename)) {
			if (access(filename, R_OK || X_OK)) {
				// Not readable
				cur_id = strdup("directory/locked");
				cur_typestr = strdup("Directory (not accessible)");
				cur_iconname = strdup(FOLDERLOCKED_ICON);
			} else {
				cur_id = strdup("directory");
				cur_typestr = strdup("Directory");
				asprintf(&cur_command, "%s \"%s\"", FILE_MANAGER, filename);
				cur_iconname = strdup(FOLDER_ICON);
			}
			return;
		}

		// File not readable
		if (access(filename, R_OK)) {
			if (errno == EACCES) {
				cur_id = strdup("locked");
				cur_typestr = strdup("Can't read file");
				cur_iconname = strdup(LOCKED_ICON);
			}
			// we don't handle other errors specially
			return;
		}

		// Backup file
		if (filename[strlen(filename)-1] == '~') {
			cur_id = strdup("backup");
			cur_typestr = strdup("Backup file");
			cur_iconname = strdup(RECYCLED_ICON);
			return;
		}

		if (!mime_first) get_mimedata();

		// Stuff we need to declare before goto for visibility reasons
		MimeData *m = mime_first;
		//char buffer[256];
		const char* buffer;
		int found=0, foundext = 0;
		MimeData *file_matches[20], *ext_matches[20] = {0}; // this is for if(!ext_matches[0])

		if (!usefind) goto nofind; // using goto here makes less indentation ;)

		// execute file command
		// TODO: save cookie in a static variable
		magic_t cookie = magic_open(MAGIC_NONE);
		magic_load(cookie, NULL);
		buffer = magic_file(cookie, filename);
		magic_close(cookie);
/*		const int ourpid = getpid();
		run_program(tsprintf("%s -bLnNp '%s' >/tmp/ede-%d", GFILE, filename, ourpid));

		// read cmd output from temp file
		FILE *f = fopen (tsprintf("/tmp/ede-%d",ourpid),"r");
		fgets(buffer,255,f);
		fclose(f); // won't be more than 255 chars*/

fprintf (stderr,"(%s) File said: %s (Error: %s)\n",filename,buffer,magic_error(cookie));

		// find matches for 'file' command output
		// TODO: add wildcard matching
		do {
			if (m->file_output && (strstr(buffer,m->file_output)))
				file_matches[found++]=m;
		} while ((m=m->next));

		if (found == 1) { // one result found
			this->set_found(file_matches[0]->id); 
			return; 
		}

		if (found > 1) { // multiple results - find best match
			// We look for longest file match
			uint max=0;
			for (int i=0; i<found; i++)
				if (strlen(file_matches[i]->file_output)>max) 
					max = strlen(file_matches[i]->file_output);

fprintf(stderr, "Max: %d\n",max);
			// If all matches are empty, this is probably bogus
			if (max == 0) goto nofind;

			// Test to see if there are multiple best choices
			int j=0;
			for (int i=0; i<found; i++)
				if (strlen(file_matches[i]->file_output) == max) {
					fprintf (stderr, "Lokalni maximum '%s'\n", file_matches[i]->id);
					file_matches[j++] = file_matches[i];
				}
			// Now **file_matches should contain only maximums
			if (j==1) { this->set_found(file_matches[0]->id); return; }

			// Compare maximums on extension
			for (int i=0; i<j; i++)
				if (test_extension(filename,file_matches[i]->extension))
					ext_matches[foundext++] = file_matches[i];

			// No extension matches - accept first result (FIXME)
			if (foundext == 0) {
				this->set_found(file_matches[0]->id);
				return;
			}
			// From here we jump to comment " // continue extension matching"
		}

	nofind:
		if (!ext_matches[0]) {
			// Try extension matching on all mimetypes
			// This code will be executed if:
			//   a) find command is disabled
			//   b) find command returned zero matches (not likely, 
			// because some mimetypes have empty 'find' field)
			//   c) all of find results have equal length and no extensions
			// match (this is probably a misconfiguration, but its possible)
			m = mime_first;
			do {
				// take care not to match empty extension
				if (m->extension 
				&& (strlen(m->extension)>0) 
				&&
				(m->extension[strlen(m->extension)-1] != '/')
				&& (test_extension(filename,m->extension))) {
					fprintf (stderr, "Extenzija '%s'\n", m->id);
					ext_matches[foundext++]=m;
				}
			} while ((m=m->next));
		}

fprintf(stderr, "Foundext: %d\n", foundext);
		// continue extension matching
		if (foundext == 1) { // one result found
			this->set_found(ext_matches[0]->id); 
			return; 
		}

		if (foundext > 1) { // multiple results - find best match
			// Code is almost the same as above
			// We look for longest extension match
			uint max=0;
			for (int i=0; i<foundext; i++)
				if (strlen(ext_matches[i]->extension)>max &&
				(ext_matches[i]->extension[strlen(ext_matches[i]->extension)-1] != '/')) 
					max = strlen(ext_matches[i]->extension);

			// Test to see if there are multiple best choices
			int j=0;
			for (int i=0; i<foundext; i++)
				if (strlen(ext_matches[i]->extension) == max)
					ext_matches[j++] = ext_matches[i];
			// Now **ext_matches should contain only maximums
			if (j==1) { this->set_found(ext_matches[0]->id); return; }

			// Now what??? we return first one whether it be the only or not!
			// FIXME
			this->set_found(file_matches[0]->id); 
			return; 
		}

		// No extension results found - this is unknown file type
		cur_id = strdup("unknown");
		cur_typestr = strdup("Unknown");
		cur_iconname = strdup(DEFAULT_ICON);
	}
}


MimeType::~MimeType() {
	if (cur_id) free(cur_id);
	if (cur_typestr) free(cur_typestr);
	if (cur_command) free(cur_command);
	if (cur_iconname) free(cur_iconname);
	if (cur_filename) free(cur_filename);
	// free_mimedata() - should we? mimedata is static for a reason...
}
