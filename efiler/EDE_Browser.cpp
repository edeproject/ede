/*
 * $Id$
 *
 * EDE Browser class
 * Part of edelib.
 * Copyright (c) 2005-2007 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#include "EDE_Browser.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <Fl/Fl.H>
#include <Fl/Fl_Window.H>



// ---------------- SORT ----------------


// convert size string to float - used for sorting files by size
double sizetof(char *s) {
	double d=atof(s);
	if (strchr(s,'g') || strchr(s,'G'))
		d*=1024*1024*1024;
	else if (strchr(s,'m') || strchr(s,'M'))
		d*=1024*1024;
	else if (strchr(s,'k') || strchr(s,'K'))
		d*=1024;
	return d;
}

// Sort function - return true if first is larger then second, otherwise false
bool EDE_Browser::sortfn(char*s1,char*s2,SortType type) {
	switch(type) {
	  case ALPHA_SORT:
		return (strcmp(s1,s2)>0);
	  case ALPHA_CASE_SORT:
		return (strcasecmp(s1,s2)>0);
	  case NUMERIC_SORT:
		return (atof(s1)>atof(s2));
	  case DATE_SORT:
		// unimplemented
		// TODO: use edelib::Date
		return false;
	  case FILE_SIZE_SORT:
		return (sizetof(s1)>sizetof(s2));
	  default:
		return false; // NO_SORT
	}
}


// Optimized quick sort algorithm 
void EDE_Browser::mqsort(char* arr[], int beg, int end, SortType type) {
	bool k = sort_direction ? false : true;
	if (end > beg + 1)
	{
		char* piv = arr[beg]; int l = beg + 1, r = end;
		while (l < r)
		{
			if (k^sortfn(arr[l],piv,type)) // ^ is XOR
				l++;
			else if (l==r-1)  // avoid costly swap if they're the same
				r--;
			else {
				swap(--r,l); // Fl_Browser::swap()
				char *tmp=arr[l]; // update array
				arr[l]=arr[r];
				arr[r]=tmp;
			}
		}
		// avoid costly swap if they're the same
		if (beg==l-1) 
			l--;
		else {
			swap(--l,beg);
			char*tmp=arr[beg]; // update array
			arr[beg]=arr[l];
			arr[l]=tmp;
		}

		// recursion
		mqsort(arr, beg, l, type);
		mqsort(arr, r, end, type);
	}
}



// callback for header buttons
void header_callback(Fl_Widget*w, void*) {
	Fl_Group* heading = (Fl_Group*)w->parent();
	EDE_Browser* browser = (EDE_Browser*)w->parent()->parent();
	for (int i=heading->children(); i--; )
		if (w == heading->child(i)) { browser->sort(i); break; }
}

// Toggle-type method
void EDE_Browser::sort(int column) {
	SortType t = column_sort_types_[column];
	if (t==NO_SORT) t=ALPHA_SORT;
	if (column!=sort_column || sort_type==NO_SORT)
		sort(column, t, false);
	else
		sort(column, sort_type, !sort_direction);
}

// Real sorting method with three parameters
#define SYMLEN 6
void EDE_Browser::sort(int column, SortType type, bool reverse) {
	char *h=column_header_;
	int hlen=strlen(h);
	char colchar = Fl_Icon_Browser::column_char();

	// FIXME sort() shouldn't call column_header() because that calls show_header() and that
	// deletes all buttons from header (so they could be recreated). This cause valgrind errors
	// since sort is called in button callback - you can't delete a widget in its own callback

	// Remove old sort direction symbol (if any) from header
	char *delim = 0;
	int col=0;
	if (sort_type != NO_SORT) {
		bool found=false;
		while ((delim=strchr(h, colchar))) {
			if (col==sort_column) {
				for (uint i=0; i<=strlen(delim); i++) delim[i-SYMLEN+1]=delim[i];
				found=true;
				break;
			}
			h=delim+1;
			col++;
		}
		if (!found && col==sort_column)
			column_header_[hlen-SYMLEN+1]='\0';
		h=column_header_;
		delim = 0;
		col=0;
	}

	// Add new symbol
	char *newheader = new char[hlen+6];
	if (type != NO_SORT) {
		// Construct symbol
		char sym[SYMLEN];
		if (reverse) snprintf(sym,SYMLEN,"@-22<");
		else snprintf(sym,SYMLEN,"@-22>");

		// Find column
		bool found=false;
		while ((delim=strchr(h, colchar))) {
			if (col==column) {
				*delim='\0';
				snprintf(newheader,hlen+SYMLEN,"%s%s\t%s",column_header_,sym,delim+1);
				found=true;
				break;
			}
			h=delim+1;
			col++;
		}
		if (!found && col==column) // Just append symbol to string
			snprintf(newheader, hlen+SYMLEN,"%s%s",column_header_,sym);
	} else {
		strncpy (newheader, column_header_, hlen);
		newheader[hlen]='\0';
	}
	column_header(newheader);
	delete[] newheader;
	sort_column=column; sort_type=type; sort_direction=reverse;

	// Start actually sorting
	if (type != NO_SORT) {
		// First create an array of strings in a given column
		char** sorttext = (char**)malloc(sizeof(char*)*(size()+1)); // index starts from 1 for simplicity in mqsort
		for (int i=1;i<=size();i++) {
			char *tmp = strdup(text(i));
			char *l = tmp;
			int col=0;
			while ((delim=strchr(l, Fl_Icon_Browser::column_char()))) {
				*delim = '\0';
				if (col==column) break;
				l=delim+1;
				col++;
			}
			sorttext[i] = strdup(l);
			free(tmp);
		}

		mqsort(sorttext, 1, size()+1, type);
		redraw();

		// Free the allocated memory
		for (int i=1; i<=size(); i++) free (sorttext[i]);
		free(sorttext);
	}
}


// --------------------------------------


static void scroll_cb(Fl_Widget* w, void*) {
	Fl_Scrollbar *s = (Fl_Scrollbar*)w;
	EDE_Browser *b = (EDE_Browser*)w->parent();
	b->hposition(s->value());
}


// ctor
EDE_Browser::EDE_Browser(int X,int Y,int W,int H,const char *L) : Fl_Icon_Browser(X,Y,W,H),
	  totalwidth_(0), column_header_(0), sort_column(0), sort_type(NO_SORT), sort_direction(false) {

	Fl_Group* thegroup = new Fl_Group(X,Y,W,H);
	thegroup->begin();
		heading = new Heading(0,0,W,buttonheight);
 		heading->box(FL_FLAT_BOX); // draw heading background
		heading->align(FL_ALIGN_CLIP);
		heading->end();
		heading->hide();
		heading->parent(this); // for callback
	
		hscrollbar = new Fl_Scrollbar(1, H-Fl::scrollbar_size()-2, W-Fl::scrollbar_size()-3, Fl::scrollbar_size()); // take account for edges
		hscrollbar->type(FL_HORIZONTAL);
		hscrollbar->hide();
		hscrollbar->parent(this); // for callback
		hscrollbar->callback(scroll_cb);
	thegroup->end();
	thegroup->add(this);

	has_scrollbar(VERTICAL);

	resizable(0);
	thegroup->resizable(this);
	thegroup->align(FL_ALIGN_CLIP);

	// EDE_Browser is always a multiple-selection browser 
	type(FL_MULTI_BROWSER);

	column_sort_types_ = new SortType[256]; // 256 columns should be enough for anyone (tm)
	for (int i=0; i<256; i++) column_sort_types_[i]=NO_SORT;
}


// Deallocate all memory used by header labels
void EDE_Browser::cleanup_header() {
	// Deallocate old button labels
	for (int i=0; i<heading->children(); i++) {
		char *l = (char*)heading->child(i)->label();
		if (l && l[0]!='\0') free(l);
	}
	heading->clear();
}

//make buttons invisible
void EDE_Browser::hide_header() {
	if (heading->visible()) resize(x(),y()-buttonheight,w(),h()+buttonheight);
	cleanup_header();
	heading->hide();
}

// Regenerate and display header
void EDE_Browser::show_header() {
	int button_x=0;
	char *hdr = column_header_;
	const int* l = Fl_Icon_Browser::column_widths();
	cleanup_header();
	for (int i=0; i==0||l[i-1]; i++) {
		// If the button is last, calculate size
		int button_w = l[i];
		if (button_w == 0) button_w = totalwidth_-button_x;

		// Get part of header until delimiter char
		char *delim = 0;
		Fl_Button *b;

		if (hdr) {
			delim = strchr(hdr, Fl_Icon_Browser::column_char());
			if (delim) *delim='\0'; // temporarily
			b=new Fl_Button(button_x,heading->y(),button_w,buttonheight,strdup(hdr));
		} else {
			b=new Fl_Button(button_x,heading->y(),button_w,buttonheight,"");
		}

		b->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_CLIP);
		b->callback(header_callback);
		b->labelsize(12); // FIXME: hack for label size
		//b->labelcolor(FL_DARK3);
		heading->add(b);
		button_x += l[i];

		if (delim) {
			*delim=Fl_Icon_Browser::column_char(); // put back delimiter
			hdr=delim+1; // next field
		}
	}
	if (!heading->visible()) resize(x(),y()+buttonheight,w(),h()-buttonheight);
	heading->resizable(0); // We will resize the heading and individual buttons manually
	heading->show();
	heading->redraw(); //in case it was already visible
}

// Subclassed to resize heading and browser if neccessary
void EDE_Browser::column_widths(const int* l) {
	totalwidth_=0;
	int i; 
	for (i=0; l[i]; i++) totalwidth_+=l[i];
//	if (total>=scroll->w()) {
//		Fl_Icon_Browser::size(total,h());
//	}
	// If there was heading before, regenerate
	if (heading->visible())
		heading->size(totalwidth_,buttonheight);
//		show_header();

	// Second array for the Fl_Browser
	static int *tmp = 0;
	if (tmp) delete[] tmp;
	tmp=new int[i]; // FIXME: Dtor should cleanup this memory
	for (int j=0; j<i-1; j++) tmp[j]=l[j];
	tmp[i-1]=0;
	Fl_Icon_Browser::column_widths(tmp);
	// delete[] tmp; -- can't do this, browser goes berserk

	// Redraw parent so we don't get ugly artifacts after shrinking last button
	// Doesn't work anymore!
//	parent()->redraw();
}

const int* EDE_Browser::column_widths() const {
	int i,total=0;
	const int *l=Fl_Icon_Browser::column_widths();
	for (i=0; l[i]; i++) total+=l[i];

	static int *tmp = 0;
	if (tmp) delete[] tmp;
	tmp=new int[i+2]; // FIXME: Someone should cleanup this memory sometimes...
	for (int j=0; l[j]; j++) tmp[j]=l[j];

	tmp[i]=(totalwidth_-total);
	tmp[i+1]=0;
	return tmp;
}


// Subclassed handle() for keyboard searching
int EDE_Browser::handle(int e) {
	if (e==FL_FOCUS) { fprintf(stderr, "EB::focus\n"); }
	if (e==FL_KEYBOARD && Fl::event_state()==0) {
		// when user presses a key, jump to row starting with that character
		int k=Fl::event_key();
		if ((k>='a'&&k<='z') || (k>='A'&&k<='Z') || (k>='0'&&k<='9')) {
			if (k>='A'&&k<='Z') k+=('a'-'A');
			int ku = k - ('a'-'A'); //upper case
			int p=lineno(selection());
			for (int i=1; i<=size(); i++) {
				int mi = (i+p-1)%size() + 1; // search from currently selected one
				if (text(mi)[0]==k || text(mi)[0]==ku) {
					// select(line,0) just moves focus to line without selecting
					// if line was already selected, it won't be anymore
					select(mi,selected(mi));
					middleline(mi);
					//break;
					return 1; // menu will get triggered on key press :(
				}
			}
		}
		// Attempt to fix erratic behavior on enter key
		// Fl_Browser seems to do the following on enter key:
		// - when item is both selected and focused, callback isn't called at all (even FL_WHEN_ENTER_KEY_ALWAYS)
		// - when no item is selected, callback is called 2 times on focused item
		// - when one item is selected and other is focused, callback is first called on selected then on
		//   focused item, then the focused becomes selected
		// This partial fix at least ensures that callback is always called. Callback function should
		// deal with being called many times repeatedly.
		if ((when() & FL_WHEN_ENTER_KEY_ALWAYS) && k == FL_Enter) {
//			if (changed()!=0) {
				//fprintf(stderr,"do_callback()\n"); 
				do_callback();
//			}
		}

		if (k == FL_Tab) {
fprintf (stderr, "TAB\n");
			
//			Fl_Icon_Browser::handle(FL_UNFOCUS); return 1;
		}
	}
	return Fl_Icon_Browser::handle(e);
}

// Overload resize for show/hide horizontal scrollbar
void EDE_Browser::resize(int X, int Y, int W, int H) {
	int fsbs = Fl::scrollbar_size();
	if (W >= totalwidth_) {
		// hide scrollbar
		if (hscrollbar->visible())
			hscrollbar->hide();
		H += fsbs;
	} else {
		// show scrollbar
		hscrollbar->value(hscrollbar->value(), W, 0, totalwidth_);
		if (!hscrollbar->visible()) {
			hscrollbar->resize(X+1, Y+H-fsbs-2, W-fsbs-3, fsbs);
			hscrollbar->show();
			H -= fsbs;
		} else {
			hscrollbar->resize(X+1, Y+H-2, W-fsbs-3, fsbs);
			hscrollbar->redraw();
		}
	}

	heading->resize(X, Y-buttonheight, W, buttonheight);
//	else
//		heading->position(X, Y);
	Fl_Icon_Browser::resize(X,Y,W,H);
}




// *****************
// class Heading - implementation
// *****************

// The following code is modified from Fl_Tile.cxx and extensively commented, as I was trying
// to figure it out ;)

static void set_cursor(Fl_Group*t, Fl_Cursor c) {
	static Fl_Cursor cursor;
	if (cursor == c || !t->window()) return;
	cursor = c;
#ifdef __sgi
	t->window()->cursor(c,FL_RED,FL_WHITE);
#else
	t->window()->cursor(c);
#endif
}

static Fl_Cursor cursors[4] = {
	FL_CURSOR_DEFAULT,
	FL_CURSOR_WE,
	FL_CURSOR_NS,
	FL_CURSOR_MOVE
};

int EDE_Browser::Heading::handle(int event) {

	static int sdrag; // Type of drag
	static int sdx; // Event distance from the widget boundary
	static int sx; // Original event x
#define DRAGH 1
#define DRAGV 2
               
// width of grabbing area in pixels:
#define GRABAREA 4

	// Event coordinates
	int evx = Fl::event_x();
	int evy = Fl::event_y();

	if (event==FL_FOCUS) return 0; // this is a focusless widget!	
// 	if (event==FL_FOCUS && Fl::event_key()==FL_Tab) {
// 		parent()->take_focus(); // heading shouldn't take focus
// 		return 1;
// 	}

	switch (event) {

		case FL_MOVE:
		case FL_ENTER:
		case FL_PUSH: {
			int mindx = 100;
//			int mindy = 100;
//			int oldx = 0;
//			int oldy = 0;
			
			Fl_Widget*const* a = array();
			
			// Is there a button boundary within GRABAREA ?
			for (int i=0; i<children(); i++) {
				Fl_Widget* o = *a++;
				if (o == resizable()) continue; // resizable has a special meaning in Fl_Tile, but we don't use it in Heading
				if (o->y()<=evy+GRABAREA && o->y()+o->h()>=evy-GRABAREA) {
					int t = evx - (o->x()+o->w());
					if (abs(t) < mindx) {
						sdx = t;
						mindx = abs(t);
					}
				}
			}
			
			sdrag = 0; sx = 0;
			if (mindx <= GRABAREA) {sdrag = DRAGH; sx = evx;}
			set_cursor(this, cursors[sdrag]);
			if (sdrag) return 1;
			return Fl_Group::handle(event);
		}

		case FL_LEAVE:
			set_cursor(this, FL_CURSOR_DEFAULT);
			break;

		case FL_DRAG:
			// This is necessary if CONSOLIDATE_MOTION in Fl_x.cxx is turned off:
			// if (damage()) return 1; // don't fall behind
		case FL_RELEASE: {
			if (!sdrag) return 0; // should not happen
			Fl_Widget* r = resizable(); if (!r) r = this;
			
			// Calculate where the new boundary (newx) should be 
			int newx; 
			if (sdrag&DRAGH) {
				newx = Fl::event_x()-sdx;
				if (newx < r->x()) newx = r->x();
				else if (newx > r->x()+r->w()) newx = r->x()+r->w();
			}
			
			// Mouse movement distance (dx)
			int dx = Fl::event_x()-sx;
			
			Fl_Widget*const* a = array();
			
			// Here we check if any widget will get size 0 
			// because column size 0 is illegal in Browser
			for (int i=children(); i--; ) {
				Fl_Widget* o = a[i];
				int end = o->x()+o->w(); // End coord. of widget
				if ((end == newx-dx || newx<1) && o->w()+dx < 1) {
					set_changed();
					do_callback();
					set_cursor(this, FL_CURSOR_DEFAULT);
					return 1;
				}
			}

			if (newx>=w()) newx+=dx; // last button is resized by dragging the edge
			
			// Go again through list of widgets and resize everything
			int *columns = new int[children()+1];
			int j=0;
			for (int i=children(); i--; ) {
				Fl_Widget* o = *a++;
				int end = o->x()+o->w(); // End coord. of widget
				
				if (end == newx-dx) { 
					// Resize left widget
					o->damage_resize(   o->x(), o->y(), o->w()+dx, o->h());
				} else if (end > newx-dx) { 
					// Resize all remaining widgets to the right
					o->damage_resize(o->x()+dx, o->y(), o->w(), o->h());
				}
				// Push new width into the columns array
				columns[j++]=o->w();
			}
			columns[j]=0;

			// This is the EDE_Browser method. It will also resize the heading and
			// the browser if neccessary
			EDE_Browser*b = (EDE_Browser*)parent(); 
			b->column_widths(columns);
			b->redraw(); // OPTIMIZE (some smart damage in column_widths() ?)
			delete[] columns;
			
			// There will be many RELEASE events, so we update sx (used when calculating dx)
			sx=Fl::event_x();
			
			if (event == FL_DRAG) set_changed();
			do_callback();
			return 1;
		}
	}

	return Fl_Group::handle(event);
}
