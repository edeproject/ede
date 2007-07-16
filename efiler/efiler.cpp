/*
 * $Id$
 *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include <Fl/Fl.H>
#include <Fl/Fl_Window.H>
#include <Fl/filename.H>
#include <Fl/fl_ask.H>

#include <edelib/Nls.h>
#include <edelib/MimeType.h>
#include <edelib/String.h>

#include "EDE_FileView.h"
#include "Util.h"


#define DEFAULT_ICON "misc-vedran"

char current_dir[FL_PATH_MAX];
FileDetailsView* view;
bool showhidden;
bool semaphore;
Fl_Window* win;
bool dirsfirst;
bool ignorecase;

// These variables are global to save time on construction/destruction
edelib::MimeType mime_resolver;
struct stat buf;


// modification of versionsort which ignores case
void lowercase(char* s) { 
	for (int i=0;i<strlen(s);i++)
		if (s[i]>='A' && s[i]<='Z') s[i] += 'a'-'A';
}
int myversionsort(const void *a, const void *b) {
	struct dirent** ka = (struct dirent**)a;
	struct dirent** kb = (struct dirent**)b;
	char *ma = strdup((*ka)->d_name);
	char *mb = strdup((*kb)->d_name);
	lowercase(ma); lowercase(mb);
	int k = strverscmp(ma,mb);
	free(ma); free(mb);
	return k;
}

void loaddir(const char *path) {
	char old_dir[FL_PATH_MAX];

	// Set current_dir
	if (fl_filename_isdir(path)) {
		if (path[0] == '~') // Expand tilde
			snprintf(current_dir,PATH_MAX,"%s/%s",getenv("HOME"),path+1);
		else 
			strcpy(current_dir,path);
	} else
		strcpy(current_dir,getenv("HOME"));

	// Trailing slash should always be there
	if (current_dir[strlen(current_dir)-1] != '/') strcat(current_dir,"/");

	// Compact dotdot (..)
	if (char *tmp = strstr(current_dir,"/../")) {
		char *tmp2 = tmp+4;
		tmp--;
		while (tmp != current_dir && *tmp != '/') tmp--;
		tmp++;
		while (*tmp2 != '\0') *tmp++ = *tmp2++;
		*tmp='\0';
	}

fprintf (stderr, "loaddir(%s) = (%s)\n",path,current_dir);
	strncpy(old_dir,current_dir,strlen(current_dir));

	// Update directory tree
//	dirtree->set_current(current_dir);

	// variables used later
	int size=0;
	dirent **files;

	// List all files in directory
	if (ignorecase) 
		size = scandir(current_dir, &files, 0, myversionsort);
	else
		size = scandir(current_dir, &files, 0, versionsort);

	if (size<0) {
		fl_alert(_("Permission denied!"));
		strncpy(current_dir,old_dir,strlen(current_dir));
		return;
	}

	// set window label
	win->label(tasprintf(_("%s - File manager"), current_dir));

	// Clean up window
	view->clear();

	FileItem **item_list = new FileItem*[size];
	int fsize=0;

	for (int i=0; i<size; i++) {
		char *n = files[i]->d_name; //shortcut

		// don't show ./ (current directory)
		if (strcmp(n,".")==0) continue;

		// hide files with dot except ../ (up directory)
		if (!showhidden && (n[0] == '.') && (strcmp(n,"..")!=0)) continue;

		// hide files ending with tilde (backup) - NOTE
		if (!showhidden && (n[strlen(n)-1] == '~')) continue;

		char fullpath[FL_PATH_MAX];
		snprintf (fullpath,FL_PATH_MAX-1,"%s%s",current_dir,files[i]->d_name);

		if (stat(fullpath,&buf)) continue; // error

		FileItem *item = new FileItem;
		item->name = n;
		item->size = nice_size(buf.st_size);
		item->realpath = fullpath;
		if (strcmp(n,"..")==0) {
			item->icon = "undo";
			item->description = "Go up";
			item->size = "";
		} else if (S_ISDIR(buf.st_mode)) { // directory
			item->icon = "folder";
			item->description = "Directory";
			// item->name += "/";
			item->size = "";
		} else {
			item->icon = "unknown";
			item->description = "Unknown";
		}
		item->date = nice_time(buf.st_mtime);
		item->permissions = ""; // ?

		item_list[fsize++] = item;
	}

//	icon_array = (Button**) malloc (sizeof(Button*) * icon_num + 1);
	// fill array with zeros, for easier detection if button exists
//	for (int i=0; i<icon_num; i++) icon_array[i]=0;

//	int myx=startx, myy=starty;

	if (dirsfirst) {
		for (int i=0; i<fsize; i++) {
			if (item_list[i]->description == "Directory" || item_list[i]->description == "Go up")
				view->add(item_list[i]);
		}
		for (int i=0; i<fsize; i++) 
			if (item_list[i]->description != "Directory" && item_list[i]->description != "Go up")
				view->add(item_list[i]);
	} else {
		for (int i=0; i<fsize; i++) 
			view->add(item_list[i]);
	}
	view->redraw();
	Fl::check();

	// Update mime types - can be slow...
	for (int i=0; i<fsize; i++) {
		if (item_list[i]->description != "Directory" && item_list[i]->description != "Go up") {
			mime_resolver.set(item_list[i]->realpath.c_str());
			edelib::String desc,icon;
			desc = mime_resolver.comment();
			icon = mime_resolver.icon_name();
			if (desc!="" || icon!="") {
				if (desc != "") item_list[i]->description = desc;
				if (icon != "") item_list[i]->icon = icon;
fprintf (stderr, "ICON: %s !!!!!\n", icon.c_str());
				view->update(item_list[i]);
			}
			Fl::check();
		}
	}

	// Cleanup
	for (int i=0; i<fsize; i++) delete item_list[i];
	delete[] item_list;
}


// Open callback
void open_cb(Fl_Widget*w, void*data) {
fprintf (stderr,"cb\n");
	if (Fl::event_clicks() || Fl::event_key() == FL_Enter) {
fprintf (stderr,"enter\n");
		static time_t tm = 0;
		if (time(0)-tm < 1) return; // no calling within 1 second
		tm = time(0);

		char* path = (char*)view->data(view->value());
		fprintf(stderr, "Path: %s (ev %d)\n",path,Fl::event());

		if (stat(path,&buf)) return; // error
		if (S_ISDIR(buf.st_mode))  // directory
			loaddir(path);

		// Call opener
		//...
	}
}




/*-----------------------------------------------------------------
	GUI design
-------------------------------------------------------------------*/


int main (int argc, char **argv) {

fl_register_images();
edelib::IconTheme::init("crystalsvg");
	win = new Fl_Window(600, 400);
//	win->color(FL_WHITE);
	win->begin();
	view = new FileDetailsView(0,0,600,400,0);
	view->callback(open_cb);

	win->end();
	win->resizable(view);
//	win->icon(Icon::get("folder",Icon::TINY));
	win->show(argc,argv);

showhidden=false; dirsfirst=true; ignorecase=true;

	if (argc==1) { // No params
		loaddir ("");
	} else {
		loaddir (argv[1]);
	}

	return Fl::run();
}
