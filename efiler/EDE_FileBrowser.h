//
// "$Id: FileBrowser.h 4926 2006-04-10 21:03:29Z fabien $"
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

#ifndef fltk_FileBrowser_h
#define fltk_FileBrowser_h

#include <fltk/Browser.h>
#include <fltk/FileIcon.h>
#include <fltk/filename.h>

namespace fltk {


//
// FileBrowser class...
//

class FL_API FileBrowser : public Browser
{
  int		filetype_;
  const char	*directory_;
  float		icon_size_;
  const char	*pattern_;

public:
  enum { FILES, DIRECTORIES };

  FileBrowser(int, int, int, int, const char * = 0);

  float		icon_size() const { 
    return (icon_size_ <0?  (2.0f* textsize()) : icon_size_); 
  }
  void 		icon_size(float f) { icon_size_ = f; redraw(); };

  void	filter(const char *pattern);
  const char	*filter() const { return (pattern_); };

  int		load(const char *directory, File_Sort_F *sort = (File_Sort_F*) fltk::numericsort);

  float		textsize() const { return (Browser::textsize()); };
  void		textsize(float s) { Browser::textsize(s); icon_size_ = (uchar)(3 * s / 2); };

  int		filetype() const { return (filetype_); };
  void		filetype(int t) { filetype_ = t; };
  const char *  directory() const {return directory_;}

  // adding or inserting a line into the fileBrowser
  void insert(int n, const char* label, FileIcon* icon);
  void insert(int n, const char* label, void* data){Menu::insert(n, label,data);}
  void add(const char * line, FileIcon* icon);

  // Showing or not showing the hidden files, that's the question:
public:
  // sets this flag if you want to see the hidden files in the browser
  void	    show_hidden(bool show) { show_hidden_= show; }
  bool	    show_hidden() const {return show_hidden_;}
private:
    bool		show_hidden_;
};

}

#endif // !_Fl_File_Browser_H_

//
// End of "$Id: FileBrowser.h 4926 2006-04-10 21:03:29Z fabien $".
//
