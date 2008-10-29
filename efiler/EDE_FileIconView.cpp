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


#include <FL/Fl_Button.H>
#include <FL/filename.H> // for FL_PATH_MAX
#include <FL/fl_draw.H> // for fl_measure and drawing selection box
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Window.H> // for window() used in ctor

#include <edelib/Nls.h>
#include <edelib/String.h>
#include <edelib/IconTheme.h>
#include <edelib/Debug.h>


// debugging help
#define DBG "FileIconView: "


// Dimensions
#define ICONW 70
#define ICONH 80

// Spacing to use between icons
#define ICON_SPACING 5

// Used to make edges of icons less sensitive to selection box, dnd and such
// (e.g. if selection laso only touches a widget with <5px, it will not be selected)
#define SELECTION_EDGE 5

// Sometimes I accidentaly "drag" an icon - don't want dnd to trigger
#define MIN_DISTANCE_FOR_DND 10

// size of editbox
#define EDITBOX_WIDTH (ICONW+50)


// --------------------------------------
//
// 	File renaming stuff
//
// --------------------------------------


// EditBox is displayed when renaming a file
// We subclass Fl_Input so we can handle keyboard
int FileIconView::EditBox::handle (int e) {
EDEBUG(DBG " Editbox handle(%d)\n", e);
	FileIconView* view = (FileIconView*)parent();
	// in FIV we handle FL_KEYDOWN and FL_KEYUP - they shouldn't be sent!
	if ((e==FL_KEYBOARD || e==FL_KEYDOWN || e==FL_KEYUP) && visible()) {
		int k = Fl::event_key();
		if (Fl::event_key()==FL_Enter && visible()) {
			view->hide_editbox();
			view->finish_rename();
		} else if (k==FL_Up || k==FL_Down || k==FL_Page_Up || k==FL_Page_Down) {
			view->hide_editbox();
			return 0; // let the view scroll
		} else if (e==FL_KEYBOARD && (k==FL_F+2 || k==FL_Escape))
			// FL_KEYUP will happen immediately after showing editbox
			view->hide_editbox();
		else
			Fl_Input::handle(e);
		
		return 1; // don't send keys to view
	}

	if (e==FL_MOUSEWHEEL && visible()) view->hide_editbox();
	return Fl_Input::handle(e);
}


// show editbox at specified row and make the row "invisible" (bgcolor same as fgcolor)
void FileIconView::show_editbox(int row) {
	if (!rename_callback_) return;
	if (row<1 || row>children()) return; // nothing selected

	Fl_Widget* w = child(row-1);
	const char* label = w->label();
	if (label[0]=='.' && label[1]=='.' && label[2]=='\0') return; // can't rename "go up" button

	// unselect the icon with focus - it's prettier that way
	select(row,0);
	editbox_row=row;

	// Copy filename to editbox
	char* path_ = strdup(path(row));
	if (path_[strlen(path_)-1] == '/') path_[strlen(path_)-1]='\0';
	const char* filename = fl_filename_name(path_);
	editbox_->value(filename);
	free(path_);

	// make the label "invisible"
	w->labelcolor(FL_BACKGROUND2_COLOR);

	// calculate location for editbox
	// "Magic constants" here are just nice spacing
	int X=w->x() + (w->w()/2) - (EDITBOX_WIDTH/2);
	if (X<x()) X=x();
	int Y=w->y() + edelib::ICON_SIZE_MEDIUM + 5;
	int W=EDITBOX_WIDTH;
	int H=FL_NORMAL_SIZE + 4;
EDEBUG(DBG"editbox (%d,%d,%d,%d) \n",X,Y,W,H);

	editbox_->resize(X,Y,W,H);
	editbox_->show();
	editbox_->take_focus();
	redraw();
	w->redraw();
	editbox_->redraw();
}


void FileIconView::hide_editbox() {
EDEBUG(DBG "hide_editbox()\n");
	editbox_->hide();

	// Make the edited row visible again
	Fl_Widget* w = child(editbox_row-1);
	w->labelcolor(FL_FOREGROUND_COLOR);
	w->redraw(); // label will be updated by rename_callback, if necessary
}


// This is called to actually rename file (when user presses Enter in editbox)
void FileIconView::finish_rename() {
EDEBUG(DBG "finish_rename()\n");
	rename_callback_(editbox_->value());
}



// ctor

#ifdef USE_FLU_WRAP_GROUP
FileIconView::FileIconView(int X, int Y, int W, int H, char*label) : Flu_Wrap_Group(X,Y,W,H,label)
#else
FileIconView::FileIconView(int X, int Y, int W, int H, char*label) : edelib::ExpandableGroup(X,Y,W,H,label)
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

	editbox_ = new EditBox(0, 0, 0, 0);
	window()->add(editbox_); // this is required to make editbox visible
	editbox_->box(FL_BORDER_BOX);
	editbox_->parent(this);
	editbox_->hide();
}




// Try various names and sizes for icons
Fl_Image* FileIconView::try_icon(edelib::String icon_name) {
	edelib::String icon_path;
	edelib::String tmpname;

fprintf (stderr, "-- Trying %s\n", icon_name.c_str());

	icon_path = edelib::IconTheme::get(icon_name.c_str(), edelib::ICON_SIZE_MEDIUM);
	if (icon_path!="")
		return Fl_Shared_Image::get(icon_path.c_str());

	// Because edelib doesn't resample icons, we need to do that manually
	icon_path = edelib::IconTheme::get(icon_name.c_str(),edelib::ICON_SIZE_SMALL);
	if (icon_path=="") icon_path = edelib::IconTheme::get(icon_name.c_str(),edelib::ICON_SIZE_TINY);
	if (icon_path=="") icon_path = edelib::IconTheme::get(icon_name.c_str(),edelib::ICON_SIZE_LARGE);
	if (icon_path=="") icon_path = edelib::IconTheme::get(icon_name.c_str(),edelib::ICON_SIZE_HUGE);
	if (icon_path=="") icon_path = edelib::IconTheme::get(icon_name.c_str(),edelib::ICON_SIZE_ENORMOUS);

	if (icon_path!="") {
		Fl_Image *temp = Fl_Shared_Image::get(icon_path.c_str());
		if (temp) {
			return temp->copy(edelib::ICON_SIZE_MEDIUM, edelib::ICON_SIZE_MEDIUM);
		}
	}

	// Try Gnome icons
	if (icon_name.substr(0,11) != "gnome-mime-") {
		tmpname = "gnome-mime-";
		tmpname += icon_name;
		Fl_Image *tmpimage = try_icon(tmpname);
		if (tmpimage) return tmpimage;

		// Try generic icons without last part
		if (icon_name.find('-',0) != edelib::String::npos) {
			tmpname = icon_name.substr(0,icon_name.find('-',0));
			tmpimage = try_icon(tmpname);
			if (tmpimage) return tmpimage;

			// Replace text with txt
			if (tmpname == "text") {
				tmpimage = try_icon("txt");
				if (tmpimage) return tmpimage;
			}
		}

		// No icon found for this mimetype, we're trying a generic "file" icon
		// (in crystalsvg "misc" looks better...)
		if (icon_name != "empty") {
			return try_icon("empty");
		}
	}
	

	// Not even generic icon is found :( 
	return 0;
}

// Methods for manipulating items
// They are named similarly to Browser methods, except that they take 
// struct FileItem (defined in EDE_FileView.h)

void FileIconView::insert(int row, FileItem *item) {
	// update list of selected[] items
	if (!m_selected)
		m_selected = (int*)malloc(sizeof(int)*(children()+1));
	else
		m_selected = (int*)realloc(m_selected,sizeof(int)*(children()+1));
	m_selected[children()]=0;

	Fl_Button* b = new Fl_Button(0,0,ICONW,ICONH);
//	b->box(FL_RFLAT_BOX); // Rounded box - has bug with selection box
	b->box(FL_FLAT_BOX);
	b->color(FL_BACKGROUND2_COLOR);
	b->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER|FL_ALIGN_CLIP);

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

	// Don't crash if try_icon returns nothing
	Fl_Image *tmp = try_icon(item->icon);
	if (tmp) b->image(tmp);

#ifdef USE_FLU_WRAP_GROUP
	Flu_Wrap_Group::insert(*b,row);
#else
	edelib::ExpandableGroup::insert(*b,row);
	edelib::ExpandableGroup::handle(FL_SHOW); // This will force calling reposition_childs()
#endif
	//insert(*b,row); -- why doesn't this work?
	redraw();
}

void FileIconView::remove(FileItem *item) {
	Fl_Widget* w = find_icon(item->realpath);
	if (w) {
#ifdef USE_FLU_WRAP_GROUP
		Flu_Wrap_Group::remove(*w); // note that FWG requires to dereference the pointer
#else
		edelib::ExpandableGroup::remove(w);
#endif
		//remove(w);
		delete w;
	}
}

void FileIconView::remove(int row) {
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


void FileIconView::update(FileItem *item) {
	Fl_Widget* w = find_icon(item->realpath);
	if (!w) return;

	// Tooltip text
	edelib::String tooltip = _("Name: ")+item->name+_("\nSize: ")+item->size+_("\nType: ")+item->description+_("\nDate: ")+item->date+_("\nPermissions: ")+item->permissions;
	w->tooltip(strdup(tooltip.c_str()));

	// Don't crash if try_icon returns nothing
	Fl_Image *tmp = try_icon(item->icon);
	if (tmp) w->image(tmp);

	w->redraw();
}

// This method is needed because update() uses path to find item in list
void FileIconView::update_path(const char* oldpath,const char* newpath) {
	Fl_Widget* w = find_icon(oldpath);
	if (!w) return;
	w->user_data(strdup(newpath));

	// Set the label
	char buffer[FL_PATH_MAX];
	const char* name = fl_filename_name(newpath);
	uint j=0;
	for (uint i=0; i<strlen(name); i++,j++) {
		buffer[j] = name[i];
		buffer[j+1] = '\0';
		if (buffer[j] == '@') { // escape @
			buffer[++j] = '@';
			buffer[j+1] = '\0';
		}
		w->copy_label(buffer);
		int lw =0, lh = 0;
		fl_measure(w->label(), lw, lh);
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
	while (j<=strlen(name)+2){ 
		buffer[j++]='\n';
		//buffer[j++]='L';
	}
	buffer[j]='\0';
	w->copy_label(buffer);
}

// Select item (if value==1) or unselect (if value==0)
// We are faking selection by manipulating fg & bg color
// TODO: create a new class for icons that should handle selection internally
void FileIconView::select(int row, int value) {
	if (row<1 || row>children()) return;
	if (!m_selected) return; // shouldn't happen
	set_focus(row);
	int i=0;
	Fl_Button* b = (Fl_Button*)child(row-1);
	if (value) {
		while (m_selected[i++]!=0);
		m_selected[i-1]=row;

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

// Scroll the view until item becomes visible
void FileIconView::show_item(int row) {
EDEBUG(DBG"show_item(%d)\n", row);
	if (row<1 || row>children()) return;
	Fl_Button* b = (Fl_Button*)child(row-1);

#ifdef USE_FLU_WRAP_GROUP
	Fl_Scrollbar* s = &scrollbar;
#else
	Fl_Scrollbar* s = get_scroll();
#endif

	// Flu_Wrap_Group::scroll_to() will scroll until icon is at the top of view
	// I prefer that view is scrolled just enough so icon is visible
	// FIXME: sometimes b->y() is so large that it wraps around and becomes positive
	int scrollto=0;
	if (b->y()+b->h() > y()+h())
		scrollto = s->value() + (b->y()+b->h()-y()-h())+1;
	else if (b->y() < y())
		scrollto = s->value() - (y()-b->y());

	if (scrollto) {
		EDEBUG(DBG"by: %d bh: %d y: %d h: %d sv: %d\n", b->y(), b->h(), y(), h(), s->value());
#ifdef USE_FLU_WRAP_GROUP
		((Fl_Valuator*)s)->value(scrollto);
		redraw();
		draw(); // we need to relayout the group so that b->y() value is updated for next call
#else
		scrolly(scrollto); // ExpandableGroup method
#endif
	}
}


// Overloaded handle() method
// it's currently a bit messy, see implementation comments for details
int FileIconView::handle(int e) {
//EDEBUG(DBG"Event %d\n",e);

	// ------------------------------------------------
	//    Fixes for focus management
	// ------------------------------------------------

	// fltk provides focus management for members of Fl_Group, but it has minor problems:
	//  - after switching to another window and back, focused widget is forgotten
	//  - if no widget is focused, arrow keys will navigate outside group
	//  - Tab has same effect as right arrow - we want arrows to only be used inside
	// group, and (Shift+)Tab to be used for going outside

	if (Fl::focus()->inside(this)) { // is focus inside?
		int k = Fl::event_key();
//EDEBUG(DBG"event_key: %d\n",k);
		if (k==FL_Up || k==FL_Down || k==FL_Left || k==FL_Right) {
#ifdef USE_FLU_WRAP_GROUP
			// Wrap around - FWG only (due to methods such as above())
			// FL_KEYDOWN happens only if key is otherwise unhandled
			if (e==FL_KEYDOWN) {
				int x = get_focus()-1;
				Fl_Widget* b = child(x);
				Fl_Widget* b2;

				switch(k) {
				case FL_Up:
					b2=above(b);
					if (x==0) b2=child(children()-1);
					break;
				case FL_Down:
					b2=below(b);
					// Bug in Flu_Wrap_Group - inconsistent behavior of below():
					if (b2==b) b2=child(children()-1);
					if (x==children()-1) b2=child(0);
					break;
				case FL_Left:
					b2=left(b);
					if (x==0) b2=child(children()-1);
					break;
				case FL_Right:
					b2=next(b);
					if (x==children()-1) b2=child(0);
					break;
				default: // to silence compiler warnings
					break;
				}

				for (int i=0; i<children(); i++)
					if (b2==child(i)) set_focus(i+1);
				return 1;
			}
#else
			// edelib::ExpandableGroup - Full keyboard navigation
			if (e==FL_KEYDOWN) {
				int x = get_focus();

				// Wrap around
				if ((k==FL_Up || k==FL_Left) && x==1)
					set_focus(children());
				else if ((k==FL_Down || k==FL_Right) && x==children())
					set_focus(1);

				// Left/right use item ordering
				else if (k==FL_Left)
					set_focus(x-1);
				else if (k==FL_Right)
					set_focus(x+1);

				// Use widget x() && y() to find item above/below
				// This isn't very elegant, but with ExpandableGroup there is no other way...
				else {
					Fl_Widget* w = child(x-1);
					Fl_Widget* w2 = 0;
					int dest=0;
					for (int i=0; i<children(); i++) {
						Fl_Widget* t=child(i);
						if (k==FL_Up && t->x()==w->x() && t->y()<w->y())
							if (!w2 || w2->y()<t->y()) { 
								w2=t;
								dest=i+1;
							}
						if (k==FL_Down && t->x()==w->x() && t->y()>w->y())
							if (!w2 || w2->y()>t->y()) {
								w2=t;
								dest=i+1;
							}
					}

					if (w2) 
						set_focus(dest);
					else { // nothing found
						if (k==FL_Up)
							set_focus(1);
						else
							set_focus(children());
					}
				}
				return 1;
			}
#endif

			// Remember focus for restoring later
			if (e==FL_KEYUP || e==FL_KEYBOARD) { // Sometimes only one of these is triggered
				focused = get_focus();
				if (!focused) {
					// Nothing is focused - this is initial state
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
		// NOTE: This code is ugly! I am ashamed...
		if (e!=FL_NO_EVENT && e!=FL_KEYUP && k==FL_Tab) { // FL_NO_EVENT happens a lot
EDEBUG(DBG"TAB %d - focused: (%s)\n",e,Fl::focus()->label());
			// Try to find another widget to give focus to.
			// Because widgets can be *very* nested, we go straight
			// to window()

			Fl_Group* parent_=(Fl_Group*)window();
			Fl_Widget* jumpto=0;
			int i=0;
			while (jumpto==0 && parent_!=this) {
				jumpto=parent_->child(i);
EDEBUG(DBG" -- (%s)\n", jumpto->label());
				if (this->inside(jumpto) || !jumpto->visible())
					jumpto=0;
				i++;
				if (i==parent_->children()) {
					int j;
					for (j=0; j<parent_->children(); j++)
						if (parent_->child(j)->visible()) break;
					if (j==parent_->children()) {
						// nothing is visible!?
EDEBUG(DBG"WTF!\n");
						parent_=(Fl_Group*)this;
					} else {
						parent_=(Fl_Group*)parent_->child(j);
EDEBUG(DBG" -> [%s]\n", parent_->label()); // label everything to debug!!!
					}
					i=0;
				}
				if (jumpto!=0 && !jumpto->take_focus())
					jumpto=0; // widget refused focus, keep searching
			}
EDEBUG(DBG"[X]\n");
			// if this is the only widget, do nothing
			return 1;
		}

	} // if (Fl::focus()->inside(this)) ...

	// Restore focused widget after losing focus
	if (e==FL_FOCUS) {
		if (focused) set_focus(focused);
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

		if (e==FL_PUSH) { renaming=false;
EDEBUG(DBG"FL_PUSH\n");
}

		// Click once on item that is already selected AND focused to rename it
		if (e==FL_PUSH && !editbox_->visible() && Fl::event_clicks()==0 && Fl::event_button()==1) {
EDEBUG(DBG"start rename\n");
			// allow to click for focusing
			if (!Fl::focus()->inside(this)) return 1;

			// Is click inside a label?
			int clicked=0;
			for (int i=0; i<children(); i++) {
				Fl_Widget* w = child(i);
				int X=Fl::event_x(); int Y=Fl::event_y();
				if (X>w->x() && X<w->x()+w->w() && Y>w->y()+edelib::ICON_SIZE_MEDIUM && Y<w->y()+w->h())
					{ clicked=i+1; break; }
			}

			// Is clicked item selected and focused?
			if (clicked && get_focus()==clicked && selected(clicked)) {
				renaming=true; // On next event, we will check if it's doubleclick
			} // otherwise, let the event become select/focus event
EDEBUG(DBG"clicked: %d \n",clicked);
if (renaming) EDEBUG(DBG"renaming!\n");
		}
		
		if (renaming && (e==FL_RELEASE || e==FL_MOVE) && Fl::event_is_click()!=0  && Fl::event_clicks()==0) {
EDEBUG(DBG"stop rename\n");
			// Fl::event_is_click() will be ==0 on drag
			// Fl::event_clicks() will be >0 on doubleclick
			show_editbox(get_focus());
			renaming=false;
			// don't pass mouse event, otherwise item will become reselected which is a bit ugly
			return 1;
		}
	}


	// ------------------------------------------------
	//    Make Fl_Group behave like proper widget
	// ------------------------------------------------

	// We accept focus (this defaults to 0 with FL_GROUP)
	if (e==FL_FOCUS) return 1;

	// We accept mouse clicks as long as they're inside
	if (e==FL_PUSH && Fl::event_x()>x() && Fl::event_x() < x()+w()-Fl::scrollbar_size()) return 1;

	// Do callback on enter key
	// (because icons don't have callbacks)
	if (e==FL_KEYBOARD && Fl::event_key()==FL_Enter) {
		do_callback();
		return 1;
	}

	// Select/unselect toggle using SPACE
	if ((e==FL_KEYBOARD || e==FL_KEYUP) && Fl::event_key()==' ') {
		if (selected(get_focus()))
			select(get_focus(),0);
		else
			select(get_focus(),1);
		return 1;
	}


	// ------------------------------------------------
	//    FL_DRAG does two things: laso and dnd
	// ------------------------------------------------

	// "laso" a.k.a. selection box is common name for box used to select many widgets at once

	static bool laso=false; // are we laso-ing?
	static bool scrolling=false;
	static int dragx,dragy; // to check for min dnd distance
	if (e==FL_DRAG) {
EDEBUG(DBG"FL_DRAG! ");

		// No laso is active, either start laso or start dnd operation
		if (!laso) {
			// Drag inside child is dnd
			int ex=Fl::event_x(); int ey=Fl::event_y();
#ifdef USE_FLU_WRAP_GROUP
			if (scrollbar.visible() && (ex>x()+w()-scrollbar.w() || scrolling)) {
EDEBUG(DBG"scrolling\n");
				this->take_focus();
				scrolling=true;
				return scrollbar.handle(e);
			}
#else
			if (get_scroll()->visible() && (ex>x()+w()-get_scroll()->w() || scrolling)) {
EDEBUG(DBG"scrolling\n");
				this->take_focus();
				scrolling=true;
				return get_scroll()->handle(e);
			}
#endif
			int inside=0;
			for (int i=0; i<children(); i++) {
				Fl_Button* b = (Fl_Button*)child(i);
				if (ex>b->x()+SELECTION_EDGE && ex<b->x()+b->w()-SELECTION_EDGE && ey>b->y()+SELECTION_EDGE && ey<b->y()+b->h()-SELECTION_EDGE) {
					inside=i+1; break;
				}
			}
			if (inside) {
				// If widget isn't selected, unselect everything else and select this
				if (!selected(inside)) {
					for (int i=0;i<children();i++) {
						// We cannot use select(i+1,0) because that would mess with focus
						Fl_Button* b = (Fl_Button*)child(i);
						b->color(FL_BACKGROUND2_COLOR);
						b->labelcolor(FL_FOREGROUND_COLOR);
					}
					int i=0;
					while (m_selected[i]!=0) m_selected[i++]=0;
					select(inside,1);
					redraw(); // show changes in selection state
				}
	
				// Construct dnd string and start dnd
				edelib::String selected_items;
				for (int i=1; i<=children(); i++)
					if (selected(i)==1) {
						selected_items += "file://";
						selected_items += path(i);
						selected_items += "\r\n";
					}

				// If paste_callback_ isn't set, that means we don't support dnd
				if (paste_callback_) {
EDEBUG(DBG"- dnd.\n");
					dragx=ex; dragy=ey; // to test the min. distance
					Fl::belowmouse(this); // without this, we will never get FL_DND_RELEASE!!!
					Fl::copy(selected_items.c_str(),selected_items.length(),0);
					Fl::dnd();
				}
				return 1;

			} else { // if(inside)...

				// Drag to select (a.k.a. "laso") operation (outside widgets)
				// Draw a dashed line around icons
EDEBUG(DBG"- laso begin.\n");
				laso=true;
				// Set coordinates for selection box (drawn in draw())
				select_x1=select_x2=Fl::event_x();
				select_y1=select_y2=Fl::event_y();
			}


		} else { // if (!laso) ...

			// Ongoing laso operation, update selection coordinates
			select_x2=Fl::event_x();
			select_y2=Fl::event_y();
			redraw();
EDEBUG(DBG"- laso box (%d,%d,%d,%d).\n",select_x1,select_y1,select_x2,select_y2);
		}
		return 1;
	} // if (e==FL_DRAG) ...





	// ------------------------------------------------
	//    FL_RELEASE does many things: 
	//    - (un)select items,
	//    - terminate laso operation,
	//    - show context menu,
	//    - doubleclick,
	// ------------------------------------------------


	// Mouse button released
	if (e==FL_RELEASE) {
EDEBUG(DBG"FL_RELEASE! ");

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

		if (scrolling) {
EDEBUG(DBG"- stop scrolling.\n");
			scrolling=false;
		}

		// Stop laso operation
		if (laso) {
EDEBUG(DBG"- stop laso.\n");
			laso=false;

			// Order coordinates
			int tmp;
			if (select_x1>select_x2) { tmp=select_x1; select_x1=select_x2; select_x2=tmp; }
			if (select_y1>select_y2) { tmp=select_y1; select_y1=select_y2; select_y2=tmp; }

			// Exclude edges
			select_x1 += SELECTION_EDGE;
			select_y1 += SELECTION_EDGE;
			select_x2 -= SELECTION_EDGE;
			select_y2 -= SELECTION_EDGE;
EDEBUG(DBG"After fixing the box coords: (%d,%d,%d,%d)\n", select_x1, select_y1, select_x2, select_y2);

			// Find which buttons were lasoed
			int i;
			for (i=0; i<children(); i++) {
				Fl_Widget* w = child(i); // some shortcuts
				int wx2 = w->x()+w->w();
				int wy2 = w->y()+w->h();
				// ignore the empty space below label -- equals number of \n's
				const char* lbl=w->label();
				for (int j=strlen(lbl)-2; j>0; j--) {
					if (lbl[j]!='\n') break;
					wy2 -= fl_size();
				}
				if (select_x2>w->x() && select_x1<wx2 && select_y2>w->y() && select_y1<wy2) {
					select(i+1,1);
EDEBUG(DBG"Select widget: '%20s' (%d,%d,%d,%d)\n", w->label(), w->x(), w->y(), wx2, wy2);
				}
			}
			// Shift key is the same as Ctrl with laso

			select_x1=select_x2=select_y1=select_y2=0;
			redraw();

		// Single click
		} else { // if(laso)...

			// Find child that was clicked
			int i;	
			for (i=0; i<children(); i++)
				if (Fl::event_inside(child(i))) {
EDEBUG(DBG"in child %d\n",i);
					// Shift key means "select everything in between"
					if (Fl::event_state(FL_SHIFT)) {
						int dir = (get_focus()<i) ? 1 : -1;
						for (int j=get_focus(); j!=i+1; j+=dir)
							select(j,1);
					}
					select(i+1,1);
				}

			// Didn't click on icon - remove focus from all icons
			if (i==children()) {
				this->take_focus(); 
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


	// ------------------------------------------------
	//    Drag&drop support (apart from FL_DRAG)
	// ------------------------------------------------

	// If paste_callback_ isn't set, that means we don't support dnd
	if (paste_callback_) {
if (e==FL_DND_ENTER) { EDEBUG(DBG"FL_DND_ENTER\n"); }
if (e==FL_DND_DRAG) { EDEBUG(DBG"FL_DND_DRAG\n"); }
if (e==FL_DND_RELEASE) { EDEBUG(DBG"FL_DND_RELEASE\n"); }
		// Let the window manager know that we accept dnd
		if (e==FL_DND_ENTER||e==FL_DND_LEAVE||e==FL_DND_DRAG) return 1;

		static bool dndrelease=false;
		if (e==FL_DND_RELEASE) {
EDEBUG(DBG"FL_DND_RELEASE '%s'\n", Fl::event_text());
			// Sometimes drag is accidental
			if (abs(Fl::event_x()-dragx)<MIN_DISTANCE_FOR_DND && abs(Fl::event_y()-dragy)<MIN_DISTANCE_FOR_DND)
				return 0;
				// return 1 would call Fl::paste(*belowmouse(),0) (see fl_dnd_x.cxx around line 168).
				// In our case that could be catastrophic

			dndrelease=true;
			Fl::paste(*this,0);
		}

		if (e==FL_PASTE) {
EDEBUG(DBG"FL_PASTE\n");
			if (!Fl::event_text() || !Fl::event_length()) return 1;
EDEBUG(DBG"1 '%s' (%d)\n",Fl::event_text(),Fl::event_length());

			// Paste comes from menu/keyboard
			if (!dndrelease) {
				paste_callback_(0); // 0 = current dir
				return 1;
			}
			dndrelease=false;

			// Where is the user dropping?
			// It it's inside an item, try pasting
			int ex=Fl::event_x(); int ey=Fl::event_y();
			for (int i=0; i<children(); i++) {
				Fl_Button* b = (Fl_Button*)child(i);
				if (ex>b->x()+SELECTION_EDGE && ex<b->x()+b->w()-SELECTION_EDGE && ey>b->y()+SELECTION_EDGE && ey<b->y()+b->h()-SELECTION_EDGE) {
					paste_callback_(path(i+1));
					return 0;
				}
			}

			// Nothing found... assume current directory
			paste_callback_(0);
			return 0;
		}

	} // if(paste_callback_)...


	// End of handle()
#ifdef USE_FLU_WRAP_GROUP
	return Flu_Wrap_Group::handle(e);
#else
	return edelib::ExpandableGroup::handle(e);
#endif

}


/* $Id */
