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


#ifndef EDE_FileView_H
#define EDE_FileView_H

#include <Fl/Fl_Shared_Image.H>
#include <Fl/Fl_Input.H>

#include <edelib/String.h>
#include <edelib/IconTheme.h>

#include "EDE_Browser.h"

// This class provides the generic FileView class and two
// derived classes called FileIconView and FileDetailsView.
// These classes are interchangeable, so that application 
// can just declare FileView* and use.
// FileView is populated with FileItem's.

struct FileItem {
	edelib::String name; // just the name
	edelib::String icon;
	edelib::String size;
	edelib::String realpath; // whatever the caller uses to access the file
	edelib::String description;
	edelib::String date;
	edelib::String permissions;
};


// Type for rename_callback
// I don't know how to do this without creating a new type :(
typedef void (rename_callback_type)(const char*);
typedef void (paste_callback_type)(const char*);




class FileDetailsView_ : public EDE_Browser {
private:
//	EDE_Browser* browser; - yada
	// internal list of items
/*	struct myfileitem {
		struct FileItem *item;
		myfileitem* previous;
		myfileitem* next;
	} *firstitem;*/

	int findrow(edelib::String realpath) {
		for (int i=1; i<=size(); i++) {
			char *tmp = (char*)data(i);
			if (realpath==tmp) return i;
		}
		return 0;
	}


	rename_callback_type* rename_callback_;
	paste_callback_type* paste_callback_;
	Fl_Callback* context_callback_;

	// Subclass Fl_Input so we can handle keyboard
	class EditBox : public Fl_Input {
		friend class FileDetailsView;
	public:
		EditBox(int x, int y, int w, int h, const char* label = 0) : Fl_Input(x,y,w,h,label)  {}
		int handle (int e) {
			FileDetailsView_* view = (FileDetailsView_*)parent();
			if (e==FL_KEYBOARD && visible()) {
				int k = Fl::event_key();
				if (Fl::event_key()==FL_Enter && visible()) {
					view->hide_editbox();
					view->do_rename();
				} else if (k==FL_Up || k==FL_Down || k==FL_Page_Up || k==FL_Page_Down) {
					view->hide_editbox();
					return 0; // let the view scroll
				} else if (k==FL_F+2 || k==FL_Escape)
					view->hide_editbox();
				else
					Fl_Input::handle(e);
				
				return 1; // don't send keys to view
			}
			if (e==FL_MOUSEWHEEL && visible()) view->hide_editbox();
			return Fl_Input::handle(e);
		}
	}* editbox_;
	int editbox_row;

	// show editbox at specified row and make the row "invisible" (bgcolor same as fgcolor)
	void show_editbox(int row) {
		if (!rename_callback_) return;
		if (row<1 || row>size()) return; // nothing selected
		if (text(row)[0]=='.' && text(row)[1]=='.' && text(row)[2]==column_char()) return; // can't rename "go up" button

		// unselect the row with focus - it's prettier that way
		select(row,0);

		// Copy filename to editbox
		char* filename = strdup(text(row));
		char* tmp = strchr(filename, column_char());
		if (tmp) *tmp='\0';
		editbox_->value(filename);
		bucket.add(filename);

		// make the row "invisible"
		editbox_row=row;
		char* ntext = (char*)malloc(sizeof(char)*(strlen(text(row))+8)); // add 7 places for format chars
		strncpy(ntext+7, text(row), strlen(text(row)));
		strncpy(ntext, "@C255@.", 7);
		ntext[strlen(text(row))+7]='\0';
		text(row,ntext);
		bucket.add(ntext);


		// calculate location for editbox
		// "Magic constants" here are just nice spacing
		int X=x()+get_icon(row)->w()+8; 
		int W=150; // reasonable
		int H=get_icon(row)->h()+2;
		int Y=y()+2 - position();
		void* item = item_first();
		for (int i=1; i<row; i++) {
			Y+=item_height(item);
			item=item_next(item);
		}
		editbox_->resize(X,Y,W,H);
		editbox_->show();
		editbox_->take_focus();
		editbox_->redraw();
	}

	void hide_editbox() {
		fprintf(stderr, "hide_editbox()\n");
		editbox_->hide();

		// Make the edited row visible again
		char* ntext = (char*)malloc(sizeof(char)*(strlen(text(editbox_row))-6)); // remove 7 places for format chars
		strncpy(ntext, text(editbox_row)+7, strlen(text(editbox_row))-7);
		ntext[strlen(text(editbox_row))-7]='\0';
		text(editbox_row,ntext);
		bucket.add(ntext);
	}

	// Do the rename
	void do_rename() {
		fprintf(stderr, "editbox_cb()\n");
		rename_callback_(editbox_->value());
		//hide_editbox();
	}

	// Bucket class is used to prevent memleaks
	// It stores pointers to allocated memory that will be cleaned up later
	// Just remember to call empty() when needed - everything else is automatic :)
	class Bucket {
			void** items;
			int size, capacity;
		public:
			Bucket() : size(0), capacity(1000) {
				items = (void**)malloc(sizeof(void*)*capacity);
				for (int i=0; i<capacity; i++) items[i]=0;
			}
			~Bucket() { empty(); free(items); }
			void add(void* p) {
				if (size>=capacity) {
					capacity+=1000;
					items = (void**)realloc(items,sizeof(void*)*capacity);
					for (int i=capacity-1000; i<capacity; i++)
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
	FileDetailsView_(int X, int Y, int W, int H, char*label=0) : EDE_Browser(X,Y,W,H,label) {
//		browser = new EDE_Browser(X,Y,W,H,label);
//		browser->end();
//		end();
//		resizable(browser->the_scroll());
//		resizable(browser->the_group());
//		resizable(browser);
		const int cw[]={250,200,70,130,100,0};
		column_widths(cw);
		column_char('\t');
		column_header("Name\tType\tSize\tDate\tPermissions");
		const SortType st[]={ALPHA_CASE_SORT, ALPHA_CASE_SORT, FILE_SIZE_SORT, DATE_SORT, ALPHA_SORT, NO_SORT};
		column_sort_types(st);
		when(FL_WHEN_ENTER_KEY_ALWAYS);
		//when(FL_WHEN_ENTER_KEY_CHANGED);
		//first=0;

		editbox_ = new EditBox(0, 0, 0, 0);
		editbox_->box(FL_BORDER_BOX);
		editbox_->parent(this);
		editbox_->hide();

		rename_callback_ = 0;
		paste_callback_ = 0;
		context_callback_ = 0;
	}
//	~FileDetailsView() { delete browser; }

	void insert(int row, FileItem *item) {
		// Construct browser line
		edelib::String value;
		value = item->name+"\t"+item->description+"\t"+item->size+"\t"+item->date+"\t"+item->permissions;
		char* realpath = strdup(item->realpath.c_str());
		EDE_Browser::insert(row, value.c_str(), realpath); // put realpath into data
		bucket.add(realpath);
fprintf (stderr, "value: %s\n", value.c_str());

		// Get icon
		edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_TINY);
		if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_TINY,edelib::ICON_CONTEXT_MIMETYPE);
		set_icon(row, Fl_Shared_Image::get(icon.c_str()));
	}


	void add(FileItem *item) { insert(size()+1, item); }

	void remove(FileItem *item) {
		int row = findrow(item->realpath);
		if (row) EDE_Browser::remove(row);
	}
	void remove(int row) { EDE_Browser::remove(row); } // why???
	void update(FileItem *item) {
		int row=findrow(item->realpath);
		if (row==0) return;

		//EDE_Browser::remove(row);
		//insert(row, item);
		// this was reimplemented because a) it's unoptimized, b) adds everything at the end, 
		// c) causes browser to lose focus, making it impossible to click on something while
		// directory is loading

		edelib::String value;
		value = item->name+"\t"+item->description+"\t"+item->size+"\t"+item->date+"\t"+item->permissions;
		char* realpath = strdup(item->realpath.c_str());
		text(row, value.c_str());
		data(row, realpath);
		bucket.add(realpath);
fprintf (stderr, "value: %s\n", value.c_str());

		// Get icon
		edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_TINY);
		if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_TINY,edelib::ICON_CONTEXT_MIMETYPE);
		set_icon(row, Fl_Shared_Image::get(icon.c_str()));
	}

	// This is needed because update() uses path to find item in list
	void update_path(const char* oldpath,const char* newpath) {
		int row=findrow(oldpath);
		if (row==0) return;
		char* c = strdup(newpath);
		data(row,c);
		bucket.add(c);
	}

	// Change color of row to gray
	void gray(int row) {
		if (text(row)[0] == '@' && text(row)[1] == 'C') return; // already greyed

		char *ntext = (char*)malloc(sizeof(char)*(strlen(text(row))+7)); // add 6 places for format chars
		strncpy(ntext+6, text(row), strlen(text(row)));
		strncpy(ntext, "@C25@.", 6);
		ntext[strlen(text(row))+6]='\0';
		text(row,ntext);
		bucket.add(ntext);

		// grey icon - but how to ungray?
		Fl_Image* im = get_icon(row)->copy();
		im->inactive();
		set_icon(row,im);

	}

	void ungray(int row) {
		if (text(row)[0] != '@' || text(row)[1] != 'C') return;  // not greyed

		char *ntext = (char*)malloc(sizeof(char)*(strlen(text(row))-5)); // remove 6 places for format chars
		strncpy(ntext, text(row)+6, strlen(text(row))-6);
		ntext[strlen(text(row))-6]='\0';
		text(row,ntext);
		bucket.add(ntext);

		// doesn't work
		//Fl_Image* im = get_icon(row);
		//im->uncache(); // doesn't work

		//redraw(); // OPTIMIZE
	}


	// Overloaded handle for file renaming and dnd support

	int handle(int e) {
		// Right click

//fprintf(stderr, "Event: %d\n", e);

		if (e==FL_RELEASE && Fl::event_button()==3) {
			void* item = item_first();
			int itemy=y()-position();
			int i;
			for (i=1; i<=size(); i++) {
				itemy+=item_height(item);
				if (itemy>Fl::event_y()) break;
			}
			
			set_focus(i);
			Fl::event_is_click(0); // prevent doubleclicking with right button
			if (context_callback_) context_callback_(this, data(i));
			return 1;
		}
//		if (e==FL_RELEASE && Fl::event_button()==3) { return 1; }

		/* ------------------------------
			Rename support
		--------------------------------- */
//		fprintf (stderr, "Event: %d\n", e);

/*		if (e==FL_KEYBOARD) {
			if (Fl::event_key()==FL_F+2) {
				if (editbox_->visible())
					hide_editbox();
				else
					show_editbox(get_focus());
			}
		}*/
		if (e==FL_PUSH && editbox_->visible() && !Fl::event_inside(editbox_))
			hide_editbox(); // hide editbox when user clicks outside of it
		if (e==FL_MOUSEWHEEL && editbox_->visible())
			hide_editbox(); // hide editbox when scrolling mouse

		// Click once on item that is already selected AND focused to rename it
		static bool renaming=false;
		if (e==FL_PUSH) renaming=false;
		if (e==FL_PUSH && !editbox_->visible() && Fl::event_clicks()==0 && Fl::event_button()==1) {
			const int* l = column_widths();
			if (Fl::event_x()<x() || Fl::event_x()>x()+l[0])
				return Fl_Icon_Browser::handle(e); // we're only interested in first column
			
			// Is clicked item also focused?
			void* item = item_first();
			int focusy=y()-position();
			for (int i=1; i<get_focus(); i++) {
				focusy+=item_height(item);
				if (focusy>Fl::event_y()) break;
				item=item_next(item);
			}
			if (Fl::event_y()<focusy || Fl::event_y()>focusy+item_height(item))
				return Fl_Icon_Browser::handle(e); // It isn't
			if (selected(get_focus())!=1)
				return Fl_Icon_Browser::handle(e); // If it isn't selected, then this action is select
			
			renaming=true; // On next event, we will check if it's doubleclick
		}

		if (renaming && (e==FL_DRAG || e==FL_RELEASE || e==FL_MOVE) && Fl::event_is_click()==0) {
			// Fl::event_is_click() will be >0 on doubleclick
			show_editbox(get_focus());
			renaming=false;
			return 1; // don't pass mouse event, otherwise item will become reselected which is a bit ugly
		}

		/* ------------------------------
			Drag&drop support
		--------------------------------- */

		// If paste_callback_ isn't set, that means we don't support dnd
		if (paste_callback_==0) return EDE_Browser::handle(e);

		// Let the window manager know that we accept dnd
		if (e==FL_DND_ENTER||e==FL_DND_DRAG) return 1;

		// Scroll the view by dragging to border (only works on heading... :( )
		if (e==FL_DND_LEAVE) {
			if (Fl::event_y()<y())
				position(position()-1);
			if (Fl::event_y()>y()+h())
				position(position()+1);
			return 1;
		}

		// Don't unselect items on FL_PUSH cause that could be dragging
		if (e==FL_PUSH && Fl::event_clicks()!=1) return 1;

		static int dragx,dragy;
		if (e==FL_DRAG) { 
			edelib::String selected_items;
			for (int i=1; i<=size(); i++)
				if (selected(i)==1) {
//					if (selected_items != "") selected_items += "\r\n";
					selected_items += "file://";
					selected_items += (char*)data(i);
					selected_items += "\r\n";
				}
			Fl::copy(selected_items.c_str(),selected_items.length(),0);
			Fl::dnd();
			dragx = Fl::event_x(); dragy = Fl::event_y();
			return 1; // don't do the multiple selection thing from Fl_Browser
		}

		static bool dndrelease=false;
		if (e==FL_DND_RELEASE) {
			if (Fl::event_y()<y() || Fl::event_y()>y()+h()) return 1; 
				// ^^ this can be a source of crashes in Fl::dnd()

fprintf(stderr, "FL_DND_RELEASE '%s'\n", Fl::event_text());
			// Sometimes drag is accidental
			if (abs(Fl::event_x()-dragx)>10 || abs(Fl::event_y()-dragy)>10) {
				dndrelease=true;
				Fl::paste(*this,0);
			}
			return 1;
		}
		if (e==FL_PASTE) {
fprintf(stderr, "FL_PASTE\n");
			if (!Fl::event_text() || !Fl::event_length()) return 1;
fprintf(stderr, "1 '%s' (%d)\n",Fl::event_text(),Fl::event_length());

			// Paste comes from menu/keyboard
			if (!dndrelease) {
				paste_callback_(0);
				return 1;
			}
			dndrelease=false;

			// Where is the user dropping?
			// If it's not the first column, we assume the current directory
			const int* l = column_widths();
			if (Fl::event_x()<x() || Fl::event_x()>x()+l[0]) {
				paste_callback_(0);
				return 1;
			}

			// Find item where stuff is dropped
			void* item = item_first();
			int itemy=y()-position();
			int i;
			for (i=1; i<=size(); i++) {
				itemy+=item_height(item);
				if (itemy>Fl::event_y()) break;
			}

			paste_callback_((const char*)data(i));
			return 1; // Fl_Browser doesn't know about paste so don't bother it
		}


		return EDE_Browser::handle(e);
	}

	// Setup callback that will be used when renaming and dnd
	void rename_callback(rename_callback_type* cb) { rename_callback_ = cb; }
	void paste_callback(paste_callback_type* cb) { paste_callback_ = cb; }
	void context_callback(Fl_Callback* cb) { context_callback_ = cb; }

	// Avoid memory leak
	void clear() {
fprintf(stderr, "Call FileView::clear()\n");
		bucket.empty();
		EDE_Browser::clear();
	}
};




class FileView : public Fl_Group {
public:
	FileView(int X, int Y, int W, int H, char*label=0) : Fl_Group(X,Y,W,H,label) {}

	void insert(int row, FileItem *item);
	void add(FileItem *item);
	void remove(FileItem *item);
	void update(FileItem *item);

	void update_path(const char* oldpath,const char* newpath);

	void gray(int row);
	void ungray(int row);

	void rename_callback(rename_callback_type* cb);
	void paste_callback(paste_callback_type* cb);
};


class FileDetailsView : public FileView {
private:
	FileDetailsView_ *browser;
public:
	FileDetailsView(int X, int Y, int W, int H, char*label=0) : FileView(X,Y,W,H,label) {
		browser = new FileDetailsView_(X,Y,W,H,label);
		browser->end();
		end();
	}
//	~FileDetailsView() { delete browser; }

	void insert(int row, FileItem *item) { browser->insert(row,item); }
	void add(FileItem *item) { browser->add(item); }
	void remove(FileItem *item) { browser->remove(item); }
	void update(FileItem *item) { browser->update(item); }

	void update_path(const char* oldpath,const char* newpath) { browser->update_path(oldpath,newpath); }

	void gray(int row) { browser->gray(row); }
	void ungray(int row) { browser->ungray(row); }

	void rename_callback(rename_callback_type* cb) { browser->rename_callback(cb); }
	void paste_callback(paste_callback_type* cb) { browser->paste_callback(cb); }
	void context_callback(Fl_Callback* cb) { browser->context_callback(cb); }

	// Browser methods
	const char* path(int i) { return (const char*)browser->data(i); }
	int size() { return browser->size(); }
	int selected(int i) { return browser->selected(i); }
	void select(int i, int k) { browser->select(i,k); browser->middleline(i); }
	int get_focus() { return browser->get_focus(); }
	void set_focus(int i) { browser->set_focus(i); }
	void remove(int i) { browser->remove(i); }
	void clear() { browser->clear(); }
	void callback(Fl_Callback*cb) { browser->callback(cb); }

	// These methods are used by do_rename
	const char* text(int i) { return browser->text(i); }
	void text(int i, const char* c) { return browser->text(i,c); }
	uchar column_char() { return browser->column_char(); }
};


#endif

/* $Id */
