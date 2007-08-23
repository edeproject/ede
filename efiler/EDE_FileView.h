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
 * and permission
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
 * programs that don't work with standard filesystem.
 *
 */


#ifndef EDE_FileView_H
#define EDE_FileView_H


#include <edelib/String.h>

struct FileItem {
	edelib::String name; // just the name
	edelib::String icon;
	edelib::String size;
	edelib::String realpath; // whatever the caller uses to access the file
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
	void insert(int row, FileItem *item) { browser->insert(row,item); icons->insert(row,item); }
	void add(FileItem *item) { browser->add(item); icons->add(item); }
	void remove(FileItem *item) { browser->remove(item); icons->remove(item); }
	void update(FileItem *item) { browser->update(item); icons->update(item); }

	void update_path(const char* oldpath,const char* newpath) { browser->update_path(oldpath,newpath); icons->update_path(oldpath,newpath); }

	void callback(Fl_Callback*cb) { browser->callback(cb); icons->callback(cb);}
	void rename_callback(rename_callback_type* cb) { browser->rename_callback(cb); icons->rename_callback(cb); }
	void paste_callback(paste_callback_type* cb) { browser->paste_callback(cb); icons->paste_callback(cb); }
	void context_callback(Fl_Callback* cb) { browser->context_callback(cb); icons->context_callback(cb); }

	void remove(int i) { browser->remove(i); icons->remove(i);}
	void clear() { browser->clear(); icons->clear();}

	// Methods forwarded to just the active view
	void gray(int row) { if (m_type==FILE_DETAILS_VIEW) browser->gray(row); else icons->gray(row); }
	void ungray(int row) { if (m_type==FILE_DETAILS_VIEW) browser->ungray(row); else icons->ungray(row); }

	const char* path(int i) { if (m_type==FILE_DETAILS_VIEW) return browser->path(i); else return icons->path(i); }
	int size() { if (m_type==FILE_DETAILS_VIEW) return browser->size(); else return icons->children();}
	int selected(int i) { if (m_type==FILE_DETAILS_VIEW) return browser->selected(i); else return icons->selected(i); }
	void select(int i, int k) { if (m_type==FILE_DETAILS_VIEW) { browser->select(i,k); browser->show_item(i); } else { icons->select(i,k); icons->show_item(i); } }
	int get_focus() { if (m_type==FILE_DETAILS_VIEW) return browser->get_focus(); else return icons->get_focus(); }
	void set_focus(int i) { if (m_type==FILE_DETAILS_VIEW) browser->set_focus(i); else icons->set_focus(i);}
	int take_focus() { if (m_type==FILE_DETAILS_VIEW) return browser->take_focus(); else return icons->take_focus(); }

	// These methods are used by do_rename() - should become obsoleted
	const char* text(int i) { return browser->text(i); }
	void text(int i, const char* c) { return browser->text(i,c); }
	uchar column_char() { return browser->column_char(); }
};



#endif

/* $Id */
