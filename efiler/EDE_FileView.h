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



//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////



/*// Event handler for EditBox
int EditBox::handle(int event) {
	if (!this->visible()) return parent()->handle(event);
	bool above=false;
	//fprintf(stderr,"Editbox event: %d (%s)\n",event,event_name(event));

	// Change filename
	if (event==KEY && (event_key()==ReturnKey || event_key()==KeypadEnter)) {
		// split old filename to path and file
		char path[PATH_MAX], file[PATH_MAX];
		strcpy(path, (char*)editing_->user_data());
		if (path[strlen(path)-1] == '/') path[strlen(path)-1]='\0';
		char *p = strrchr(path,'/');
		if (p==0 || *p=='\0') {
			strcpy(file,path);
			path[0]='\0';
		} else { // usual case
			p++;
			strcpy(file,p);
			*p='\0';
		}

		if (strlen(file)!=strlen(text()) || strcmp(file,text())!=0) {
			// Create new filename
			strncat(path, text(), PATH_MAX-strlen(path));
			char oldname[PATH_MAX];
			strcpy(oldname, (char*)editing_->user_data());
			if (rename(oldname, path) == -1) {
				alert(tsprintf(_("Could not rename file! Error was:\n\t%s"), strerror(errno)));
			} else {
				// Update browser
				free(editing_->user_data());
				editing_->user_data(strdup(path));
				const char* l = editing_->label();
				editing_->label(tasprintf("%s%s",text(),strchr(l, '\t')));
			}
		}
		
		above=true;
	}

	// Hide editbox 
	if (above || ( event==KEY && event_key()==EscapeKey ) ) {
		this->hide();
		return 1;
	}
	Input::handle(event);
}


// We override hide method to ensure certain things done
void EditBox::hide() {
	Input::hide();
	// Remove box so it doesn't get in the way
	this->x(0);
	this->y(0);
	this->w(0);
	this->h(0);
	// Return the browser item into "visible" state
	if (editing_) {
		editing_->textcolor(textcolor());
		editing_->redraw();
		parent()->take_focus();
	}
}

*/
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

// Type for rename_callback
// I don't know how to do this without creating a new type :(
typedef void (my_callback)(const char*);



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
			return Fl_Input::handle(e);
		}
	}* editbox_;
	int editbox_row;

	my_callback* rename_callback_;

	// show editbox at specified row and make the row "invisible" (bgcolor same as fgcolor)
	void show_editbox(int row) {
		if (row<1 || row>size()) return; // nothing selected
		if (strcmp(text(row), "..")==0) return; // can't rename "go up" button

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
	}
//	~FileDetailsView() { delete browser; }

	void insert(int row, FileItem *item) {
		// Construct browser line
		edelib::String value;
		value = item->name+"\t"+item->description+"\t"+item->size+"\t"+item->date+"\t"+item->permissions;
		EDE_Browser::insert(row, value.c_str(), strdup(item->realpath.c_str())); // put realpath into data
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
	void update(FileItem *item) {
		int row=findrow(item->realpath);
		if (row==0) return;
		EDE_Browser::remove(row);
		insert(row, item);
		// FIXME: this will lose focus, making it impossible to click on something while
		// directory is loading
	}

	// Change color of row to gray
	void gray(int row) {
		if (text(row)[0] == '@' && text(row)[1] == 'C') return; // already greyed

		char *ntext = (char*)malloc(sizeof(char)*strlen(text(row))+4); // add 4 places for format chars
		strncpy(ntext+4, text(row), strlen(text(row)));
		ntext[0]='@'; ntext[1]='C'; ntext[2]='2'; ntext[3]='5';
		text(row,ntext);
		free(ntext);

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
		free(ntext);

		// don't work
		//Fl_Image* im = get_icon(row);
		//im->uncache(); // doesn't work

		//redraw(); // OPTIMIZE
	}

	// Renaming support
	int handle(int e) {
		if (e==FL_KEYBOARD) {
			if (Fl::event_key()==FL_F+2) {
				if (editbox_->visible())
					hide_editbox();
				else
					show_editbox(get_focus());
			}
		}
		if (e==FL_PUSH && editbox_->visible() && !Fl::event_inside(editbox_))
 			hide_editbox(); // hide editbox when click outside it
		return Fl_Icon_Browser::handle(e);
	}

	// Setup callback that will be used when renaming
	void rename_callback(my_callback* cb) { rename_callback_ = cb; }
};


#endif

/* $Id */
