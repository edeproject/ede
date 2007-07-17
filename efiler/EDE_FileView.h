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
/*	struct myfileitem* finditem(edelib::String realpath) {
		if (!firstitem) return;
		struct myfileitem* work = firstitem;
		do {
			if (work->item->realpath == realpath) return work;
		} while (work=work->next != 0);
		return 0;
	}*/
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
//		browser->redraw();
/*		struct myfileitem* i;
		if (!first)
			i = first = new myfileitem;
		else {
			i=first;
			while (i->next!=0) i=i->next;
			i->next = new myfileitem;
			i->next->previous = i;
			i=i->next;
		}
		i->item = item;
		i->next = 0;*/
//	}
	void remove(FileItem *item) {
		int row = findrow(item->realpath);
		if (row) EDE_Browser::remove(row);
/*		struct myfileitem* i = finditem(item->realpath);
		if (i) {
			i->previous->next = i->next;
			i->next->previous = i->previous;
			delete i;
		}*/
	}
	void update(FileItem *item) {
		int row=findrow(item->realpath);
		if (row==0) return;
		EDE_Browser::remove(row);
		insert(row, item);
/*
		//
		edelib::String value = item->name + "\t" + item->description + "\t" + item->size + "\t" + item->date + "\t" + item->permissions;
		browser->insert(row,value.c_str(),strdup(item->realpath.c_str()));

		edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_TINY);
		browser->set_icon(row, Fl_Shared_Image::get(icon.c_str()));

		browser->redraw(); // OPTIMIZE
//		browser->redraw_line(browser->find_item(row));*/
	}

	// Change color of row to gray
	void gray(int row) {
		char *ntext = strdup(text(row));
		if (ntext[0] == '@' && ntext[1] == 'C') { free(ntext); return; } //already greyed
		ntext = (char*)realloc(ntext, strlen(ntext)+4);
		for(int i=strlen(ntext); i>=0; i--) ntext[i+4]=ntext[i];
		ntext[0]='@'; ntext[1]='C'; ntext[2]='2'; ntext[3]='5';
		text(row,ntext);
fprintf (stderr, "row(%d): '%s'\n",row,ntext);
//		free(ntext);

		// grey icon - it will work automagically since we work with a pointer
		Fl_Image* im = get_icon(row)->copy();
		im->inactive();
		set_icon(row,im);

		//redraw(); // OPTIMIZE
	}

	void ungray(int row) {
		char *ntext = strdup(text(row));
		if (ntext[0] != '@' || ntext[1] != 'C') { free(ntext); return; } //not greyed
		snprintf(ntext, strlen(ntext), "%s", ntext+4);
		text(row,ntext);
		free(ntext);

		// grey icon - it will work automagically since we work with a pointer
		Fl_Image* im = get_icon(row);
		im->uncache(); // doesn't work

		//redraw(); // OPTIMIZE
	}
};


#endif

/* $Id */
