//
// "$Id$"
//
// FileBrowser routines.
//
// Copyright 1999-2006 by Michael Sweet.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//
// Contents:
//
//   FileBrowser::full_height()     - Return the height of the list.
//   FileBrowser::item_height()     - Return the height of a list item.
//   FileBrowser::item_width()      - Return the width of a list item.
//   FileBrowser::item_draw()       - Draw a list item.
//   FileBrowser::FileBrowser() - Create a FileBrowser widget.
//   FileBrowser::load()            - Load a directory into the browser.
//   FileBrowser::filter()          - Set the filename filter.
//

//
// Include necessary header files...
//

#include "EDE_FileBrowser.h"

#include <fltk/Browser.h>
#include <fltk/Item.h>
#include <fltk/draw.h>
#include <fltk/Color.h>
#include <fltk/Flags.h>
#include <fltk/Font.h>
#include <fltk/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __CYGWIN__
#  include <mntent.h>
#elif defined(WIN32)
#  include <windows.h>
#  include <direct.h>
// Apparently Borland C++ defines DIRECTORY in <direct.h>, which
// interfers with the FileIcon enumeration of the same name.
#  ifdef DIRECTORY
#    undef DIRECTORY
#  endif // DIRECTORY
#endif // __CYGWIN__

#ifdef __EMX__
#  define  INCL_DOS
#  define  INCL_DOSMISC
#  include <os2.h>
#endif // __EMX__

// CodeWarrior (__MWERKS__) gets its include paths confused, so we
// temporarily disable this...
#if defined(__APPLE__) && !defined(__MWERKS__)
#  include <sys/param.h>
#  include <sys/ucred.h>
#  include <sys/mount.h>
#endif // __APPLE__ && !__MWERKS__

#include "../edelib2/Icon.h"
#include "../edelib2/Util.h"
#include "../edelib2/MimeType.h"
#include "../edelib2/NLS.h"

#define DEFAULT_ICON "misc-vedran"
#define FOLDER_ICON "folder"
#define UPDIR_ICON "undo"

#include <fltk/run.h>

#include <fltk/Input.h>
#include <fltk/events.h>
#include <fltk/ask.h>

#include <errno.h>

using namespace fltk;
using namespace edelib;



// Event handler for EditBox
int EditBox::handle(int event) {
	if (!this->visible()) return parent()->handle(event);
	bool above=false;
	//fprintf(stderr,"Editbox event: %d (%s)\n",event,event_name(event));

	// Change filename
	if (event==KEY && (event_key()==ReturnKey || event_key()==KeypadEnter)) {
		// split old filename to path and file
		char path[PATH_MAX], file[PATH_MAX];
		strcpy(path, (char*)editing_->user_data());
		if (path[strlen(path)-1] == '/') path[strlen(path)-1]='\0';
		char *p = strrchr(path,'/');
		if (p==0 || *p=='\0') {
			strcpy(file,path);
			path[0]='\0';
		} else { // usual case
			p++;
			strcpy(file,p);
			*p='\0';
		}

		if (strlen(file)!=strlen(text()) || strcmp(file,text())!=0) {
			// Create new filename
			strncat(path, text(), PATH_MAX-strlen(path));
			char oldname[PATH_MAX];
			strcpy(oldname, (char*)editing_->user_data());
			if (rename(oldname, path) == -1) {
				alert(tsprintf(_("Could not rename file! Error was:\n\t%s"), strerror(errno)));
			} else {
				// Update browser
				free(editing_->user_data());
				editing_->user_data(strdup(path));
				const char* l = editing_->label();
				editing_->label(tasprintf("%s%s",text(),strchr(l, '\t')));
			}
		}
		
		above=true;
	}

	// Hide editbox 
	if (above || ( event==KEY && event_key()==EscapeKey ) ) {
		this->hide();
		return 1;
	}
	Input::handle(event);
}


// We override hide method to ensure certain things done
void EditBox::hide() {
	Input::hide();
	// Remove box so it doesn't get in the way
	this->x(0);
	this->y(0);
	this->w(0);
	this->h(0);
	// Return the browser item into "visible" state
	if (editing_) {
		editing_->textcolor(textcolor());
		editing_->redraw();
		parent()->take_focus();
	}
}



// Column widths and titles
// TODO: make more configurable

const char *labels[] = {_("Name"),_("Type"),_("Size"),_("Date"),0};
int widths[]   = {200, 150, 100, 150, 0};


//
// 'FileBrowser::FileBrowser()' - Create a FileBrowser widget.
//


FileBrowser::FileBrowser(int        X,  // I - Upper-lefthand X coordinate
			int        Y,  // I - Upper-lefthand Y coordinate
			int        W,  // I - Width in pixels
			int        H,  // I - Height in pixels
			const char *l)	// I - Label text
 : Browser(X, Y, W, H, l) {
	// Initialize the filter pattern, current directory, and icon size...
	pattern_   = "*";
	directory_ = "";
	//icon_size_  = 12.0f;
	filetype_  = BOTH;
	show_hidden_ = false;
	show_dotdot_ = true;
	column_labels(labels);
	column_widths(widths);

	// Editbox
	editbox_ = new EditBox (0, 0, 0, 0);
	editbox_->box(BORDER_FRAME);
	editbox_->parent(this);
	editbox_->hide();
}


//
// 'FileBrowser::load()' - Load a directory into the browser.
//

int						// O - Number of files loaded
FileBrowser::load(const char     *directory,// I - Directory to load
                      File_Sort_F *sort)	// I - Sort function to use
{
	int		i;				// Looping var
	int		num_files;			// Number of files in directory
	int		num_dirs;			// Number of directories in list
	char		filename[PATH_MAX];		// Current file
	//FileIcon	*icon;				// Icon to use


//  printf("FileBrowser::load(\"%s\")\n", directory);

	if (!directory)
		return (0);

	clear();
	directory_ = directory;

	if (directory_[0] == '\0')
	{
		//
		// No directory specified; for UNIX list all mount points.  For DOS
		// list all valid drive letters...
		//
		
		// TODO!
		fprintf (stderr, "Drive list not implemented yet");
		return 0;
	}

	// Scan directory and store list in **files
	dirent	**files;
	num_files = fltk::filename_list(directory_, &files, sort);
	if (num_files <= 0) return (0);

	// Allocate array for icons
	Item** icon_array = (Item**) malloc (sizeof(Item*) * num_files + 1);
	// fill array with zeros, for easier detection if item exists
	for (i=0; i<num_files; i++) icon_array[i]=0;

	// Show the up directory - "../"
	num_dirs = 0;
	if (show_dotdot_ && strcmp(directory_,"/") != 0) {
		num_dirs++;
		Item* o = new Item ( Icon::get ( UPDIR_ICON, Icon::TINY ), "..\tGo up" );
		Menu::add(*o);
		snprintf(filename, PATH_MAX, "%s../", directory_);
		o->user_data(strdup(filename));
	}

	// Main loop for populating browser
	for (i = 0; i < num_files; i ++) {
		if (strcmp(files[i]->d_name, "./")==0)
			continue;

		char *n = files[i]->d_name; // shorter

		snprintf(filename, PATH_MAX, "%s%s", directory_, n);

		if (strcmp(n, ".")==0 || strcmp(n, "./")==0 || (!show_hidden_ &&  (n[0]=='.' || n[strlen(n)-1]=='~') ) ) 
			continue;
		fltk::check(); //update interface

		// Add directory
		if (filetype_ != FILES && fltk::filename_isdir(filename)) {
			num_dirs ++;

			// strip slash from filename
			char *fn = strdup(n);
			if (fn[strlen(fn)-1] == '/')
				fn[strlen(fn)-1] = '\0';

			Item* o = new Item ( Icon::get ( FOLDER_ICON,Icon::TINY ), fn);
			Menu::insert(*o, num_dirs-1);
			o->user_data(strdup(filename)); // we keep full path for callback
			icon_array[i]=o;

		// Add file
		} else if (filetype_ != DIRECTORIES && fltk::filename_match(n, pattern_)) {
			Item* o = new Item(Icon::get(DEFAULT_ICON,Icon::TINY), strdup(n));
			Menu::add(*o);
			o->user_data(strdup(filename)); // we keep full path for callback
			icon_array[i]=o;
		}
	} // end for

	this->redraw();

	//
	// Detect icon mimetypes etc.
	//

	MimeType *m = new MimeType();

	for (i=0; i<num_files; i++) {
		// ignored files
		if (!icon_array[i]) continue;
		fltk::check(); // update interface

		// get mime data
		snprintf (filename,4095,"%s%s",directory_,files[i]->d_name);
		m->set(filename);

		// change label to complete data in various tabs
		char *label;
		if (strncmp(m->id(),"directory",9)==0) {
			// Strip slash from filename
			char *n = strdup(files[i]->d_name);
			n[strlen(n)-1] = '\0';
			asprintf(&label, "%s\t%s\t\t%s", n, m->type_string(), nice_time(filename_mtime(filename)));
			free(n);
		} else 
			asprintf(&label, "%s\t%s\t%s\t%s", files[i]->d_name, m->type_string(), nice_size(filename_size(filename)), nice_time(filename_mtime(filename)));
		icon_array[i]->label(label);

		// icon
		icon_array[i]->image(m->icon(Icon::TINY));

		icon_array[i]->redraw();
		free(files[i]);
	}
	delete m;
	free(files);

	return (num_files);
}


//
// 'FileBrowser::filter()' - Set the filename filter.
//
// I - Pattern string
void FileBrowser::filter(const char *pattern)	{
	// If pattern is NULL set the pattern to "*"...
	if (pattern) pattern_ = pattern;
	else pattern_ = "*";
}



// We override the fltk standard event handling to detect when 
// user is clicking on already selected item and show filename editbox

int FileBrowser::handle(int event) {
	const int iconspace=20;

	// Handle all events in editbox
	if (editbox_->visible()) {
		if (event == PUSH && !event_inside(Rectangle(editbox_->x(), editbox_->y(), editbox_->w(), editbox_->h())))
			editbox_->hide();
		else
			return editbox_->handle(event);
	}

	if (event==PUSH && !event_clicks() && 
	// Don't accept clicks outside first column:
	event_x()<widths[0])
		for(int i=0; i<children()-1; i++)
			if (event_y()>child(i)->y() && 
			// Handle last child
			(i==children()-1 || event_y()<child(i+1)->y()) && 
			child(i)->flags()&SELECTED && 
			// Make sure only one item is selected:
			child(i)==item() &&
			// Can't rename "up directory"
			strcmp(child(i)->label(),"../") != 0) {
				// "hide" item
				editbox_->textcolor(child(i)->textcolor());
				child(i)->textcolor(child(i)->color());
				child(i)->throw_focus();

				// deselect all
				set_item_selected(false);
				//deselect();

				// Show editbox at item coordinates
				editbox_->x(this->x()+child(i)->x()+iconspace);
				editbox_->y(child(i)->y());
				editbox_->w(150);
				editbox_->h(20);
				editbox_->show();

				// Copy last part of path to editbox
				char*p = strdup((char*)child(i)->user_data());
				if (p[strlen(p)-1] == '/') p[strlen(p)-1]='\0';
				char*q = strrchr(p,'/');
				if (q!=0)
					editbox_->text(q+1);
				else
					editbox_->text(p);
				free(p);
				/*char*p = strdup((char*)this->user_data());
				editbox_->text(filename_name(p));*/

				editbox_->take_focus();
				editbox_->editing(child(i));
				return 0;
			}
	return Browser::handle(event);
}


//
// End of "$Id$".
//
