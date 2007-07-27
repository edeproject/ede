
// This file implements copy / move / delete operation with files
// NOT TO BE CONFUSED WITH edelib::File.h !!!
// Functions here usually call stuff from File.h, but also update
// efiler interface, display warnings to user etc.

#include "fileops.h"

#include <sys/stat.h>

#include <Fl/Fl_Window.H>
#include <Fl/Fl_Progress.H>
#include <Fl/Fl_Button.H>
#include <Fl/filename.H>
#include <Fl/fl_ask.H>

#include <edelib/File.h>
#include <edelib/Nls.h>


#include "EDE_FileView.h"
#include "Util.h"


Fl_Progress* cut_copy_progress;
bool stop_now;
bool overwrite_all, skip_all;

char **cut_copy_buffer = 0;
bool operation_is_copy = false;



// Execute cut or copy operation when List View is active
void do_cut_copy(bool m_copy) {

	// Count selected icons, for malloc
	int num = view->size();
	int nselected = 0;
	for (int i=1; i<=num; i++)
		if (view->selected(i)) {
			nselected++;

			// Can not cut/copy the up directory button (..)
			const char* tmp = view->text(i);
			if (tmp[0]=='.' && tmp[1]=='.' && (tmp[2]=='\0' || tmp[2]==view->column_char())) return;
		}

	// Clear cut/copy buffer and optionally ungray the previously cutted icons
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=1; i<=num; i++)
				view->ungray(i);
		}
	}

	// Allocate buffer
	cut_copy_buffer = (char**)malloc(sizeof(char*) * (nselected+2));

	// Add selected files to buffer and optionally grey icons (for cut effect)
	int buf=0;
	for (int i=1; i<=num; i++) {
		if (view->selected(i)==1) {
			cut_copy_buffer[buf] = strdup((char*)view->data(i));
			if (!m_copy) view->gray(i);
			buf++;
		}
	}
	if (buf==0) { //nothing selected, use the focused item
		int i=view->get_focus();
		cut_copy_buffer[buf] = strdup((char*)view->data(i));
		if (!m_copy) view->gray(i);
		buf++;
		nselected=1;
	}

	cut_copy_buffer[buf] = 0;
	operation_is_copy = m_copy;

	// Deselect all
	int focused = view->get_focus();
	for (int i=1; i<=num; i++)
		if (view->selected(i)) view->select(i,0);
	view->set_focus(focused);

	// Update statusbar
	if (m_copy)
		statusbar->label(tsprintf(_("Selected %d items for copying"), nselected));
	else
		statusbar->label(tsprintf(_("Selected %d items for moving"), nselected));
}


// Helper functions for paste:

// Copy single file. Returns true if operation should continue
// Note that at this point directories should be expanded into subdirectories etc.
// so when "copying" a directory we actually mean creating a new directory with same info
bool my_copy(const char* src, const char* dest) {
	FILE *fold, *fnew;
	int c;

	if (strcmp(src,dest)==0)
		// this shouldn't happen
		return true;

	if (edelib::file_exists(dest)) {
		// if both src and dest are directories, do nothing
		if (fl_filename_isdir(src) && fl_filename_isdir(dest))
			return true;

		int c = -1;
		if (!overwrite_all && !skip_all) {
			// here was choice_alert
			c = fl_choice(tsprintf(_("File already exists: %s. What to do?"), dest), _("&Overwrite"), _("Over&write all"), _("&Skip"), _("Skip &all"), 0); // asterisk (*) means default
		}
		if (c==1) overwrite_all=true;
		if (c==3) skip_all=true;
		if (c==2 || skip_all) {
			return true;
		}
		// At this point either c==0 (overwrite) or overwrite_all == true

		// copy directory over file
		if (fl_filename_isdir(src))
			unlink(dest);

		// copy file over directory
		// TODO: we will just skip this case, but ideally there should be
		// another warning
		if (fl_filename_isdir(dest))
			return true;
	}

	if (fl_filename_isdir(src)) {
		if (mkdir (dest, umask(0))==0)
			return true; // success
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot create directory %s"),dest), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
	}

	if ( !edelib::file_readable(src) ) {
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot read file %s"),src), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
	}

// edelib::file_writeable() returns false if dest doesn't exist
/*	if ( !edelib::file_writeable(dest)  )
	{
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot create file %s"),dest), _("&Stop"), _("&Continue"), 0);
		if (q == 0) return true; else return false;
	}*/

	// we will try to preserve permissions etc. cause that's usually what people want
	if (!edelib::file_copy(src,dest,true)) 
		fl_alert(tsprintf(_("Error copying %s to %s"),src,dest));

//	fclose(fold);
//	fclose(fnew);
	return true;
}


// Recursive function that creates a list of all files to be copied, such that 
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

	if (fl_filename_isdir(src)) {
		char new_src[PATH_MAX];
		dirent	**files;
		// FIXME: use same sort as used in view
		// FIXME: detect errors on accessing folder
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


// Callback for Stop button on progress window
void stop_copying_cb(Fl_Widget*,void* v) {
	stop_now=true;
	// Let's inform user that we're stopping...
	Fl_Box* caption = (Fl_Box*)v;
	caption->label(_("Stopping..."));
	caption->redraw();
	caption->parent()->redraw();
}

// Execute paste operation - this will copy or move files based on chosen
// operation
void do_paste() {

	if (!cut_copy_buffer || !cut_copy_buffer[0]) return;

	overwrite_all=false; skip_all=false;

	// Moving files on same filesystem is trivial, just like rename
	// We don't even need a progress bar
	if (!operation_is_copy && is_on_same_fs(current_dir, cut_copy_buffer[0])) {
		for (int i=0; cut_copy_buffer[i]; i++) {
			char *newname;
			asprintf(&newname, "%s%s", current_dir, fl_filename_name(cut_copy_buffer[i]));
			if (edelib::file_exists(newname)) {
				int c = -1;
				if (!overwrite_all && !skip_all) {
					// here was choice_alert
					c = fl_choice(tsprintf(_("File already exists: %s. What to do?"), newname), _("&Overwrite"), _("Over&write all"), _("&Skip"), _("Skip &all")); // * means default
				}
				if (c==1) overwrite_all=true;
				if (c==3) skip_all=true;
				if (c==2 || skip_all) {
					free(cut_copy_buffer[i]);
					continue; // go to next entry
				}
				// At this point c==0 (Overwrite) or overwrite_all == true
				unlink(newname);
			} 
			rename(cut_copy_buffer[i],newname);
			free(cut_copy_buffer[i]);
			free(newname);
		}
		free(cut_copy_buffer);
		cut_copy_buffer=0;
		loaddir(current_dir); // Update display
		return;

	//
	// Real file moving / copying using recursive algorithm
	//

	} else {
		stop_now = false;

		// Create srcdir string
		char *srcdir = strdup(cut_copy_buffer[0]);
		char *p = strrchr(srcdir,'/');
		if (*(p+1) == '\0') { // slash is last - find one before
			*p = '\0';
			p = strrchr(srcdir,'/');
		}
		*(p+1) = '\0';

		if (strcmp(srcdir,current_dir)==0) {
			fl_alert(_("You cannot copy a file onto itself!"));
			free(srcdir);
			return;
		}

		// Draw progress dialog
		Fl_Window* progress_window = new Fl_Window(350,150);
		if (operation_is_copy)
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

		// How many items do we copy?
		int copy_items=0;
		for (; cut_copy_buffer[copy_items]; copy_items++);
		// Set ProgressBar range accordingly
		cut_copy_progress->minimum(0);
		cut_copy_progress->maximum(copy_items);
		cut_copy_progress->value(0);

		// Count files in directories
		int list_size = 0, list_capacity = 1000;
		char** files_list = (char**)malloc(sizeof(char**)*list_capacity);
		for (int i=0; i<copy_items; i++) {
			// We must ensure that cut_copy_buffer is deallocated
			// even if user clicked on Stop
			if (!stop_now) expand_dirs(cut_copy_buffer[i], files_list, list_size, list_capacity);
			free(cut_copy_buffer[i]);
			cut_copy_progress->value(i+1);
			Fl::check(); // check to see if user pressed Stop
		}
		free (cut_copy_buffer);
		cut_copy_buffer=0;

		// Now copying those files
		if (!stop_now) {
			char label[150];
			char dest[PATH_MAX];
			cut_copy_progress->minimum(0);
			cut_copy_progress->maximum(list_size);
			cut_copy_progress->value(0);

			for (int i=0; i<list_size; i++) {
				// Prepare dest filename
				char *srcfile = files_list[i] + strlen(srcdir);
				snprintf (dest, PATH_MAX, "%s%s", current_dir, srcfile);

				if (operation_is_copy)
					snprintf(label, 150, _("Copying %d of %d files to %s"), i, list_size, current_dir);
				else
					snprintf(label, 150, _("Moving %d of %d files to %s"), i, list_size, current_dir);
				caption->label(label);
				caption->redraw();
				if (stop_now || !my_copy(files_list[i], dest))
					break;
				// Delete file after moving
				if (!operation_is_copy) unlink(files_list[i]);
				cut_copy_progress->value(cut_copy_progress->value()+1);
				Fl::check(); // check to see if user pressed Stop
			}
		}
		progress_window->hide();

		// Reload current dir
		loaddir(current_dir);

		// select the just pasted files and cleanup memory
		for (int i=0; i<list_size; i++) { 
			char* tmp = strrchr(files_list[i],'/')+1;
//			if (!tmp) { fprintf (stderr, "not found\n"); continue; }
//			tmp++;
			for (int j=1; j<=view->size(); j++) 
				if (strncmp(tmp, view->text(j), strlen(tmp))==0)
					view->select(j,1);
			free(files_list[i]);
		}

		free(files_list);
		free(srcdir);
	}
}


// Delete currently selected file(s) or directory(es)
void do_delete() {
	// Count all selected item, expanding directories as neccessary
	int list_size = 0, list_capacity = 1000;
	char** files_list = (char**)malloc(sizeof(char**)*list_capacity);

	for (int i=1; i<=view->size(); i++)
		if (view->selected(i)==1)
			expand_dirs((char*)view->data(i), files_list, list_size, list_capacity);

	if (list_size==0) { //nothing selected, use the focused item
		int i=view->get_focus();
		expand_dirs((char*)view->data(i), files_list, list_size, list_capacity);
	}

	// Issue a warning
		// here was choice_alert
	int c;
	if (list_size==1) {
		char* k=strrchr(files_list[0],'/');
		c = fl_choice(tsprintf(_("Are you sure that you want to delete file %s ?"), k+1), _("Do&n't delete"), _("&Delete"), 0);
	} else
		c = fl_choice(tsprintf(_("Are you sure that you want to delete %d files?"), list_size), _("Do&n't delete"), _("&Delete"), 0);
	if (c!=1) return;

	// delete
	for (int i=0; i<list_size; i++) 
		edelib::file_remove(files_list[i]);
	// loaddir(current_dir); - optimized
	for (int i=1; i<=view->size(); i++)
		if (view->selected(i)==1)
			view->remove(i);
}


// Rename the file with focus to given name
void do_rename(const char* c) {
	edelib::String oldname(current_dir);
	edelib::String newname(current_dir);
	int focus = view->get_focus();
	edelib::String vline(view->text(focus)); // get filename

	oldname += vline.substr(0,vline.find(view->column_char(),0));
	newname += c;
	
	if (edelib::file_exists(newname.c_str()))
		fl_alert(tsprintf(_("Filename already in use: %s"), newname.c_str()));
// For some reason edelib::file_rename() always fails
//	else if (!edelib::file_rename(oldname.c_str(),newname.c_str()))
//		fl_alert(tsprintf(_("Rename %s to %s failed!"), oldname.c_str(), newname.c_str()));
	else {
		rename(oldname.c_str(),newname.c_str());

		// Insert new name into vline
		int i=0,j=0;
		while (i != edelib::String::npos) { j=i; i=newname.find('/',j+1); }
		
		vline = newname.substr(j+1) + vline.substr(vline.find(view->column_char(),0));
		view->text(focus,vline.c_str());
	}

}

// Drag & drop callback - mostly copied from do_cut_copy()
void dnd_cb(const char* from,const char* to) {
	fprintf (stderr, "PASTE from '%s', to '%s'\n",from,to);
	return;

	char *t = (char*)to;
	if (!fl_filename_isdir(to))
		t=current_dir;

	
/*
	// Clear cut/copy buffer and optionally ungray the previously cutted icons
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=1; i<=num; i++)
				view->ungray(i);
		}
	}

	// Allocate buffer
	cut_copy_buffer = (char**)malloc(sizeof(char*) * (nselected+2));

	// Add selected files to buffer
	int buf=0;
	for (int i=1; i<=num; i++)
		if (view->selected(i)==1)
			cut_copy_buffer[buf++] = strdup((char*)view->data(i));
			// We don't know yet if this is cut or copy, so no need to gray anything

	// 

	// Clear cut/copy buffer and optionally ungray the previously cutted icons
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=1; i<=num; i++)
				view->ungray(i);
		}
	}
	
	
	int c = fl_choice(tsprintf(_("Do you want to copy or move file %s to directory %s?"), k+1), _("Do&n't delete"), _("&Delete"), 0);*/
}
