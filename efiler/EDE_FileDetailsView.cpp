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


#include "EDE_FileView.h"

#include <FL/Fl_Input.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl.H>
#include <FL/filename.H> // used in update_path()

#include <edelib/Nls.h>
#include <edelib/IconTheme.h>
#include <edelib/String.h>
#include <edelib/Debug.h>

#include "EDE_Browser.h"


// debugging help
#define DBG "FileDetailsView: "


// --------------------------------------
//
// 	File renaming stuff
//
// --------------------------------------


// EditBox is displayed when renaming a file
// We subclass Fl_Input so we can handle keyboard
int FileDetailsView::EditBox::handle (int e) {
	FileDetailsView* view = (FileDetailsView*)parent();
	if (e==FL_KEYBOARD && visible()) {
		int k = Fl::event_key();
		if (Fl::event_key()==FL_Enter && visible()) {
			view->hide_editbox();
			view->finish_rename();
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


// show editbox at specified row and make the row "invisible" (bgcolor same as fgcolor)
void FileDetailsView::show_editbox(int row) {
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
	int W=150; 
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
	redraw();
	editbox_->redraw();
}


void FileDetailsView::hide_editbox() {
EDEBUG(DBG "hide_editbox()\n");
	editbox_->hide();

	// Make the edited row visible again
	char* ntext = (char*)malloc(sizeof(char)*(strlen(text(editbox_row))-6)); // remove 7 places for format chars
	strncpy(ntext, text(editbox_row)+7, strlen(text(editbox_row))-7);
	ntext[strlen(text(editbox_row))-7]='\0';
	text(editbox_row,ntext);
	bucket.add(ntext);
}


// This is called to actually rename file (when user presses Enter in editbox)
void FileDetailsView::finish_rename() {
EDEBUG(DBG "finish_rename()\n");
	rename_callback_(editbox_->value()); 
}








// --------------------------------------
//
// 	FileDetailsView
//
// --------------------------------------


// ctor

FileDetailsView::FileDetailsView(int X, int Y, int W, int H, char*label) : EDE_Browser(X,Y,W,H,label) {
//	browser = new EDE_Browser(X,Y,W,H,label);
//	browser->end();
//	end();
//	resizable(browser->the_scroll());
//	resizable(browser->the_group());
//	resizable(browser);

	// EDE_Browser properties
	const int cw[]={250,200,70,130,100,0};
	column_widths(cw);
	column_char('\t');
	column_header(_("Name\tType\tSize\tDate\tPermissions"));
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


// Methods for mainpulating items
// They are named similarly to browser methods, except that they take 
// struct FileItem (defined in EDE_FileView.h)

void FileDetailsView::insert(int row, FileItem *item) {
	// edelib::String doesn't support adding plain char
	char cc[2]; cc[0]=column_char(); cc[1]='\0';

	// Construct browser line
	edelib::String value = item->name + cc + item->description + cc + item->size + cc + item->date + cc + item->permissions;
	char* realpath = strdup(item->realpath.c_str());
	EDE_Browser::insert(row, value.c_str(), realpath); // put realpath into data
	bucket.add(realpath);
EDEBUG(DBG "value: %s\n", value.c_str());

	// Get icon
	edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_TINY);
	if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_TINY,edelib::ICON_CONTEXT_MIMETYPE);
	set_icon(row, Fl_Shared_Image::get(icon.c_str()));
}


void FileDetailsView::update(FileItem *item) {
	int row=findrow(item->realpath);
	if (row==0) return;

	//EDE_Browser::remove(row);
	//insert(row, item);
	//  the above was reimplemented because a) it's unoptimized, b) adds everything at the end, 
	//  c) causes browser to lose focus, making it impossible to click on something while
	//  directory is loading

	// edelib::String doesn't support adding plain char
	char cc[2]; cc[0]=column_char(); cc[1]='\0';

	// Construct browser line
	edelib::String value = item->name + cc + item->description + cc + item->size + cc + item->date + cc + item->permissions;
	char* realpath = strdup(item->realpath.c_str());
	text(row, value.c_str());
	data(row, realpath);
	bucket.add(realpath);
EDEBUG(DBG "value: %s\n", value.c_str());

	// Get icon
	edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_TINY);
	if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_TINY,edelib::ICON_CONTEXT_MIMETYPE);
	set_icon(row, Fl_Shared_Image::get(icon.c_str()));
}

// This method is needed because update() uses path to find item in list
void FileDetailsView::update_path(const char* oldpath,const char* newpath) {
	int row=findrow(oldpath);
	if (row==0) return;
	char* c = strdup(newpath);
	data(row,c);
	bucket.add(c);

	// Update filename in list
	char* oldline = strchr(text(row), column_char());
	edelib::String line = fl_filename_name(newpath);
	line += oldline;
	text(row, line.c_str());
}



// Make row look "disabled" (actually we use it for cut operation)

void FileDetailsView::gray(int row) {
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

void FileDetailsView::ungray(int row) {
	if (text(row)[0] != '@' || text(row)[1] != 'C') return;  // not greyed

	char *ntext = (char*)malloc(sizeof(char)*(strlen(text(row))-5)); // remove 6 places for format chars
	strncpy(ntext, text(row)+6, strlen(text(row))-6);
	ntext[strlen(text(row))-6]='\0';
	text(row,ntext);
	bucket.add(ntext);

	// doesn't work
	Fl_Image* im = get_icon(row)->copy();
	//im->refresh();
	//im->uncache(); // doesn't work
	//im->color_average(FL_BACKGROUND_COLOR, 1.0);
	//im->data(im->data(),im->count());
	set_icon(row,im);

	//redraw(); // OPTIMIZE
}



// Overloaded handle for: context menu, file renaming and dnd support

int FileDetailsView::handle(int e) {
//EDEBUG(DBG"Event: %d\n", e);

	/* ------------------------------
		Right-click event
	--------------------------------- */

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


	/* ------------------------------
		Rename support
	--------------------------------- */

	// If rename_callback_ isn't set, we don't support renaming
	if (rename_callback_) {

		// hide editbox when user clicks outside of it
		if (e==FL_PUSH && editbox_->visible() && !Fl::event_inside(editbox_))
			hide_editbox();
		
		// hide editbox when scrolling mouse
		if (e==FL_MOUSEWHEEL && editbox_->visible())
			hide_editbox(); 
		
		
		static bool renaming=false; 
			// this variable is used because we want to select item on FL_PUSH, but
			// actually start editing on FL_RELEASE (otherwise it would be impossible
			// to doubleclick)

		if (e==FL_PUSH) renaming=false;

		// Click once on item that is already selected AND focused to rename it
		if (e==FL_PUSH && !editbox_->visible() && Fl::event_clicks()==0 && Fl::event_button()==1) {

			// we're only interested in the first column
			// columns 2-5 are "safe haven" where user can click without renaming anything
			const int* l = column_widths();
			if (Fl::event_x()<x() || Fl::event_x()>x()+l[0])
				return EDE_Browser::handle(e); 

			// Is clicked item also focused?
			void* item = item_first();
			int focusy=y()-position();
			for (int i=1; i<get_focus(); i++) {
				focusy+=item_height(item);
				if (focusy>Fl::event_y()) break;
				item=item_next(item);
			}
			if (Fl::event_y()<focusy || Fl::event_y()>focusy+item_height(item))
				// It isn't
				return EDE_Browser::handle(e); 
			if (selected(get_focus())!=1)
				// If it's focused but not selected, then this action is select
				return EDE_Browser::handle(e); 
			
			renaming=true; // On next event, we will check if it's doubleclick
		}
		
		if (renaming && (e==FL_DRAG || e==FL_RELEASE || e==FL_MOVE) && Fl::event_is_click()==0) {
			// Fl::event_is_click() will be >0 on doubleclick
			show_editbox(get_focus());
			renaming=false;
			// don't pass mouse event, otherwise item will become reselected which is a bit ugly
			return 1;
		}
	}


	/* ------------------------------
		Drag&drop support
	--------------------------------- */

	// If paste_callback_ isn't set, we can't support dnd
	if (paste_callback_) {

		// Let others know that we accept dnd
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
	
		static int dragx,dragy; // to check min drag distance
		if (e==FL_DRAG) {
			edelib::String selected_items;
			for (int i=1; i<=size(); i++)
				if (selected(i)==1) {
					selected_items += "file://";
					selected_items += (char*)data(i);
					selected_items += "\r\n";
				}
EDEBUG(DBG "DnD buffer: '%s'\n", selected_items.c_str());
			Fl::copy(selected_items.c_str(),selected_items.length(),0);
			Fl::dnd();
			dragx = Fl::event_x(); dragy = Fl::event_y();
			return 1; // don't do the multiple selection thing from Fl_Browser
		}

		static bool dndrelease=false; // so we know if paste is from dnd or not
		if (e==FL_DND_RELEASE) {
EDEBUG(DBG "FL_DND_RELEASE '%s'\n", Fl::event_text());

			// Sometimes drag is accidental
			if (abs(Fl::event_x()-dragx)>MIN_DISTANCE_FOR_DND || abs(Fl::event_y()-dragy)>MIN_DISTANCE_FOR_DND) {
				dndrelease=true;
				Fl::paste(*this,0);
			}
			return 0;
			// return 1 would call Fl::paste(*belowmouse(),0) (see fl_dnd_x.cxx around line 168).
			// In our case that could be catastrophic
		}

		if (e==FL_PASTE) {
EDEBUG(DBG "FL_PASTE\n");
			if (!Fl::event_text() || !Fl::event_length()) return 1;
EDEBUG(DBG "-- '%s' (%d)\n",Fl::event_text(),Fl::event_length());

			// Paste is coming from menu/keyboard
			if (!dndrelease) {
				paste_callback_(0); // 0 = current dir
				return 1;
			}
			dndrelease=false;

			// Where is user dropping?
			// If it's not the first column, we assume current directory
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
			return 1;
		}
	} // if (paste_callback_) ...

	return EDE_Browser::handle(e);
}


/* $Id */
