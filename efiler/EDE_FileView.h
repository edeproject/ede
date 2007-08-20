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



#define USE_FLU_WRAP_GROUP


#include <FL/filename.H> // for FL_PATH_MAX
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>

#include <edelib/String.h>
#include <edelib/IconTheme.h>
#include <edelib/Nls.h>

#include "EDE_Browser.h"

#ifdef USE_FLU_WRAP_GROUP
#  include "Flu_Wrap_Group.h"
#else
#  include <edelib/ExpandableGroup.h>
#endif



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


#define ICONW 70
#define ICONH 80
// Spacing to use between icons
#define ICON_SPACING 5

// How many pixels from selection edge will be ignored
// (e.g. if selection laso only touches a widget with <5px, it will not be selected)
#define SELECTION_EDGE 5

#ifdef USE_FLU_WRAP_GROUP
class FileIconView_ : public Flu_Wrap_Group {
#else
class FileIconView_ : public edelib::ExpandableGroup {
#endif
private:
	// private vars for handling selection and focus
	// because group doesn't do it
	int focused;
	int* m_selected;

	rename_callback_type* rename_callback_;
	paste_callback_type* paste_callback_;
	Fl_Callback* context_callback_;

	int select_x1,select_y1,select_x2,select_y2;

	Fl_Button* find_button(edelib::String realpath) {
		for (int i=0; i<children(); i++) {
			Fl_Button* b = (Fl_Button*)child(i);
			char *tmp = (char*)b->user_data();
			if (realpath==tmp) return b;
		}
		return 0;
	}
public:
#ifdef USE_FLU_WRAP_GROUP
	FileIconView_(int X, int Y, int W, int H, char*label=0) : Flu_Wrap_Group(X,Y,W,H,label)
#else
	FileIconView_(int X, int Y, int W, int H, char*label=0) : edelib::ExpandableGroup(X,Y,W,H,label)
#endif
	{
		end();
		box(FL_DOWN_BOX);
		color(FL_BACKGROUND2_COLOR);
#ifdef USE_FLU_WRAP_GROUP
		spacing(ICON_SPACING,ICON_SPACING);
#endif

		select_x1=select_y1=select_x2=select_y2=0;
		focused=0;
		m_selected = 0;

		rename_callback_ = 0;
		paste_callback_ = 0;
		context_callback_ = 0;
	}
	void insert(int row, FileItem *item) {
		// update list of selected[] items
		if (!m_selected)
			m_selected = (int*)malloc(sizeof(int)*(children()+1));
		else
			m_selected = (int*)realloc(m_selected,sizeof(int)*(children()+1));
		m_selected[children()]=0;

		Fl_Button* b = new Fl_Button(0,0,ICONW,ICONH);
		b->box(FL_FLAT_BOX);
		b->color(FL_BACKGROUND2_COLOR);
		b->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER|FL_ALIGN_WRAP|FL_ALIGN_CLIP);

		// Set the label
		char buffer[FL_PATH_MAX];
		uint j=0;
		for (uint i=0; i<item->name.length(); i++,j++) {
			buffer[j] = item->name[i];
			buffer[j+1] = '\0';
			if (buffer[j] == '@') { // escape @
				buffer[++j] = '@';
				buffer[j+1] = '\0';
			}
			b->copy_label(buffer);
			int lw =0, lh = 0;
			fl_measure(b->label(), lw, lh);
			if (lw>ICONW) {
				if (j==i+2) { // already added 2 newlines
					buffer[j-2]='.';
					buffer[j-1]='.';
					buffer[j]='.';
					buffer[j+1]='\0';
					j+=2;
					break; // for
				} else { // add newline
					buffer[j+1]=buffer[j];
					buffer[j]='\n';
					buffer[j+2]='\0';
					j++;
				}
			}
		}
		while (j<=item->name.length()+2){ 
			buffer[j++]='\n';
			//buffer[j++]='L';
		}
		buffer[j]='\0';
		b->copy_label(buffer);

		// Tooltip text
		edelib::String tooltip = _("Name: ")+item->name;
		if (item->size != "") tooltip += _("\nSize: ")+item->size;
		tooltip += _("\nType: ")+item->description+_("\nDate: ")+item->date+_("\nPermissions: ")+item->permissions;
		b->tooltip(strdup(tooltip.c_str()));
		b->user_data(strdup(item->realpath.c_str()));

		// Set icon
		edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_MEDIUM);
		if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_MEDIUM,edelib::ICON_CONTEXT_MIMETYPE);
		b->image(Fl_Shared_Image::get(icon.c_str()));

#ifdef USE_FLU_WRAP_GROUP
		Flu_Wrap_Group::insert(*b,row);
#else
		edelib::ExpandableGroup::insert(*b,row);
#endif
		//insert(*b,row); -- why doesn't this work?
		redraw();
	}
	void add(FileItem *item) { insert(children()+1, item); }

	void remove(FileItem *item) {
		Fl_Button* b = find_button(item->realpath);
		if (b) {
#ifdef USE_FLU_WRAP_GROUP
			Flu_Wrap_Group::remove(*b); // note that FWG requires to dereference the pointer
#else
			edelib::ExpandableGroup::remove(b);
#endif
			//remove(b);
			delete b;
		}
	}

	void remove(int row) {
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
#ifdef USE_FLU_WRAP_GROUP
		Flu_Wrap_Group::remove(*b); // note that FWG requires to dereference the pointer
#else
		edelib::ExpandableGroup::remove(b);
#endif
		//remove(b);
		delete b;
	}


	void update(FileItem *item) {
		Fl_Button* b = find_button(item->realpath);
		if (!b) return;

		// Tooltip text
		edelib::String tooltip = _("Name: ")+item->name+_("\nSize: ")+item->size+_("\nType: ")+item->description+_("\nDate: ")+item->date+_("\nPermissions: ")+item->permissions;
		b->tooltip(strdup(tooltip.c_str()));

		// Set icon
		edelib::String icon = edelib::IconTheme::get(item->icon.c_str(),edelib::ICON_SIZE_MEDIUM);
		if (icon=="") icon = edelib::IconTheme::get("misc",edelib::ICON_SIZE_MEDIUM,edelib::ICON_CONTEXT_MIMETYPE);
		b->image(Fl_Shared_Image::get(icon.c_str()));

		b->redraw();
	}

	// This is needed because update() uses path to find item in list
	void update_path(const char* oldpath,const char* newpath) {
		Fl_Button* b = find_button(oldpath);
		if (!b) return;
		b->user_data(strdup(newpath));
	}

	// Set item to "disabled"
	void gray(int row) { 
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
		// FIXME this also means that item can't be selected
		b->deactivate();
	}
	// Set item to "enabled"
	void ungray(int row) {
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
		b->activate();
	}

	// Setup callback that will be used when renaming and dnd
	void rename_callback(rename_callback_type* cb) { rename_callback_ = cb; }
	void paste_callback(paste_callback_type* cb) { paste_callback_ = cb; }
	void context_callback(Fl_Callback* cb) { context_callback_ = cb; }

	// Item real path (data value)
	const char* path(int row) {
		if (row<1 || row>children()) return 0;
		Fl_Button* b = (Fl_Button*)child(row-1);
		return (const char*)b->user_data();
	}

	// Is item selected?
	int selected(int row) { 
		if (row<1 || row>children()) return 0;
		int i=0;
		while(m_selected[i]!=0)
			if (m_selected[i++]==row) return 1;
		return 0;
	}

	// Select item (if value=1) or unselect (if value=0)
	void select(int row, int value) {
		if (row<1 || row>children()) return;
		if (!m_selected) return; // shouldn't happen
		set_focus(row);
		int i=0;
		Fl_Button* b = (Fl_Button*)child(row-1);
		if (value) {
			while (m_selected[i++]!=0);
			m_selected[i]=row;

			b->color(FL_SELECTION_COLOR);
			b->labelcolor(fl_contrast(FL_FOREGROUND_COLOR,FL_SELECTION_COLOR));
			b->redraw();
		} else {
			while (m_selected[i]!=0) {
				if (m_selected[i]==row) {
					int j=i;
					while (m_selected[j]!=0) {
						m_selected[j]=m_selected[j+1];
						j++;
					}
					break;
				}
				i++;
			}
			
			b->color(FL_BACKGROUND2_COLOR);
			b->labelcolor(FL_FOREGROUND_COLOR);
			b->redraw();
		}
	}

	// Return nr. of widget that has keyboard focus
	int get_focus() {
/*#ifdef USE_FLU_WRAP_GROUP
fprintf( stderr, "---- get_focus()=%d\n", focused);
		return focused;
#else*/
		Fl_Widget* focus = Fl::focus();
		int i = find(focus); // Fl_Group method
		if (i<children()) return i+1;
		else return 0;
//#endif
	}

	// Scroll view until item becomes visible
	void show_item(int row) {
fprintf(stderr, "show_item(%d)\n", row);
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);

#ifdef USE_FLU_WRAP_GROUP
		// scroll_to() will scroll widget to the top of view
		// I prefer that view is scrolled just enough so icon is visible
		// FIXME: sometimes b->y() is so large that it wraps around and becomes positive
		if (b->y()+b->h() > y()+h()) {
			int scrollto = scrollbar.value() + (b->y()+b->h()-y()-h())+1;
fprintf (stderr, "by: %d bh: %d y: %d h: %d sv: %d\n", b->y(), b->h(), y(), h(), scrollbar.value());
			((Fl_Valuator*)&scrollbar)->value(scrollto);
			redraw();
			draw(); // we need to relayout the group so that b->y() value is updated for next call
		}
		if (b->y() < y())
			scroll_to(b);
#else
		// Not tested and probably broken:
		Fl_Scrollbar* s = get_scroll();
		if (b->y() > s->value()+h()) {
			// Widget is below current view
			scrolly(b->y()+b->h()-h());
		} else if (b->y() < s->value()) {
			// Widget is above current view
			scrolly(b->y());
		}
		// else { widget is visible, do nothing }
#endif
	}

	// Set keyboard focus to given item
	void set_focus(int row) {
fprintf( stderr, "---- set_focus(%d)\n", row);
		if (row<1 || row>children()) return;
		Fl_Button* b = (Fl_Button*)child(row-1);
		b->take_focus();
		show_item(row);
		focused=row;
	}


	// Overloaded handle() method, see inline comments for details
	int handle(int e) {
//fprintf (stderr, " -- FileIconView - Event %d\n",e);

		// Fixes for focus management
		// fltk provides focus management for members of Fl_Group, but it has minor problems:
		//  - when pressing Alt+Tab, focused widget is forgotten
		//  - if no widget is focused, arrow keys will navigate outside group
		//  - Tab has same effect as right arrow

		
		if (Fl::focus()->inside(this)) { // is focus inside?
			int k = Fl::event_key();
//fprintf(stderr, "event: %d key: %d\n",e,k);
			if (k==FL_Up || k==FL_Down || k==FL_Left || k==FL_Right) {
				// Wrap around
				// FL_KEYDOWN happens only if key is otherwise unhandled
				if (e==FL_KEYDOWN) {
					int x = get_focus()-1;
					Fl_Widget* b = child(x);
					Fl_Widget* b2;
					if (k==FL_Up) b2=above(b);
					if (k==FL_Down) b2=below(b);
						// Bug in Flu_Wrap_Group - inconsistent behavior of below()
						if (k==FL_Down && b2==b) b2=child(children()-1);
					if (k==FL_Left) b2=left(b);
					if (k==FL_Right) b2=next(b);
					if (((k==FL_Up) || (k==FL_Left)) && x==0) 
						b2=child(children()-1);
					if (((k==FL_Down) || (k==FL_Right)) && x==children()-1) 
						b2=child(0);
					for (int i=0; i<children(); i++)
						if (b2==child(i)) set_focus(i+1);
					return 1;
				}

				// Remember focus for restoring later
				if (e==FL_KEYUP || e==FL_KEYBOARD) {// Sometimes only one of these is triggered
					focused = get_focus();
					if (!focused) {
						// Nothing is focused
						if (k==FL_Up || k==FL_Left)
							set_focus(children());
						else
							set_focus(1);
					} else
						show_item(focused); // scroll follows focus
					return 1; // Don't let fltk navigate to another widget
				}
			}

			// Tab key should always navigate outside group
			if (e!=FL_NO_EVENT && e!=FL_KEYUP && k==FL_Tab) { // NO_EVENT happens before widget gets focus
fprintf(stderr, "TAB %d - focused: (%s)\n",e,Fl::focus()->label());
				// Because widgets can be *very* nested, we go straight
				// to window()
				Fl_Group* master=window();
				Fl_Widget* jumpto=0;
				int i=0;
				while (jumpto==0 && master!=this) {
					jumpto=master->child(i);
fprintf (stderr, " -- (%s)\n", jumpto->label());
					if (this->inside(jumpto) || !jumpto->visible())
						jumpto=0;
					i++;
					if (i==master->children()) {
						int j;
						for (j=0; j<master->children(); j++)
							if (master->child(j)->visible()) break;
						if (j==master->children()) {// nothing is visible!?
							master=(Fl_Group*)this;
fprintf (stderr, "WTF\n");
						}else{
							master=(Fl_Group*)master->child(j);
fprintf (stderr, " -> [%s]\n", master->label());
}
						i=0;
					}
					if (jumpto!=0 && !jumpto->take_focus())
						jumpto=0; // widget refused focus, keep searching
				}
fprintf (stderr, "[X]\n");
				// if this is the only widget, do nothing
				return 1;
			}
		}
		if (e==FL_FOCUS) {
			// Restore focused widget after losing focus
			if (focused) set_focus(focused);
			return 1; 
		}

/*#ifdef USE_FLU_WRAP_GROUP
		int k = Fl::event_key();
//		if (e==FL_KEYBOARD) fprintf (stderr, "Key: %d", k);
		if ((e==FL_KEYUP || e==FL_KEYBOARD) && (k==FL_Up || k==FL_Down || k==FL_Left || k==FL_Right)) {
			Fl_Widget* nextw=0;
			if (focused) {
				Fl_Widget* currentw = child(focused-1);
				switch(k) { // Flu_Wrap_Group methods:
					case FL_Up:
						nextw = above(currentw); break;
					case FL_Down:
						nextw = below(currentw); break;
					case FL_Left:
						nextw = left(currentw); break;
					case FL_Right:
						nextw = next(currentw); break;
				}
				// Flu_Wrap_Group bug: all methods will wrap around EXCEPT
				// Flu_Wrap_Group::below() ?!
				if (k==FL_Down && nextw==currentw)
					nextw = child(0);
			}
fprintf (stderr, "Event k: %d focused: %d\n", k, focused);

			if (focused && nextw)
				for (int i=0; i<children(); i++) { 
					if (child(i) == nextw) {
fprintf (stderr, "Call %d\n", i);
						set_focus(i+1);
					}
				}
			else // No widget is selected, or it is first/last
				switch(k) {
					 // TODO: wrap around
					case FL_Up:
					case FL_Left:
						set_focus(children()); break;
					case FL_Down:
					case FL_Right:
						set_focus(1); break;
				}
			return 1;
		}
		if (e==FL_FOCUS) {
			// Restore focused widget after losing focus
			if (focused) set_focus(focused);
			return 1; 
		}
#endif*/

		// We accept mouse clicks
		if (e==FL_PUSH && Fl::event_x()>x() && Fl::event_x() < x()+w()-Fl::scrollbar_size()) {
			return 1;
		}
		// We accept focus (defaults to 0 with FL_GROUP)
		if (e==FL_FOCUS) return 1;

		// Do callback on enter key
		// (because icons don't have callbacks)
		if (e==FL_KEYBOARD && Fl::event_key()==FL_Enter) {
			do_callback();
			return 1;
		}

		// Drag to select (a.k.a. "laso") operation
		// Draw a dashed line around icons
		static bool laso=false;
		if (e==FL_DRAG) {
fprintf (stderr, "FL_DRAG! ");
			if (!laso) {
fprintf (stderr, "- begin.\n");
				laso=true;
				// Set coordinates for selection box (drawn in draw())
				select_x1=select_x2=Fl::event_x();
				select_y1=select_y2=Fl::event_y();
			} else {
fprintf (stderr, "- box (%d,%d,%d,%d).\n",select_x1,select_y1,select_x2,select_y2);
				select_x2=Fl::event_x();
				select_y2=Fl::event_y();
				redraw();
			}
			return 1;
		}

		// Mouse button released
		if (e==FL_RELEASE) {
fprintf (stderr, "FL_RELEASE! ");
			// Unselect everything unless Shift or Ctrl is held
			if (!Fl::event_state(FL_SHIFT) && !Fl::event_state(FL_CTRL)) {
				for (int i=0;i<children();i++) {
					// We cannot use select(i+1,0) because that would mess with focus
					Fl_Button* b = (Fl_Button*)child(i);
					b->color(FL_BACKGROUND2_COLOR);
					b->labelcolor(FL_FOREGROUND_COLOR);
				}
				redraw();
				int i=0;
				while (m_selected[i]!=0) m_selected[i++]=0;
			}

			// Stop laso operation
			if (laso) {
fprintf (stderr, "- stop drag.\n");
				laso=false;

				// Order coordinates
				int tmp;
				if (select_x1>select_x2) { tmp=select_x1; select_x1=select_x2; select_x2=tmp; }
				if (select_y1>select_y2) { tmp=select_y1; select_y1=select_y2; select_y2=tmp; }

				// Don't include the edges
				select_x1 += SELECTION_EDGE;
				select_y1 += SELECTION_EDGE;
				select_x2 -= SELECTION_EDGE;
				select_y2 -= SELECTION_EDGE;
fprintf(stderr, "After fixing the box coords: (%d,%d,%d,%d)\n", select_x1, select_y1, select_x2, select_y2);

				// Calculate which buttons were lasoed
				int i;
				for (i=0; i<children(); i++) {
					Fl_Widget* w = child(i); // shortcuts
					int wx2 = w->x()+w->w();
					int wy2 = w->y()+w->h();
					if (select_x2>w->x() && select_x1<wx2 && select_y2>w->y() && select_y1<wy2) {
						select(i+1,1);
fprintf (stderr, "Select widget: '%20s' (%d,%d,%d,%d)\n", w->label(), w->x(), w->y(), wx2, wy2);
					}
				}
				// TODO: add code for Shift key

				select_x1=select_x2=select_y1=select_y2=0;
				redraw();

			// Single click
			} else {
				// Find child that was clicked
				int i;	
				for (i=0; i<children(); i++)
					if (Fl::event_inside(child(i))) {
fprintf (stderr, "in child %d\n",i);
						select(i+1,1);
					}
				// TODO: add code for Shift key

				// Didn't click on icon
				if (i==children()) {
					this->take_focus(); // Remove focus from all buttons
					focused=0;
				}
			}

			// Right button - call context menu
			if (Fl::event_button() == 3) {
				Fl::event_is_click(0); // prevent doubleclicking with right button
				if (context_callback_) context_callback_(this, (void*)path(get_focus()));
			}

			// Double-click operation
			if (Fl::event_clicks()) {
				do_callback();
			}
		}
/*
		// Check if it is drag
		static bool clicked=false;
		if (e==FL_PUSH) {
fprintf (stderr, "FL_PUSH %d %d...\n",Fl::event_is_click(),Fl::event_clicks());
			clicked=true;
			return 1;
		} else if (e != FL_DRAG && e != FL_NO_EVENT && clicked) {
fprintf (stderr, "-- triggered click %d\n",e);
			clicked=false;
			// Unselect everything unless Shift or Alt is held
			if (!Fl::event_key(FL_Shift_L) && !Fl::event_key(FL_Shift_R) && !Fl::event_key(FL_Shift_L)) 
				for (int i=0; i<children(); i++)
					select(i+1,0);

			for (int i=0; i<children(); i++)
				if (Fl::event_inside(child(i))) {
fprintf (stderr, "in child %d\n",i);
					select(i+1,1);
					return 0;
				}
fprintf (stderr, "in no child\n");
			// Didn't click on icon
			take_focus(); // Remove focus from all buttons
		}*/

#ifdef USE_FLU_WRAP_GROUP
		return Flu_Wrap_Group::handle(e);
#else
		return edelib::ExpandableGroup::handle(e);
#endif

	}

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
#else
		edelib::ExpandableGroup::clear();
#endif
	}
};


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
		Fl_Image* im = get_icon(row)->copy();
		//im->refresh();
		//im->uncache(); // doesn't work
		//im->color_average(FL_BACKGROUND_COLOR, 1.0);
		//im->data(im->data(),im->count());
		set_icon(row,im);

		redraw(); // OPTIMIZE
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

	int visible() { return Fl_Widget::visible(); }
};




/*class FileView : public Fl_Group {
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
};*/

enum FileViewType {
	FILE_DETAILS_VIEW,
	FILE_ICON_VIEW
};


class FileDetailsView : public Fl_Group {
private:
	FileDetailsView_* browser;
	FileIconView_* icons;
	FileViewType m_type;
public:
	FileDetailsView(int X, int Y, int W, int H, char*label=0) : Fl_Group(X,Y,W,H,label) {
		browser = new FileDetailsView_(X,Y,W,H,label);
		browser->end();
//		browser->hide();
		icons = new FileIconView_(X,Y,W,H,label);
		icons->end();
		end();

		// Set default to FILE_DETAILS_VIEW
		icons->hide(); 
		m_type=FILE_DETAILS_VIEW;
	}

	void setType(FileViewType t) {
		m_type=t;
		if (t==FILE_DETAILS_VIEW) {
			icons->hide(); 
			//browser->show();
		}
		if (t==FILE_ICON_VIEW) {
			//browser->hide(); 
			icons->show(); 
			//redraw();
		}
	}
	FileViewType getType() {
		return m_type;
	}

	// View methods
	void insert(int row, FileItem *item) { browser->insert(row,item); icons->insert(row,item); }
	void add(FileItem *item) { browser->add(item); icons->add(item); }
	void remove(FileItem *item) { browser->remove(item); icons->remove(item); }
	void update(FileItem *item) { browser->update(item); icons->update(item); }

	void update_path(const char* oldpath,const char* newpath) { browser->update_path(oldpath,newpath); icons->update_path(oldpath,newpath); }

	void gray(int row) { browser->gray(row); icons->gray(row); }
	void ungray(int row) { browser->ungray(row); icons->ungray(row); }

	void rename_callback(rename_callback_type* cb) { browser->rename_callback(cb); icons->rename_callback(cb); }
	void paste_callback(paste_callback_type* cb) { browser->paste_callback(cb); icons->paste_callback(cb); }
	void context_callback(Fl_Callback* cb) { browser->context_callback(cb); icons->context_callback(cb); }

	// Browser methods
	const char* path(int i) { if (m_type==FILE_DETAILS_VIEW) return (const char*)browser->data(i); else return icons->path(i); }
	int size() { if (m_type==FILE_DETAILS_VIEW) return browser->size(); else return icons->children();}
	int selected(int i) { if (m_type==FILE_DETAILS_VIEW) return browser->selected(i); else return icons->selected(i); }
	void select(int i, int k) { browser->select(i,k); browser->middleline(i); icons->select(i,k); icons->show_item(i); }
	int get_focus() { if (m_type==FILE_DETAILS_VIEW) return browser->get_focus(); else return icons->get_focus(); }
	void set_focus(int i) { browser->set_focus(i); icons->set_focus(i);}
	void remove(int i) { browser->remove(i); icons->remove(i);}
	void clear() { browser->clear(); icons->clear();}
	void callback(Fl_Callback*cb) { browser->callback(cb); icons->callback(cb);}
	int take_focus() { if (m_type==FILE_DETAILS_VIEW) return browser->take_focus(); else return icons->take_focus(); }

	// These methods are used by do_rename()
	const char* text(int i) { return browser->text(i); }
	void text(int i, const char* c) { return browser->text(i,c); }
	uchar column_char() { return browser->column_char(); }

	// Overloaded...
	//int handle(int e) { if
};



#endif

/* $Id */
