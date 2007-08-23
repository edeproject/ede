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


#ifndef EDE_FileIconView_H
#define EDE_FileIconView_H


#include <FL/Fl_Button.H>

#include <edelib/String.h>

//#include "EDE_FileView.h"


// uncomment this to use edelib::ExpandableGroup
#define USE_FLU_WRAP_GROUP

#ifdef USE_FLU_WRAP_GROUP
 #include "Flu_Wrap_Group.h"
#else
 #include <edelib/ExpandableGroup.h>
#endif




// Dimensions
#define ICONW 70
#define ICONH 80

// Spacing to use between icons
#define ICON_SPACING 5

// Used to make edges of icons less sensitive to selection box, dnd and such
// (e.g. if selection laso only touches a widget with <5px, it will not be selected)
#define SELECTION_EDGE 5

// Sometimes I accidentaly "drag" an icon - don't want dnd to trigger
#define MIN_DISTANCE_FOR_DND 10


#ifdef USE_FLU_WRAP_GROUP
class FileIconView : public Flu_Wrap_Group {
#else
class FileIconView : public edelib::ExpandableGroup {
#endif

private:
	// private vars for handling selection and focus
	// because group doesn't do it
	int focused;
	int* m_selected;

	// callbacks
	rename_callback_type* rename_callback_;
	paste_callback_type* paste_callback_;
	Fl_Callback* context_callback_;

	// selection box
	int select_x1,select_y1,select_x2,select_y2;

	// Find the Fl_Button corresponding to real path (encoded in user_data())
	Fl_Button* find_button(edelib::String realpath) {
		for (int i=0; i<children(); i++) {
			Fl_Button* b = (Fl_Button*)child(i);
			char *tmp = (char*)b->user_data();
			if (realpath==tmp) return b;
		}
		return 0;
	}

public:

	FileIconView(int X, int Y, int W, int H, char*label=0);

	// Methods for manipulating items
	// They are named similarly to Browser methods, except that they take 
	// struct FileItem (defined in EDE_FileView.h)

	// NOTE: For compatibility with Fl_Browser-based views, row is 1..children()
	// as opposed to Fl_Group methods that use 0..children()-1
	// This is a fltk bug

	void insert(int row, FileItem *item);
	void add(FileItem *item) { insert(children()+1, item); }
	void remove(FileItem *item);
	void remove(int row);
	void update(FileItem *item);

	// This method is needed because update() uses path to find item in list
	void update_path(const char* oldpath,const char* newpath);

	// Make row look "disabled" (actually we use it for cut operation)
	void gray(int row) { 
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
		// FIXME this also means that item can't be selected
		b->deactivate();
	}
	void ungray(int row) {
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
		b->activate();
	}

	// Callback when user renames a file (takes const char* filename)
	void rename_callback(rename_callback_type* cb) { rename_callback_ = cb; }

	// Callback that is called by FL_PASTE event (such as drag&drop) 
	// (takes const char* paste_target /e.g. target directory or 0 for current directory/)
	void paste_callback(paste_callback_type* cb) { paste_callback_ = cb; }

	// Callback that is called on right mouse click (for e.g. context menu)
	void context_callback(Fl_Callback* cb) { context_callback_ = cb; }

	// Item real path (user_data)
	const char* path(int row) {
		if (row<1 || row>children()) return 0;
		Fl_Widget* w = child(row-1);
		return (const char*)w->user_data();
	}

	// Is item selected?
	int selected(int row) { 
		if (row<1 || row>children()) return 0;
		int i=0;
		while(m_selected[i]!=0)
			if (m_selected[i++]==row) return 1;
		return 0;
	}

	// Select item (if value==1) or unselect (if value==0)
	void select(int row, int value);

	// Return nr. of widget that has keyboard focus
	int get_focus() {
		Fl_Widget* focus = Fl::focus();
		int i = find(focus); // a Fl_Group method
		if (i<children()) return i+1;
		else return 0;
	}

	// Set keyboard focus to given item
	void set_focus(int row) {
		if (row<1 || row>children()) return;
		Fl_Widget* w = child(row-1);
		w->take_focus();
		show_item(row);
		focused=row;
	}

	// Scroll the view until item becomes visible
	void show_item(int row);

	// Overloaded handle() method
	// it's currently a bit messy, see implementation comments for details
	int handle(int e);

	// Override draw() for selection box
	void draw() {
#ifdef USE_FLU_WRAP_GROUP
		Flu_Wrap_Group::draw();
#else
		edelib::ExpandableGroup::draw();
#endif
		if (select_x1>0 && select_y1>0) {
			fl_color(33);
			fl_line_style(FL_DASH);
			fl_line(select_x1,select_y1,select_x1,select_y2);
			fl_line(select_x1,select_y2,select_x2,select_y2);
			fl_line(select_x2,select_y2,select_x2,select_y1);
			fl_line(select_x2,select_y1,select_x1,select_y1);
			fl_line_style(0);
		}
	}

	// Override clear so that we can empty selected & focused values
	void clear() {
		focused=0;
		if (m_selected) free(m_selected);
		m_selected = 0;
#ifdef USE_FLU_WRAP_GROUP
		Flu_Wrap_Group::clear();
		scroll_to_beginning(); // move scrollbar to top
#else
		edelib::ExpandableGroup::clear();
#endif
	}
};


#endif

/* $Id */
