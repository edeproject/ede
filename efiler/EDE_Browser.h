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


#ifndef EDE_Browser_H
#define EDE_Browser_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Image.H>
#include <FL/fl_draw.H>

#include "Fl_Icon_Browser.h"

enum SortType {
	NO_SORT		= 0, ///< don't sort, items will be shown in order of adding
	ALPHA_SORT	= 1, ///< standard bibliographic sort
	ALPHA_CASE_SORT	= 2, ///< standard bibliographic sort ignoring the case
	NUMERIC_SORT	= 3, ///< items are converted to numbers and sorted by size
	DATE_SORT	= 4, ///< sort by date (depends on user locale)
	FILE_SIZE_SORT	= 5  ///< same as numeric, but also considering units such as kB, MB, GB
};

/**
 * \class EDE_Browser
 * \brief An improved version of Fl_Browser
 *
 * This is a (mostly compatible) version of Fl_Browser that adds
 * following features:
 * * sorting by column, using one of the several offered sort types,
 * * optional header which enables resizing columns and sorting 
 * (by clicking the title of column that you wish to sort),
 * * quick search (by pressing the first character of the item you
 * are searching).
 *
 * The type of browser is set by default to FL_MULTI_BROWSER (call 
 * method type() to change).
 *
 * The class is based on Fl_Icon_Browser (which is just a Fl_Browser
 * that can display icons) that I sincerelly hope will one day go
 * upstream.
 *
*/
class EDE_Browser : public Fl_Icon_Browser {
private:
	// Button row height
	static const int buttonheight=20;

	// Subclass Fl_Group for handle()-ing button resizing and clicks
	class Heading : public Fl_Group {
	public:
		Heading(int x, int y, int w, int h, const char *label = 0) : Fl_Group(x,y,w,h,label) {}
		int handle(int e);
		void draw() {
			if (!visible()) return;
			if (x() && y() && w() && h() && fl_not_clipped(x(),y(),w(),h())) fl_push_clip(x(),y(),w(),h());
			Fl_Group::draw();
			if (x() && y() && w() && h() && fl_not_clipped(x(),y(),w(),h())) fl_pop_clip();
		}
	} *heading;

	Fl_Scrollbar *hscrollbar;

	int totalwidth_; // total width of all columns

	char *column_header_;
	SortType *column_sort_types_;

	// current sort:
	int sort_column;
	SortType sort_type; 
	bool sort_direction;

	void cleanup_header();
	void hide_header();
	void show_header();

	void mqsort(char *labels[], int beg, int end, SortType type); // my implementation of qsort
	bool sortfn(char *,char*,SortType); // sort function, per type

	Fl_Group* thegroup;

public:
	EDE_Browser(int X,int Y,int W,int H,const char *L=0);

	~EDE_Browser() {
		delete[] column_sort_types_;
		cleanup_header();
		delete heading;
		// delete scroll;
		delete hscrollbar;
		if (column_header_) free(column_header_); // this is a C-string
	}

	/**
	 * Get or set the column titles and display the header.
	 *
	 * \param c is formatted just like other items, e.g. it uses the same column_char()
	 * To hide the header, just call with a null pointer (this is the default value).
	 */
	void column_header(const char *c) { 
		free(column_header_);
		if (c) { 
			column_header_=strdup(c);
			show_header();
		} else {
			column_header_=0;
			hide_header();
		} 
	}
	const char *column_header() { return column_header_; }

	/**
	 * Sets the way various columns are sorted. By default all columns will use
	 * ALPHA_SORT e.g. classic bibliographic sort. Usually the programmer knows in 
	 * advance what kind of data will be put into which column, so (s)he can set
	 * how each column will be sorted when the header buttons are clicked.
	 *
	 * \param l constant pointer to a sufficiently large array of SortType, corresponding 
	 * to columns. Last value in this array must be 0 (NO_SORT) (there's no point for
	 * setting sort type of a column to NO_SORT; if you want to unsort the browser, just
	 * call sort(0, NO_SORT) ).
	 */
	void column_sort_types (const SortType *l) { for (int i=0; l[i]; i++) column_sort_types_[i]=l[i]; }

	/**
	 * Sort items by column.
	 *
	 * \param column number of column to use for sorting
	 * \param type type of sorting to be used
	 * \param reverse if true, items will be sorted in reverse order
	 */
	void sort(int column, SortType type, bool reverse=false);

	/**
	 * Sort items by column (toggle-type method). It will use the default sort type for 
	 * given column, as set by column_sort_types(). If column is already sorted in normal 
	 * order, this will resort in reverse - this method gives the same effect as clicking 
	 * on column button.
	 *
	 * \param column number of column to use for sorting
	 */
	void sort(int column);

	/**
	 * Set and get column widths. 
	 * NOTE: *Unlike* Fl_Browser, you need to specify the width of the 
	 * last column as well! If the sum of witdhs is larger than the 
	 * Browser, a horizontal scrollbar will be shown. If it is smaller,
	 * there will be some unused space to the right.
	 *
	 * \param l array of column widths in pixels, ending with zero (please
	 * don't forget to set the last value of the array to 0). If you wish 
	 * to make a column (practically) invisible, set width to 1 pixel.
	 */
	void column_widths(const int* l);
	const int* column_widths() const;

	// Overload handle for keyboard quick search
	int handle(int e);

	// Overload resize for show/hide horizontal scrollbar
	void resize(int X, int Y, int W, int H);

	// Overload hposition(), so that heading scrolls together with browser
	void hposition(int X) {
		if (heading->visible()) {
			static int oldX=0;
			// Move each button by X pixels
			for(int i=0; i<heading->children(); i++) {
				Fl_Widget* c = heading->child(i);
				c->position(c->x()-X+oldX, c->y());
			}
			fl_push_clip(x(),y(),w(),h());
			heading->redraw();
			fl_pop_clip();
			oldX=X;
		}
		Fl_Icon_Browser::hposition(X);
	}

	void hide() { 
		heading->hide(); 
		thegroup->hide(); /* group */
		Fl_Widget::hide(); 
	}

	void show() {
		Fl_Widget::show();
		heading->show();
		thegroup->show();
	}
};

#endif

//
// End of "$Id$".
//

