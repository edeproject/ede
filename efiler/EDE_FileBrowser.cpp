//
// "$Id: FileBrowser.cxx 5071 2006-05-02 21:57:08Z fabien $"
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

#define DEFAULT_ICON "misc-vedran"
#define FOLDER_ICON "folder"
#include <fltk/run.h>


using namespace fltk;

//
// 'FileBrowser::FileBrowser()' - Create a FileBrowser widget.
//

  const char *labels[] = {"Name","Type","Size","Date",0};
  int widths[]   = {200, 150, 100, 150, 0};

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
  //filetype_  = FILES;
  show_hidden_ = false;
  column_labels(labels);
  column_widths(widths);

  //Symbol* fileSmall = edelib::Icon::get(DEFAULT_ICON,edelib::Icon::SMALL);
  //set_symbol(Browser::LEAF, fileSmall, fileSmall);
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
  char		filename[4096];			// Current file
  FileIcon	*icon;				// Icon to use


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

  }
  else
  {
    dirent	**files;	// Files in in directory


    //
    // Build the file list...
    //

    num_files = fltk::filename_list(directory_, &files, sort);

    if (num_files <= 0)
      return (0);

    Item** icon_array = (Item**) malloc (sizeof(Item*) * num_files + 1);
// fill array with zeros, for easier detection if item exists
	for (i=0; i<num_files; i++) icon_array[i]=0;

    for (i = 0, num_dirs = 0; i < num_files; i ++) {
      if (strcmp(files[i]->d_name, "./") ) {
	snprintf(filename, sizeof(filename), "%s%s", directory_,
	         files[i]->d_name);

        //bool ft = true;	if (ft) {FileIcon::load_system_icons(); ft=false;}

        //icon = FileIcon::find(filename);
	//printf("%s\n",files[i]->d_name);
	if (!strcmp(files[i]->d_name, ".") || !strcmp(files[i]->d_name, "./") || 
	    !show_hidden_ &&  files[i]->d_name[0]=='.' &&  strncmp(files[i]->d_name,"../",2)) 
	  continue;

	if (fltk::filename_isdir(filename)) {
          num_dirs ++;
		Item* o = new Item(edelib::Icon::get(FOLDER_ICON,edelib::Icon::TINY), strdup(files[i]->d_name));
		Menu::insert(*o, num_dirs-1);
		o->user_data(strdup(filename)); // we keep full path for callback
		icon_array[i]=o;
	} else if (filetype_ == FILES &&
	           fltk::filename_match(files[i]->d_name, pattern_)) {
		this->begin();
		Item* o = new Item(edelib::Icon::get(DEFAULT_ICON,edelib::Icon::TINY), strdup(files[i]->d_name));
		this->end();
		o->user_data(strdup(filename)); // we keep full path for callback
		icon_array[i]=o;
	}
      }

    }


	this->redraw();

	// Detect icon mimetypes etc.
	for (i=0; i<num_files; i++) {
		// ignored files
		if (!icon_array[i]) continue;
		fltk::check(); // update interface

		// get mime data
		snprintf (filename,4095,"%s%s",directory_,files[i]->d_name);
		edelib::MimeType *m = new edelib::MimeType(filename);

		// change label
		char *label;
		if (strncmp(m->id(),"directory",9)==0) 
			asprintf(&label, "%s\t%s\t\t%s", files[i]->d_name, m->type_string(), edelib::nice_time(filename_mtime(filename)));
		else 
			asprintf(&label, "%s\t%s\t%s\t%s", files[i]->d_name, m->type_string(), edelib::nice_size(filename_size(filename)), edelib::nice_time(filename_mtime(filename)));
		icon_array[i]->label(label);

		// icon
		icon_array[i]->image(m->icon(edelib::Icon::TINY));

		icon_array[i]->redraw();
		delete m;
		free(files[i]);
	}

	free(files);


  }

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

////////////////////////////////////////////////////////////////
//#include <pixmaps/file_small.xpm>
//#include <fltk/xpmImage.h>
class FileItem : public Item {
public:
    FileItem(const char * label, FileIcon * icon);
    void draw();
private:
    FileIcon* fileIcon_;
};

FileItem::FileItem(const char * label, FileIcon * icon) : Item(label) {
    fileIcon_=icon;
//    textsize(14);
    if(icon) icon->value(this,true);
}
void FileItem::draw()  {
  if (fileIcon_) fileIcon_->value(this,true);
    Item::draw();
}
////////////////////////////////////////////////////////////////

/*void FileBrowser::add(const char *line, FileIcon *icon) {
    this->begin();
    FileItem * i = new FileItem(strdup(line),icon);
    i->w((int) icon_size());  i->h(i->w());
    this->end();
}

void FileBrowser::insert(int n, const char *label, FileIcon*icon) {
    current(0);
    FileItem * i = new FileItem(strdup(label),icon);
    i->w((int) icon_size());  i->h(i->w());
    Menu::insert(*i,n);
}*/

//
// End of "$Id: FileBrowser.cxx 5071 2006-05-02 21:57:08Z fabien $".
//
