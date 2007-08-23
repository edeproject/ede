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


#ifndef EDE_FileDetailsView_H
#define EDE_FileDetailsView_H


#include <FL/Fl_Input.H>

#include <edelib/String.h>

#include "EDE_Browser.h"
//#include "EDE_FileView.h"



// Sometimes I accidentaly "drag" an icon - don't want dnd to trigger
#define MIN_DISTANCE_FOR_DND 10


class FileDetailsView : public EDE_Browser {
private:
//	EDE_Browser* browser; - yada
	// internal list of items
/*	struct myfileitem {
		struct FileItem *item;
		myfileitem* previous;
		myfileitem* next;
	} *firstitem;*/

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


	// Find row number corresponding to realpath (encoded in data())
	int findrow(edelib::String realpath) {
		for (int i=1; i<=size(); i++) {
			char *tmp = (char*)data(i);
			if (realpath==tmp) return i;
		}
		return 0;
	}

	// Callbacks
	rename_callback_type* rename_callback_;
	paste_callback_type* paste_callback_;
	Fl_Callback* context_callback_;



	// Deallocator class is used to prevent memleaks
	// It stores pointers to allocated memory that will be cleaned up later
	// Just remember to call empty() when needed - everything else is automatic :)

	// -- The reason we need such class is that there are no insert_copy() or 
	// data_copy() methods in Fl_Browser (cf. label_copy() in Fl_Widget)

	#define MEM_STEP 1000
	class Deallocator {
		void** items;
		int size, capacity;
	public:
		Deallocator() : size(0), capacity(MEM_STEP) {
			items = (void**)malloc(sizeof(void*)*capacity);
			for (int i=0; i<capacity; i++) items[i]=0;
		}
		~Deallocator() { empty(); free(items); }
		void add(void* p) {
			if (size>=capacity) {
				capacity+=MEM_STEP;
				items = (void**)realloc(items,sizeof(void*)*capacity);
				for (int i=capacity-MEM_STEP; i<capacity; i++)
					items[i]=0;
			}
			items[size++]=p;
		}
		void empty() {
			for (int i=0; i<size; i++) {
				if (items[i]) free(items[i]);
				items[i]=0;
			}
			size=0;
		}
		void debug() {
			fprintf(stderr, "Bucket size %d, capacity %d\n", size, capacity);
		}
	} bucket;

public:
	FileDetailsView(int X, int Y, int W, int H, char*label=0);
//	~FileDetailsView() { delete browser; }

	// Methods for manipulating items
	// They are named similarly to Browser methods, except that they take 
	// struct FileItem (defined in EDE_FileView.h)
	void insert(int row, FileItem *item);
	void add(FileItem *item) { insert(size()+1, item); }
	void remove(FileItem *item) {
		int row = findrow(item->realpath);
		if (row) EDE_Browser::remove(row);
	}
	void remove(int row) { EDE_Browser::remove(row); } // doesnt work without this method :(
	void update(FileItem *item); // refresh one row without reloading the whole list

	// This method is needed because update() uses path to find item in list
	void update_path(const char* oldpath,const char* newpath);

	// Make row look "disabled" (actually we use it for cut operation)
	void gray(int row);
	void ungray(int row);

	// Overloaded handle for: context menu, file renaming and dnd support
	int handle(int e);

	// Callback when user renames a file (takes const char* filename)
	void rename_callback(rename_callback_type* cb) { rename_callback_ = cb; }

	// Callback that is called by FL_PASTE event (such as drag&drop) 
	// (takes const char* paste_target /e.g. target directory or 0 for current directory/)
	void paste_callback(paste_callback_type* cb) { paste_callback_ = cb; }

	// Callback that is called on right mouse click (for e.g. context menu)
	void context_callback(Fl_Callback* cb) { context_callback_ = cb; }

	// Avoid memory leak
	void clear() {
		bucket.empty();
		EDE_Browser::clear();
	}

	int visible() { return Fl_Widget::visible(); }

	// Aliases for better compatibility with other views
	const char*path(int row) { return (const char*)data(row); }
	void show_item(int row) { middleline(row); }
};




#endif

/* $Id */
