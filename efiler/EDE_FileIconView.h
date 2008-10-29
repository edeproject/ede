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


#include <edelib/String.h>

//#include "EDE_FileView.h"


// comment this to use edelib::ExpandableGroup
//#define USE_FLU_WRAP_GROUP

#ifdef USE_FLU_WRAP_GROUP
 #include "Flu_Wrap_Group.h"
#else
 #include <edelib/ExpandableGroup.h>
#endif



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

	// Find the widget corresponding to real path (encoded in user_data())
	Fl_Widget* find_icon(edelib::String realpath) {
		for (int i=0; i<children(); i++) {
			Fl_Widget* w = child(i);
			char *tmp = (char*)w->user_data();
			if (realpath==tmp) return w;
		}
		return 0;
	}

	// EditBox is displayed when renaming a file
	// We subclass Fl_Input so we can handle keyboard events
	class EditBox : public Fl_Input {
	public:
		EditBox(int x, int y, int w, int h, const char* label = 0) : Fl_Input(x,y,w,h,label)  {}
		int handle (int e);
	}* editbox_;
	int editbox_row;

	// show editbox at specified row and make the row "invisible" (bgcolor same as fgcolor)
	void show_editbox(int row);
	void hide_editbox();

	// This is called to actually rename file (when user presses Enter in editbox)
	void finish_rename();

	// Try various names and sizes for icons
	Fl_Image* try_icon(edelib::String icon_name);


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

	void start_rename() {
		show_editbox(get_focus());
	}
};


#endif

/* $Id */
