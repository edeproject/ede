//
// "$Id$"
//
// FileBrowser definitions.
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

//
// Include necessary header files...
//

#ifndef edelib_DirTree_h
#define edelib_DirTree_h

#include <fltk/Browser.h>
#include <fltk/filename.h>

//namespace fltk {


//
// FileBrowser class...
//

class DirTree : public fltk::Browser
{


public:

  DirTree(int, int, int, int, const char * = 0);

  /*float		icon_size() const { 
    return (icon_size_ <0?  (2.0f* textsize()) : icon_size_); 
  }
  void 		icon_size(float f) { icon_size_ = f; redraw(); };*/

	// Set given directory as current. Returns false if directory doesn't exist
	// (the closest parent directory will be selected)
	bool 		set_current(const char* path);

	// Load base directories into widget
	int		load();

	// Browser method for opening (expanding) a subtree
	// We override this method so we could scan directory tree on demand
	bool		set_item_opened(bool);

  // adding or inserting a line into the fileBrowser
  //void insert(int n, const char* label, fltk::FileIcon* icon);
  //void insert(int n, const char* label, void* data){fltk::Menu::insert(n, label,data);}
  //void add(const char * line, fltk::FileIcon* icon);

  // Return full system path to given item
	const char* 	system_path(int i) const { 
			return (const char*)child(i)->user_data(); }

  // Showing or not showing the hidden files, that's the question:
public:
  // sets this flag if you want to see the hidden files in the browser
  void	    show_hidden(bool show) { show_hidden_= show; }
  bool	    show_hidden() const {return show_hidden_;}
private:
    bool		show_hidden_;
	bool		find_best_match(const char* path, int* indexes, int level);
};

//}

#endif // !_Fl_File_Browser_H_

//
// End of "$Id$".
//
