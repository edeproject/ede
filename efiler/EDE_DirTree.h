/*
 * $Id$
 *
 * EDE Directory tree class
 * Part of edelib.
 * Copyright (c) 2005-2007 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */



#include "Fl_Icon_Browser.h"
#include <stdlib.h>
#include <string.h>



class DirTree : public Fl_Icon_Browser
{
	bool show_hidden_;
	bool ignore_case_;
	bool subdir_scan(int line);
	int find_best_match(const char* path, int parent=0);

public:

	DirTree(int X, int Y, int W, int H, const char *l = 0) : Fl_Icon_Browser(X, Y, W, H, l), show_hidden_(false), ignore_case_(false) {
		type(FL_HOLD_BROWSER);
	}

	// All texts and datas are dynamically allocated and need to be freed
	~DirTree() {
		for (int i=1; i<=size(); i++) {
			free (data(i));
//			free ((void*)text(i));
		}
	}

	// Load top-level directories into widget
	void init();

	// Set given directory as current. Returns false if directory doesn't exist
	// (the closest parent directory will be selected)
	bool set_current(const char* path);

  // Return full system path to given item
	const char* system_path(int i) const { return (const char*)data(i); }

	// Shortcut function to redraw directory tree
	void reload() {
		char* path = strdup(system_path(get_focus()));
		clear();
		init();
		set_current(path);
		free(path);
		redraw();
	}

	// setter/getter methods for flags
	void show_hidden(bool show) { show_hidden_= show; }
	bool show_hidden() const {return show_hidden_;}
	void ignore_case(bool i) { ignore_case_= i; }
	bool ignore_case() const {return ignore_case_;}
};

//}


//
// End of "$Id: FileBrowser.h 4926 2006-04-10 21:03:29Z fabien $".
//
