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

#ifndef edelib_FileBrowser_h
#define edelib_FileBrowser_h

#include <fltk/Browser.h>
#include <fltk/filename.h>
#include <fltk/Input.h>

//namespace fltk {


// Class for the little input box used when we edit an item

// FIXME: Why can this box be resized????
// TODO: When editbox appears, select filename part and leave extension not-selected 
// (as in Konqueror)

class EditBox : public fltk::Input {
	fltk::Widget* editing_;
public:
	EditBox(int x, int y, int w, int h, const char* label = 0) : fltk::Input(x,y,w,h,label) , editing_(0) {}
	void editing(fltk::Widget*w) { editing_=w; }
	int handle (int event);
	void hide();
};


/*! \class edelib::FileBrowser

This class can be used as a drop-in replacement for fltk::FileBrowser because
it has much of the same API. In addition, it has the following features:
 - uses edelib::MimeType and edelib::Icon to display file icons
 - multicolumn view with columns: file name, type, size, date of last 
modification and file permissions
 - sorting by each column, by clicking on column headers or programmatically
 - renaming files by clicking on a already selected file
 - drag&drop support
 - several other properties for fine tuning of display.

By convention, real filename with full path of each item is stored in its
user_data() so you can easily access it from item callback or using the
const char* system_path(int index) method.

Events that take place on double clicking, dropping files, popup menu etc.
should be implemented in callback, which enables you to create read-write
or read-only widgets as desired (e.g. in file chooser).

*/

//
// FileBrowser class...
//

class FileBrowser : public fltk::Browser
{
  int		filetype_;
  const char	*directory_;
  //float		icon_size_;
  const char	*pattern_;
  EditBox	*editbox_;

  bool		show_dotdot_;

public:
  enum { FILES, DIRECTORIES, BOTH };

  FileBrowser(int, int, int, int, const char * = 0);

  /*float		icon_size() const { 
    return (icon_size_ <0?  (2.0f* textsize()) : icon_size_); 
  }
  void 		icon_size(float f) { icon_size_ = f; redraw(); };*/

  void	filter(const char *pattern);
  const char	*filter() const { return (pattern_); };

  int		load(const char *directory, fltk::File_Sort_F *sort = (fltk::File_Sort_F*) fltk::casenumericsort);

/*  float		textsize() const { return (fltk::Browser::textsize()); };
  void		textsize(float s) { fltk::Browser::textsize(s); icon_size_ = (uchar)(3 * s / 2); };*/

  int		filetype() const { return (filetype_); };
  void		filetype(int t) { filetype_ = t; };
  const char *  directory() const {return directory_;}

  void		show_up_directory(bool t) { show_dotdot_ = t; }

  // adding or inserting a line into the fileBrowser
  //void insert(int n, const char* label, fltk::FileIcon* icon);
  //void insert(int n, const char* label, void* data){fltk::Menu::insert(n, label,data);}
  //void add(const char * line, fltk::FileIcon* icon);

  // Return full system path to given item
  const char* 	system_path(int i) const { return (const char*)child(i)->user_data(); }

  // We override handle for displaying editbox
  int 		handle(int);
  // Showing or not showing the hidden files, that's the question:
public:
  // sets this flag if you want to see the hidden files in the browser
  void	    show_hidden(bool show) { show_hidden_= show; }
  bool	    show_hidden() const {return show_hidden_;}
private:
    bool		show_hidden_;

	static void	editbox_cb(Widget*,void*);
	int		editing;
};

//}

#endif // !_Fl_File_Browser_H_

//
// End of "$Id$".
//
