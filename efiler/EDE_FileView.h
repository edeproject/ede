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
typedef void (paste_callback_type)(const char*,const char*);



class FileDetailsView : public EDE_Browser {
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


	// Subclass Fl_Input so we can handle keyboard
	class EditBox : public Fl_Input {
		friend class FileDetailsView;
	public:
		EditBox(int x, int y, int w, int h, const char* label = 0) : Fl_Input(x,y,w,h,label)  {}
		int handle (int e) {
			FileDetailsView* view = (FileDetailsView*)parent();
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

	rename_callback_type* rename_callback_;
	paste_callback_type* dnd_callback_;

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

		// make the row "invisible"
		editbox_row=row;
		char* ntext = (char*)malloc(sizeof(char)*strlen(text(row))+5); // add 4 places for format chars
		strncpy(ntext+5, text(row), strlen(text(row)));
		ntext[0]='@'; ntext[1]='C'; ntext[2]='2'; ntext[3]='5'; ntext[4]='5';
		text(row,ntext);
		free(ntext);


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
		char* ntext = (char*)malloc(sizeof(char)*strlen(text(editbox_row))-5); // add 4 places for format chars
		strncpy(ntext, text(editbox_row)+5, strlen(text(editbox_row))-5);
		text(editbox_row,ntext);
		free(ntext);
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
			Bucket() : size(0), capacity(1000), items((void**)malloc(sizeof(void*)*1000)) {
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
	FileDetailsView(int X, int Y, int W, int H, char*label=0) : EDE_Browser(X,Y,W,H,label) {
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
		textsize(12); // FIXME: hack for font size
		const SortType st[]={ALPHA_CASE_SORT, ALPHA_CASE_SORT, FILE_SIZE_SORT, DATE_SORT, ALPHA_SORT, NO_SORT};
		column_sort_types(st);
		when(FL_WHEN_ENTER_KEY_ALWAYS);
		//when(FL_WHEN_ENTER_KEY_CHANGED);
		//first=0;

		editbox_ = new EditBox(0, 0, 0, 0);
		editbox_->box(FL_BORDER_BOX);
		editbox_->parent(this);
		editbox_->textsize(12); // FIXME: hack for font size
		editbox_->hide();

		rename_callback_ = 0;
		dnd_callback_ = 0;
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
		// this was reimplemented because a) it's unoptimized, b) adds stuff at the end, 
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

	// Change color of row to gray
	void gray(int row) {
		if (text(row)[0] == '@' && text(row)[1] == 'C') return; // already greyed

		char *ntext = (char*)malloc(sizeof(char)*strlen(text(row))+5); // add 4 places for format chars
		strncpy(ntext+4, text(row), strlen(text(row)));
		ntext[0]='@'; ntext[1]='C'; ntext[2]='2'; ntext[3]='5'; // @C25 - nice shade of gray
		ntext[strlen(text(row))+4]='\0'; // in case text(row) was broken
		text(row,ntext);
		bucket.add(ntext);

		// grey icon - but how to ungray?
		Fl_Image* im = get_icon(row)->copy();
		im->inactive();
		set_icon(row,im);

	}

	void ungray(int row) {
		if (text(row)[0] != '@' || text(row)[1] != 'C') return;  // not greyed

		char *ntext = (char*)malloc(sizeof(char)*strlen(text(row))-4); // 4 places for format chars
		strncpy(ntext, text(row)+4, strlen(text(row))-4);
		text(row,ntext);
		bucket.add(ntext);

		// don't work
		//Fl_Image* im = get_icon(row);
		//im->uncache(); // doesn't work

		//redraw(); // OPTIMIZE
	}

	int handle(int e) {
		// Rename support
		if (e==FL_KEYBOARD) {
			if (Fl::event_key()==FL_F+2) {
				if (editbox_->visible())
					hide_editbox();
				else
					show_editbox(get_focus());
			}
		}
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
			
			void* item = item_first();
			int focusy=y()-position();
			for (int i=1; i<get_focus(); i++) {
				focusy+=item_height(item);
				if (focusy>Fl::event_y()) break;
				item=item_next(item);
			}
			if (Fl::event_y()<focusy || Fl::event_y()>focusy+item_height(item))
				return Fl_Icon_Browser::handle(e); // Click outside selected item
			if (selected(get_focus())!=1)
				return Fl_Icon_Browser::handle(e); // allow to select item if it's just focused
			
			renaming=true;
		}
		if (e==FL_RELEASE && renaming && Fl::event_clicks()==0) {
			show_editbox(get_focus());
			renaming=false;
			return 1; // don't pass mouse event, otherwise item will become selected again
		}

		// Drag&drop support
		static int paste_event_y;

		/*--- This is to get dnd events from non-fltk apps ---
		static bool dragging=false;
		if (e==FL_PUSH) dragging=false;
		if (e==FL_DND_ENTER) dragging=true;
		if (e==FL_RELEASE && dragging) {
			paste_event_y=Fl::event_y();
			Fl::paste(*this,0);
			dragging=false;
		}
		/*--- End ugly hack ---*/

		// Don't unselect on FL_PUSH cause that could be dragging
		if (e==FL_PUSH && Fl::event_clicks()!=1) return 1;

		if (e==FL_DRAG) { 
			edelib::String selected_items;
			for (int i=1; i<=size(); i++)
				if (selected(i)==1) {
					if (selected_items != "") selected_items += ",";
					selected_items += (char*)data(i);
				}
			Fl::copy(selected_items.c_str(),selected_items.length(),0);
			Fl::dnd();
			return 1; // don't do the multiple selection thing from Fl_Browser
		}
		
		if (e==FL_DND_RELEASE) {
			paste_event_y=Fl::event_y();
			Fl::paste(*this,0);
		}
		if (e==FL_PASTE) {
			if (!Fl::event_text() || !Fl::event_length()) return 1;

			// Where is the user dropping?
			void* item = item_first();
			int itemy=y()-position();
			int i;
			for (i=1; i<=size(); i++) {
				itemy+=item_height(item);
				if (itemy>paste_event_y) break;
			}
			dnd_callback_(Fl::event_text(),(const char*)data(i));
		}
//		if (e==FL_DND_ENTER) { take_focus(); Fl::focus(this); Fl::belowmouse(this); Fl_Icon_Browser::handle(FL_FOCUS); }
//		if (e==FL_DND_LEAVE) { take_focus(); Fl::focus(this); Fl::belowmouse(this); Fl_Icon_Browser::handle(FL_FOCUS); }
		//fprintf (stderr, "Event: %d\n", e);

		return Fl_Icon_Browser::handle(e);
	}

	// Setup callback that will be used when renaming and dnd
	void rename_callback(rename_callback_type* cb) { rename_callback_ = cb; }
	void dnd_callback(paste_callback_type* cb) { dnd_callback_ = cb; }

	// Avoid memory leak
	void clear() {
fprintf(stderr, "Call FileView::clear()\n");
		bucket.empty();
		EDE_Browser::clear();
	}
};


#endif

/* $Id */
