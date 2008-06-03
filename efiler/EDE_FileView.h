/*
 * $Id$
 *
 * EDE FileView class
 * Part of edelib.
 * Copyright (c) 2005-2007 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

/**
 * \class FileView
 * \brief Widget for displaying a list of files
 *
 * FileView is a generic widget that displays files in a 
 * certain way. To get/change the type of view, use method 
 * type() which sets or returns a value of type enum FileViewType. 
 * Allowed values are:
 *    FILE_LIST_VIEW - a multicolumn list of files (if necessary,
 * a horizontal scrollbar will be shown)
 *    FILE_DETAILS_VIEW - each file is in a single row, and columns 
 * contain information such as file type, size, date of last change 
 * and permissions
 *    FILE_ICON_VIEW - large icons are shown, and filenames are 
 * displayed below icons (if necessary, a vertical scrollbar will be 
 * shown), additional data is shown in tooltips
 *
 * Of course, you are free to directly use classes FileListView, 
 * FileDetailsView and FileIconView respectively (however, you have 
 * to include FileView.h to get them).
 *
 * Unlike some similar classes, FileView doesn't do any scanning of 
 * directories. Instead, a program using FileView should use methods 
 * such as add(), insert(), remove() and update(). These methods take 
 * values of type struct FileItem, which contains all the data shown 
 * by FileDetailsView. The reason for this is to enable reuse in 
 * programs that don't work with standard filesystem (archivers,
 * network browsers etc.) These methods work on all views at the same
 * time, which means that after calling type(), you don't have to
 * repopulate the view.
 *
 * NOTE: for compatibility with Fl_Browser, item indexes are 1 to n 
 * as opposed to the C & C++ usual 0 to n-1
 *
 */


#ifndef EDE_FileView_H
#define EDE_FileView_H


#include <edelib/String.h>

struct FileItem {
	edelib::String name; // just the name
	edelib::String icon;
	edelib::String size;
	edelib::String realpath; // whatever the caller uses to access the file - can be VFS
	edelib::String description;
	edelib::String date;
	edelib::String permissions;
};

// Type for callbacks
// I don't know how to do this without creating a new type :(
typedef void (rename_callback_type)(const char*);
typedef void (paste_callback_type)(const char*);



#include "EDE_FileDetailsView.h"
#include "EDE_FileIconView.h"


enum FileViewType {
	FILE_DETAILS_VIEW,
	FILE_ICON_VIEW
};




class FileView : public Fl_Group {
private:
	FileDetailsView* browser;
	FileIconView* icons;
	FileViewType m_type;
public:
	/**
	 * Constructor - inherits Fl_Group ctor (with the same behavior)
	 */
	FileView(int X, int Y, int W, int H, char*label=0) : Fl_Group(X,Y,W,H,label) {
		browser = new FileDetailsView(X,Y,W,H,label);
		browser->end();
//		browser->hide();
		icons = new FileIconView(X,Y,W,H,label);
		icons->end();
		end();

		// Set default to FILE_DETAILS_VIEW
		icons->hide(); 
		m_type=FILE_DETAILS_VIEW;
	}

	/**
	 * Get/set type of view. View contents will be inherited, you don't
	 * have to re-add all items after changing type.
	 */
	void type(FileViewType t) {
		m_type=t;
		if (t==FILE_DETAILS_VIEW) {
			icons->hide();
			browser->show();
//			browser->resize(x(),y(),w(),h());
//			browser->redraw(); 
			Fl_Group::add(browser);
		}
		if (t==FILE_ICON_VIEW) {
			browser->hide(); 
			icons->show(); 
			Fl_Group::add(icons);
//			icons->show(); 
//			icons->redraw(); 
			//redraw();
		}
		redraw();
	}
	FileViewType type() { return m_type; }


	// Methods that must be implemented by each view

	// Setter methods (forwarded to all views)

	/**
	 * Insert given item after item with index "row". Note that index is 1 to n
	 */
	void insert(int row, FileItem *item) { browser->insert(row,item); icons->insert(row,item); }

	/**
	 * Add given item at end of list.
	 */
	void add(FileItem *item) { browser->add(item); icons->add(item); }

	/**
	 * Remove given item from list. Item is found according to realpath which is
	 * supposed to be unique.
	 */
	void remove(FileItem *item) { browser->remove(item); icons->remove(item); }

	/**
	 * Update given item. Item is matched according to realpath, which means that 
	 * this method is not suitable for renaming (but see method update_path). Method
	 * also does redrawing.
	 */
	void update(FileItem *item) { browser->update(item); icons->update(item); }

	/**
	 * Update real path of an item in the list. Since update() can't be used for 
	 * changing realpath, you can use this method for that part. This will also
	 * update the label (name of file).
	 */
	void update_path(const char* oldpath,const char* newpath) { browser->update_path(oldpath,newpath); icons->update_path(oldpath,newpath); }

	/**
	 * Define callback function for view. Callback is called when an item is 
	 * doubleclicked or Enter key is pressed. Callback function is standard
	 * Fl_Callback.
	 */
	void callback(Fl_Callback*cb) { browser->callback(cb); icons->callback(cb);}

	/**
	 * Define callback function which is called when file renaming is attempted. 
	 * Renaming will start when user clicks on an already selected item (see
	 * method start_rename() for other ways). If no rename callback is defined, 
	 * then renaming will not be possible, and nothing will happen. This is the 
	 * default behavior. 
	 *
	 * Rename callback function must be of type void()(const char*) and the 
	 * parameter is new file name (old file name can be found using 
	 * view->path(view->get_focus()) ).
	 */
	void rename_callback(rename_callback_type* cb) { browser->rename_callback(cb); icons->rename_callback(cb); }

	/**
	 * Define callback function which is called when a file is dropped using 
	 * mouse into current view (drag&drop). If no paste callback is defined,
	 * widget will refuse dropping and corresponding cursor will be shown 
	 * (usually a cross). 
	 *
	 * Paste callback function must be of type void()(const char*) and the 
	 * parameter is destination / target (usually the current directory). 
	 * If the parameter is 0, function must assume current directory, which 
	 * is what the parent loaded into view (thus parent application must know 
	 * which is the current directory). Source is naturally in 
	 * Fl::event_text()
	 */
	void paste_callback(paste_callback_type* cb) { browser->paste_callback(cb); icons->paste_callback(cb); }

	/**
	 * Define callback function which is called when right menu button is 
	 * clicked. This will usually display a context menu. If no context
	 * callback is defined, nothing will happen (default). Callback 
	 * function is standard Fl_Callback.
	 */
	void context_callback(Fl_Callback* cb) { browser->context_callback(cb); icons->context_callback(cb); }

	/**
	 * Remove item with index i from view. Note that index is 1 to n
	 */
	void remove(int i) { browser->remove(i); icons->remove(i);}

	/**
	 * Remove all items from view and redraw.
	 */
	void clear() { browser->clear(); icons->clear();}

	// Methods forwarded to just the active view
	/**
	 * Gray the item marked with index, marking it for cut operation.
	 */
	void gray(int row) { if (m_type==FILE_DETAILS_VIEW) browser->gray(row); else icons->gray(row); }

	/**
	 * Reverse the effect of method gray().
	 */
	void ungray(int row) { if (m_type==FILE_DETAILS_VIEW) browser->ungray(row); else icons->ungray(row); }

	/**
	 * Return the full path of item with index i.
	 */
	const char* path(int i) const { if (m_type==FILE_DETAILS_VIEW) return browser->path(i); else return icons->path(i); }

	/**
	 * Return the total number of items currently in view
	 */
	int size() const { if (m_type==FILE_DETAILS_VIEW) return browser->size(); else return icons->children();}

	/**
	 * Return 1 if item with index i is currently selected and 0 if it isn't.
	 * Selection means that item has a darker background and next operation
	 * will be performed on that item.
	 */
	int selected(int i) const { if (m_type==FILE_DETAILS_VIEW) return browser->selected(i); else return icons->selected(i); }

	/**
	 * Select (mark) item with index i. If k is 0, then unselect. This also 
	 * has the effect that the item is focused.
	 */
	void select(int i, int k) { if (m_type==FILE_DETAILS_VIEW) { browser->select(i,k); browser->show_item(i); } else { icons->select(i,k); icons->show_item(i); } }

	/**
	 * Return index of item which currently has focus, meaning that focus
	 * box (small dashes) is drawn around it. Focus can be changed with
	 * arrow keys and is useful for keyboard navigation.
	 */
	int get_focus() const { if (m_type==FILE_DETAILS_VIEW) return browser->get_focus(); else return icons->get_focus(); }

	/**
	 * Set focus on item that has index i. The view will also be scrolled
	 * to show that item.
	 */
	void set_focus(int i) { if (m_type==FILE_DETAILS_VIEW) browser->set_focus(i); else icons->set_focus(i);}

	/**
	 * This standard fltk method is overloaded to enable the currently
	 * visible widget to take focus from other widgets in a window.
	 */
	int take_focus() { if (m_type==FILE_DETAILS_VIEW) return browser->take_focus(); else return icons->take_focus(); }

	/**
	 * Start renaming the currently focused file. Since rename box is 
	 * implemented inside view widget, this is useful if you want to
	 * enable an alternate method for starting rename operation (e.g.
	 * F2 key).
	 */
	void start_rename() { if (m_type==FILE_DETAILS_VIEW) return browser->start_rename(); else return icons->start_rename(); }
};



#endif

/* $Id */
