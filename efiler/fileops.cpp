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

// This file implements various operations with files (copy, delete...)
// NOT TO BE CONFUSED WITH edelib::File.h !!!
// Functions here usually call stuff from File.h, but also update
// efiler interface, display warnings to user etc.

#include "fileops.h"

#include <sys/stat.h>

#include <Fl/Fl_Window.H>
#include <Fl/Fl_Progress.H>
#include <Fl/Fl_Button.H>
#include <Fl/Fl_Menu_Button.H>
#include <Fl/filename.H>

#include <edelib/File.h>
#include <edelib/Directory.h>
#include <edelib/Nls.h>
#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/MimeType.h>


#include "EDE_FileView.h"
#include "Util.h"
#include "ede_ask.h" // replacement for fl_ask
#include "filesystem.h" // is_on_same_fs()


Fl_Progress* cut_copy_progress;
bool stop_now;
bool overwrite_all, skip_all;

enum OperationType_ { // Stop idiotic warnings from gcc
	CUT,
	COPY,
	ASK 
} operation = ASK;


// ----------------------------------------------
//    Some helper functions
// ----------------------------------------------

// is last character a / (avoid calling stat)?
bool my_isdir(const char* path) {
	return (path[strlen(path)-1]=='/');
}

// wrapper around fl_filename_name() that also works with directories
const char* my_filename_name(const char* path) {
	if (!my_isdir(path)) return fl_filename_name(path);

	static char buffer[FL_PATH_MAX];
	strncpy(buffer, path, FL_PATH_MAX);
	buffer[strlen(path)-1]='\0';
	return fl_filename_name(buffer);
}

// opposite to fl_filename_name(), returns the first part of path (a.k.a. directory)
const char* my_filename_dir(const char* path) {
	const char* name = my_filename_name(path);

	int pathlen = strlen(path)-strlen(name);
	if (pathlen>=FL_PATH_MAX) pathlen=FL_PATH_MAX-1;
 
	static char buffer[FL_PATH_MAX];
	strncpy(buffer, path, pathlen);
	buffer[pathlen]='\0';
	return buffer; // This is pointer to static buffer!! warning!
}



// Execute cut or copy operation
void do_cut_copy(bool m_copy) {
	int num = view->size();
	if (m_copy) operation = COPY; else operation = CUT;

	// Allocate buffer
	uint bufsize=10000;
	char* buf = (char*)malloc(sizeof(char)*bufsize);
	buf[0]='\0';

	// Add selected files to buffer and optionally grey icons (for cut effect)
	int nselected=0;
	for (int i=1; i<=num; i++) {
		view->ungray(i);
		if (view->selected(i)==1) {
			char* p = (char*)view->path(i);
			if (strncmp(p+strlen(p)-3, "/..", 3)==0) {
				// Can't cut/copy the Up button ("..")
				free(buf);
				return;
			}
			while (strlen(buf)+strlen(p)+8 >= bufsize) {
				bufsize+=10000;
				buf = (char*)realloc(buf,sizeof(char)*bufsize);
			}
			strcat(buf, "file://");
			strncat(buf,p,strlen(p));
			strcat(buf, "\r\n");
			if (operation == CUT) view->gray(i);
			nselected++;
		}
	}
	if (nselected==0) { //nothing selected, use the focused item
		int i=view->get_focus();
		char* p = (char*)view->path(i);
		// an individual path should never be longer > 10000 chars!
		strcat(buf, "file://");
		strncat(buf,p,strlen(p));
		strcat(buf, "\r\n");
		nselected=1;
	}

	Fl::copy(buf,strlen(buf),1); // use clipboard
	free(buf);

	// Deselect all and restore focus
	int focused = view->get_focus();
	for (int i=1; i<=num; i++)
		if (view->selected(i)) view->select(i,0);
	view->set_focus(focused);

	// Update statusbar
	if (m_copy)
		statusbar->copy_label(tsprintf(_("Selected %d items for copying"), nselected));
	else
		statusbar->copy_label(tsprintf(_("Selected %d items for moving"), nselected));
	// TODO: add total selected size? would require a stat on each file + expanding directories
}


// Copy single file. Returns true if operation should continue

// Note that at this point directories should be expanded into subdirectories etc.
// so when "copying" a directory we actually mean creating a new directory with same properties
bool my_copy(const char* src, const char* dest) {
	FILE *fold, *fnew;
	int c;

	// this shouldn't happen
	if (strcmp(src,dest)==0)
		return true;

	// This case is already checked in do_paste() so it shouldn't happen here
	if (edelib::file_exists(dest))
		return true;

	if (my_isdir(src)) {
		// try to preserve permissions
		// FIXME: this is not entirely cross-platform (due to different headers on *BSD)
		struct stat buf;
		stat(src,&buf);
		if (mkdir (dest, buf.st_mode)==0)
			return true; // success

		int q = ede_choice_alert(tsprintf(_("Cannot create directory %s (%d)"),dest,strerror(errno)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return false; else return true;
	}

	if ( !edelib::file_readable(src) ) {
		int q = ede_choice_alert(tsprintf(_("Cannot read file\n\t%s\n%s"),src,strerror(errno)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return false; else return true;
	}

	// edelib::file_writeable() returns false if dest doesn't exist
	if (edelib::file_exists(dest) && !edelib::file_writeable(dest)) {
		int q = ede_choice_alert(tsprintf(_("You don't have permission to overwrite file\n\t%s"),dest), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
		// this is redundant ATM cause dest is removed in do_paste()
	}

	if (!edelib::dir_exists(my_filename_dir(dest))) {
		// Shouldn't happen, unless someone is deleting stuff behind our back
		int q = ede_choice_alert(tsprintf(_("Directory\n\t%s\ndoesn't exist."), my_filename_dir(dest)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
	}

	if (!edelib::file_exists(dest) && !edelib::dir_writeable(my_filename_dir(dest))) {
		int q = ede_choice_alert(tsprintf(_("Cannot create file in directory\n\t%s\nYou don't have permission."), my_filename_dir(dest)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
	}

/*	// we don't use edelib::file_copy because we want to call Fl::check() periodically so user
	// can stop copying
	if (!edelib::file_copy(src,dest,true)) 
		fl_alert(tsprintf(_("Error copying %s to %s"),src,dest)); */


	// This should already be checked!
	if ( ( fold = fopen( src, "rb" ) ) == NULL ) {
		int q = ede_choice_alert(tsprintf(_("Cannot read file\n\t%s\n%s"), src, strerror(errno)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return false; else return true;
	}

	if ( ( fnew = fopen( dest, "wb" ) ) == NULL  )
	{
		fclose ( fold );
		int q = ede_choice_alert(tsprintf(_("Cannot create file\n\t%s\n%s"), dest, strerror(errno)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return false; else return true;
	}

	int count=0;
	while (!feof(fold)) {
		c = fgetc(fold);
		fputc(c, fnew);
		// Update interface
		if (count++ > 1000) {
			count=0;
			Fl::check();
		}

		if (stop_now) {
			ede_alert(tsprintf(_("Copying interrupted!\nFile %s is only half-copied and probably broken."), my_filename_name(dest)));
			break; 
		}
	}

	if (ferror(fold) || ferror(fnew)) {
		// This is probably a filesystem error (such as ejected disk or bad sector)

		fclose(fold); // don't flush error buffer before calling ferror
		fclose(fnew);
		int q;
		if (ferror(fold))
			q = ede_choice_alert(tsprintf(_("Error while reading file\n\t%s\n%s"), src, strerror(errno)), _("&Stop"), _("&Continue"), 0);
		else
			q = ede_choice_alert(tsprintf(_("Error while writing file\n\t%s\n%s"), dest, strerror(errno)), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return false; else return true;
	} else {
		fclose(fold);
		fclose(fnew);
	}

	// attempt to preserve permissions - if it fails, we don't care
	// FIXME: this is not entirely cross-platform (due to different headers on *BSD)
	struct stat buf;
	stat(src,&buf);
	chmod (dest, buf.st_mode);

	return true;
}


// Recursive function that creates a list of all files to be copied. All 
// directories are opened and all files and subdirectories etc. are added
// separately to the list. The total number of elements will be stored in 
// listsize. Returns false if user decided to interrupt copying.
bool expand_dirs(const char* src, char** &list, int &list_size, int &list_capacity) {
	// Grow list if neccessary
	if (list_size >= list_capacity-1) {
		list_capacity += 1000;
		list = (char**)realloc(list, sizeof(char**)*list_capacity);
	}
	// We add both files and directories to list
	list[list_size++] = strdup(src);

	if (my_isdir(src)) { // fl_filename_list makes sure that directories are appended with /
		char new_src[FL_PATH_MAX];
		dirent	**files;
		// FIXME: use same sort as used in view
		// FIXME: detect errors on accessing directory
		int num_files = fl_filename_list(src, &files, fl_casenumericsort);
		for (int i=0; i<num_files; i++) {
			if (strcmp(files[i]->d_name,"./")==0 || strcmp(files[i]->d_name,"../")==0) continue;
			snprintf(new_src, PATH_MAX, "%s%s", src, files[i]->d_name);
			Fl::check(); // update gui
			if (stop_now || !expand_dirs(new_src, list, list_size, list_capacity))
				return false;
		}
	}
	return true;
}



// Delete currently selected file(s) and directory(es)
void do_delete() {
	// TODO: Add a progress bar??? 
	// Delete is, in my experience, *very* fast (don't know why Konqueror takes so long)

	// Count all selected items, expanding directories as neccessary
	int list_size = 0, list_capacity = 1000;
	char** files_list = (char**)malloc(sizeof(char**)*list_capacity);

	for (int i=1; i<=view->size(); i++)
		if (view->selected(i)==1)
			expand_dirs(view->path(i), files_list, list_size, list_capacity);

	if (list_size==0) { //nothing selected, use the focused item
		int i=view->get_focus();
		expand_dirs(view->path(i), files_list, list_size, list_capacity);
	}

	// Issue a warning
	int c;
	if (list_size==1 && my_isdir(files_list[0]))
		c = ede_choice_alert(tsprintf(_("Are you sure that you want to delete directory\n\t%s\nincluding everything in it?"), files_list[0]), _("Do&n't delete"), _("&Delete"), 0);
	else if (list_size==1)
		c = ede_choice_alert(tsprintf(_("Are you sure that you want to delete file %s ?"), my_filename_name(files_list[0])), _("Do&n't delete"), _("&Delete"), 0);
	else
		c = ede_choice_alert(tsprintf(_("Are you sure that you want to delete %d files and directories?"), list_size), _("Do&n't delete"), _("&Delete"), 0);

	if (c==1) {
		// first remove files...
		for (int i=0; i<list_size; i++)
			if (!my_isdir(files_list[i])) 
				if (!edelib::file_remove(files_list[i]))
					ede_alert(tsprintf(_("Couldn't delete file\n\t%s\n%s"), files_list[i], strerror(errno)));

		// ...then directories
		// since expand_dirs() returns first dirs then files, we should go in oposite direction
		for (int i=list_size-1; i>=0; i--)
			if (my_isdir(files_list[i])) 
				if (!edelib::dir_remove(files_list[i]))
					ede_alert(tsprintf(_("Couldn't delete directory\n\t%s\n%s"), files_list[i], strerror(errno)));

		// refresh directory listing - optimized
		for (int i=1; i<=view->size(); i++)
			for (int j=0; j<list_size; j++)
				if (strcmp(files_list[j],view->path(i))==0)
					view->remove(i);
	}

	// Cleanup memory
	for (int i=0; i<list_size; i++) free(files_list[i]);
	free(files_list);
}


// Rename the file that has focus to the given name 'newname'
void do_rename(const char* newname) {
	int focus = view->get_focus();
	const char* oldname = fl_filename_name(view->path(focus)); // get filename

	char oldpath[FL_PATH_MAX], newpath[FL_PATH_MAX];
	snprintf(oldpath, FL_PATH_MAX-1, "%s%s", current_dir, oldname);
	snprintf(newpath, FL_PATH_MAX-1, "%s%s", current_dir, newname);
	
	if (edelib::file_exists(newpath))
		ede_alert(tsprintf(_("Filename already in use: %s"), newname));
	else if (!edelib::file_rename(oldpath,newpath))
		ede_alert(tsprintf(_("Rename %s to %s failed!"), oldname, newname));
	else 
		view->update_path(oldpath,newpath);
}


// Callback for Stop button on progress window (created in do_paste())
void stop_copying_cb(Fl_Widget*,void* v) {
	stop_now=true;
	// Let's inform user that we're stopping...
	Fl_Box* caption = (Fl_Box*)v;
	caption->label(_("Stopping..."));
	caption->redraw();
	caption->parent()->redraw();
}



// Execute paste - this will copy or move files based on chosen operation

// FIXME: if user cuts some files, then does dnd from another window,
// do_paste() will assume operation==CUT and won't ask the user!
// - Konqueror handles this by inventing a different mimetype for non-dnd buffer
// - Nautilus has the same bug!

void do_paste(const char* t) {
	char *to = (char*)t;
	if (!t || !fl_filename_isdir(t) || (strncmp(t+strlen(t)-3,"/..",3)==0))
		to = current_dir;

fprintf (stderr, "PASTE from '%s', to '%s', type=%d\n",(char*)Fl::event_text(),to,operation);

	if (!strchr(Fl::event_text(), '/'))
		return; // User is pasting something that isn't files
		// TODO: create a text file?

	// Tokenize event text into an array of strings ("from")
	char* tmp = (char*)Fl::event_text();
	int count=0;
	do count++; while (tmp && (tmp = strchr(tmp+1,'\n')));

	char** from = (char**)malloc(sizeof(char*) * count);
	tmp = (char*)Fl::event_text();
	char* tmp2;
	int k=0;
	do {
		tmp2 = strchr(tmp,'\n');
		int len=tmp2-tmp;
		if (!tmp2) len=strlen(tmp);
		if (len<2) { tmp=tmp2+1; count--; continue; }
		from[k] = (char*)malloc(sizeof(char) * (len+2));
		strncpy(from[k],tmp,len);
		from[k][len]='\0';
		if (from[k][len-1] == '\r') from[k][--len]='\0';
		// We accept both URIs (beginning with file://) and plain filename
		if (strncmp(from[k],"file://",7)==0)
			for (int i=0; i<=len-7; i++) 
				from[k][i]=from[k][i+7];

		// All directories must end in /
		if (fl_filename_isdir(from[k]) && (from[k][len-1] != '/')) {
			from[k][len++]='/';
			from[k][len]='\0';
		}

fprintf (stderr, "from[%d]='%s'\n", k, from[k]);
		k++;
		tmp=tmp2+1;
	} while (tmp2);

	// Some sanity checks
	for (int i=0; i<count; i++) {
		// If this window is below others, try to get focus
		win->take_focus();
		if (strcmp(to,from[i])==0) {
			//ede_alert(tsprintf(_("Can't copy directory\n\t%s\ninto itself."), to));

			// This usually happens accidentally, so statusbar is less annoying
			statusbar->copy_label(tsprintf(_("Can't copy directory %s into itself."), my_filename_name(to)));
			goto FINISH;
		}
		if ((strncmp(to,from[i],strlen(to))==0)) {
			char *k = strchr(from[i]+strlen(to), '/');
			if (!k || *(k+1)=='\0') {
				//ede_alert(tsprintf(_("File %s is already in directory\n\t%s"), from[i]+strlen(to), to));
				statusbar->copy_label(tsprintf(_("File %s is already in directory %s"), from[i]+strlen(to), to));
				goto FINISH;
			}
		}
	}


	// Resolve operation
	if (operation == ASK) {
		// TODO make a popup menu just like in Konqueror
/*		fprintf(stderr, "x=%d y=%d\n", Fl::event_x_root(),Fl::event_y_root());
		Fl_Menu_Button* mb = new Fl_Menu_Button (Fl::event_x_root(),Fl::event_y_root(),0,0);
		mb->box(FL_NO_BOX);
		mb->type(Fl_Menu_Button::POPUP123);
		mb->add(_("&Copy"),0,cb_dnd_copy);
		mb->add(_("_&Move"),0,cb_dnd_cut);
		mb->add(_("C&ancel"),0,0);
		mb->popup();
		goto FINISH;*/

		int c;
		if (count==1 && my_isdir(from[0])) 
			c = ede_choice_alert(tsprintf(_("Copy or move directory\n\t%s\nto directory\n\t%s ?"), from[0], to), _("C&ancel"), _("&Copy"), _("&Move"));
		else if (count==1)
			c = ede_choice_alert(tsprintf(_("Copy or move file %s to directory\n\t%s ?"), fl_filename_name(from[0]), to), _("C&ancel"), _("&Copy"), _("&Move"));
		else
			c = ede_choice_alert(tsprintf(_("Copy or move these %d files to directory\n\t%s ?"), count, to), _("C&ancel"), _("&Copy"), _("&Move"));

		if (c==0) goto FINISH;
		if (c==1) operation=COPY; else operation=CUT;
	}


	{ // to silence goto

	overwrite_all=false; skip_all=false;
	stop_now=false;

	// srcdir is root directory of from[] array
	char *srcdir = strdup(my_filename_dir(from[0]));
	if (strcmp(srcdir,to)==0) {
		// This should never happen cause we already checked it...
		ede_alert(_("You cannot copy a file onto itself!"));
		free(srcdir);
		goto FINISH;
	}

	// Draw progress dialog
	Fl_Window* progress_window = new Fl_Window(350,150);
	if (operation == COPY)
		progress_window->label(_("Copying files"));
	else
		progress_window->label(_("Moving files"));

	progress_window->set_modal();
	progress_window->begin();
	Fl_Box* caption = new Fl_Box(20,20,310,25, _("Counting files in directories"));
	caption->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	cut_copy_progress = new Fl_Progress(20,60,310,20);
	Fl_Button* stop_button = new Fl_Button(145,100,60,40, _("&Stop"));
	stop_button->callback(stop_copying_cb,caption);
	progress_window->end();
	progress_window->show();

	// Set ProgressBar range
	cut_copy_progress->minimum(0);
	cut_copy_progress->maximum(count);
	cut_copy_progress->value(0);

	// Count files in directories
	int list_size = 0, list_capacity = 1000;
	char** files_list = (char**)malloc(sizeof(char**)*list_capacity);
	for (int i=0; i<count; i++) {
		if (!stop_now) expand_dirs(from[i], files_list, list_size, list_capacity);
		cut_copy_progress->value(i+1);
		Fl::check(); // check to see if user pressed Stop
	}

	if (stop_now) { // user pressed stop while counting, cleanup and exit
		for (int i=0; i<list_size; i++) free(files_list[i]);
		free(files_list);
		goto FINISH;
	}

	// Now copying those files
	cut_copy_progress->minimum(0);
	cut_copy_progress->maximum(list_size);
	cut_copy_progress->value(0);
	char dest[FL_PATH_MAX];


	// This list is used for updating mimetypes after the copying is finished
	FileItem **item_list=0; 
	int item_list_size=0;
	if (strncmp(to,current_dir,strlen(to))==0)
		// avoid malloc if current dir isn't inside the scope of paste
		item_list = new FileItem*[list_size];


	for (int i=0; i<list_size; i++) {

		// Prepare dest filename
		char *srcfile = files_list[i] + strlen(srcdir);
		snprintf (dest, PATH_MAX, "%s%s", to, srcfile);
		char *src = files_list[i]; // shortcut

		if (operation == COPY)
			caption->copy_label(tsprintf(_("Copying %d of %d files to %s"), i, list_size, to));
		else
			caption->copy_label(tsprintf(_("Moving %d of %d files to %s"), i, list_size, to));
		caption->redraw();

		if (edelib::file_exists(dest)) {

			// if both src and dest are directories, do nothing
			if (my_isdir(src) && my_isdir(dest))
				continue;

			// copy file over directory
			// TODO: we will just skip this case because it's "impossible",
			// but maybe there should be another warning
			if (my_isdir(dest))
				continue;
	
			// copy directory over file
			if (my_isdir(src)) {
				int q = ede_choice_alert(tsprintf(_("You're trying to copy directory\n\t%s\nbut there is already a file with this name. What to do?"),dest), _("&Stop"), _("S&kip directory"), _("&Delete file"), 0);
				if (q == 0) break; 
				else if (q == 1) continue;
				// else q==2 (delete file)

			// copy file over file
			} else {
				int c = -1;
				if (!overwrite_all && !skip_all) 
					c = ede_choice_alert(tsprintf(_("File with name\n\t%s\nalready exists. What to do?"), dest), _("&Overwrite"), _("Over&write all"), _("&Skip"), _("Skip &all"));

				if (c==1) overwrite_all=true;
				if (c==3) skip_all=true;
				if (c==2 || skip_all) continue; // go to next entry

				// At this point c==0 (Overwrite) or overwrite_all == true
			}

			if (!edelib::file_remove(dest)) {
				int q = ede_choice_alert(tsprintf(_("Couldn't remove file\n\t%s"),dest), _("&Stop"), _("&Continue"), 0);
				if (q==0) break;
				else continue;
			}

		// Update interface - add file to list (if we are viewing the destination directory)
		} else if (strcmp(current_dir,my_filename_dir(dest))==0) {
			FileItem *item = new FileItem;
			item->name = my_filename_name(dest);
			item->realpath = dest;
			if (my_isdir(dest)) {
				item->icon = "folder";
				item->description = "Directory";
				// item->name += "/";
			} else {
				item->icon = "unknown";
				item->description = "Unknown";
			}
			item_list[item_list_size++] = item;
			view->add(item); // don't bother with sorting, that would be too complex
		}

		// Moving on same filesystem is actually rename
		if (operation == CUT && is_on_same_fs(to, from[0]))
			edelib::file_rename(src,dest);
			// this is extremely fast, user won't have time to click on stop ;)

		else {
			if (stop_now || !my_copy(src, dest))
				break;
			// Delete file after moving
			if (operation == CUT) edelib::file_remove(src);
			cut_copy_progress->value(cut_copy_progress->value()+1);
			Fl::check(); // check to see if user pressed Stop
		}
	}
	progress_window->hide();

	// Cleanup memory
	for (int i=0; i<list_size; i++) free(files_list[i]);
	free(files_list);
	free(srcdir);

	if (stop_now) { // User pressed Stop but we don't know when, so we'll just reload
		loaddir(current_dir);
		goto FINISH;
	}

	// -- Update interface in a smart way

	// Remove cutted files
	if (operation == CUT)
		for (int i=0; i<count; i++)
			for (int j=1; j<=view->size(); j++)
				if (strcmp(from[i],view->path(j))==0)
					view->remove(j);

	// Update mimetypes for pasted files
	for (int i=0; i<item_list_size; i++) {
		struct stat buf;
		if (stat(from[i],&buf)) { delete item_list[i]; continue; }
		FileItem *item = item_list[i];
		item->date = nice_time(buf.st_mtime);
		item->permissions = ""; // todo
		if (item->description != "Directory") {
			item->size = nice_size(buf.st_size);
			edelib::MimeType mt;
			mt.set(from[i]);
			edelib::String desc,icon;
			desc = mt.comment();
			// First letter of desc should be upper case:
			if (desc.length()>0 && desc[0]>='a' && desc[0]<='z') desc[0] = desc[0]-'a'+'A';
			icon = mt.icon_name();
			if (desc!="" || icon!="") {
				if (desc != "") item->description = desc;
				if (icon != "") item->icon = icon;
			}
			Fl::check();
		}
		view->update(item);
		delete item_list[i];
	}
	delete[] item_list;

	// Select the just pasted files (they're at the bottom)
	if (item_list_size>0) {
		view->redraw();
		for (int i=view->size(),j=0; j<item_list_size; i--,j++)
			view->select(i,1);
	}

	} // scoping to silence goto

	// Cleanup memory and exit
FINISH:
	for (int i=0; i<count; i++) free(from[i]);
	free(from);

	// Set operation for future dnd; do_cut_copy() will change this if
	// classic cut or copy is used
	operation=ASK;
}
