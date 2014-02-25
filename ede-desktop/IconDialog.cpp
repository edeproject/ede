/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2006-2014 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <ctype.h>
#include <limits.h>

#include <FL/Fl_Window.H>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Check_Button.H>

#include <edelib/Nls.h>
#include <edelib/String.h>
#include <edelib/IconChooser.h>
#include <edelib/DesktopFile.h>
#include <edelib/Debug.h>
#include <edelib/Directory.h>
#include <edelib/File.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/MessageBox.h>
#include <edelib/IconLoader.h>

#include "IconDialog.h"
#include "DesktopIcon.h"
#include "Desktop.h"
#include "Globals.h"

EDELIB_NS_USING_LIST(12, (str_tolower, icon_chooser, dir_home, build_filename, alert, ask, file_remove,
						  ICON_SIZE_HUGE, String, IconLoader, DesktopFile, DESK_FILE_TYPE_APPLICATION))

#define DEFAULT_ICON "empty"

/* it is safe to be globals */
static Fl_Window       *win;
static Fl_Button       *img, *browse, *ok, *cancel;
static Fl_Input        *name, *comment, *execute, *workdir;
static Fl_Choice       *icon_type;
static Fl_Check_Button *run_in_terminal, *start_notify;
static String          img_path, old_desktop_path;
static DesktopIcon     *curr_icon;

/* the only supported type for now is application */
static Fl_Menu_Item menu_items[] = {
	{_("Application"), 0, 0, 0},
	{0}
};

static bool is_empty(const char *str) {
	if(!str) return true;
	const char *p = str;

	while(*p++) {
		if(!isspace(*p)) return false;
	}

	return true;
}

inline bool is_empty_input(Fl_Input *i) {
	return is_empty(i->value());
}

static void cancel_cb(Fl_Widget*, void*) {
	win->hide();
}

static void ok_cb(Fl_Widget*, void *d) {
	if(is_empty_input(name) || is_empty_input(execute) || !img->image()) {
		/* do nothing */
		win->hide();
		return;
	}
	
	Desktop *self = (Desktop*)d;
	
	DesktopFile df;
	df.create_new(DESK_FILE_TYPE_APPLICATION);
	df.set_name(name->value());

	if(comment->value())
		df.set_comment(comment->value());

	df.set_icon((img_path.length() > 1) ? img_path.c_str() : DEFAULT_ICON);
	df.set_exec(execute->value());
	
	if(!is_empty_input(workdir))
		df.set_path(workdir->value());
	
	df.set_startup_notify(start_notify->value());
	df.set_terminal(run_in_terminal->value());

	/* determine filename and save it */
	String file = name->value();
	const char *fp = file.c_str();

	str_tolower((unsigned char*)fp);
	file += EDE_DESKTOP_DESKTOP_EXT;

	/* go through the file and replace spaces with '_' */
	for(String::size_type i = 0; i < file.length(); i++)
		if(isspace(file[i])) file[i] = '_';

	String path = build_filename(self->desktop_path(), file.c_str());
	
	int  X = 0, Y = 0;
	if(curr_icon) {
		X = curr_icon->x();
		Y = curr_icon->y();
		/* try to remove icon from filesystem only when we can't overwrite old icon path */
		self->remove_icon(curr_icon, old_desktop_path != path);
	}

	if(df.save(path.c_str())) {
		DesktopIcon *ic = self->read_desktop_file(path.c_str(), file.c_str());
		if(ic) {
			if(X > 0 || Y > 0) ic->position(X, Y);
			self->add(ic);
		}

		self->redraw();
		
		/* 
		 * In case when we rename icon, icon position will not be saved (because they are saved by icon basename). So
		 * with different paths we are assured the name was changed and we proceed further.
		 */
		if(old_desktop_path != path) self->save_icons_positions();
	} else {
		alert(_("Unable to create '%s' file. Received error is: %s\n"), path.c_str(), df.strerror());
	}

	win->hide();
}

static void img_browse_cb(Fl_Widget*, void*) {
	img_path = icon_chooser(ICON_SIZE_HUGE);

	if(!img_path.empty())
		IconLoader::set(img, img_path.c_str(), ICON_SIZE_HUGE);
}	

static void file_browse_cb(Fl_Widget*, void*) {
	const char *p = fl_file_chooser(_("Choose program path to execute"), "*", 0, 0);
	if(p) execute->value(p);
}

static void dir_browse_cb(Fl_Widget*, void*) {
	const char *p = fl_dir_chooser(_("Choose directory"), "*", 0);
	if(p) workdir->value(p);
}

#define BUFSIZE PATH_MAX

void icon_dialog_icon_edit(Desktop *self, DesktopIcon *d) {
	const char  *lbl = d ? _("Edit desktop icon") : _("Create desktop icon");
	DesktopFile *df  = NULL;
	char        *buf = NULL;
	old_desktop_path = "";
	curr_icon        = d;
	
	if(d) {
		df = new DesktopFile();
		if(!df->load(d->get_path())) {
			delete df;
			df = NULL;

			int ret = ask(_("Unable to load .desktop file for this icon. Would you like to create a new icon instead?"));
			if(!ret) return;

			/* force NULL on icon, so we can run dialog in 'create' mode */
			d = NULL;
		}
		
		buf = new char[BUFSIZE];
		old_desktop_path = d->get_path();
	}
	
	win = new Fl_Window(450, 220, lbl);
	Fl_Tabs *tabs = new Fl_Tabs(10, 10, 430, 165);
	tabs->begin();
		Fl_Group *g1 = new Fl_Group(15, 30, 415, 140, _("Basic"));
		g1->begin();
			img = new Fl_Button(20, 45, 80, 75);
			img->callback(img_browse_cb);
			img->tooltip(_("Click to select icon"));
			
			if(d) {
				E_ASSERT(df != NULL);
				if(df->icon(buf, BUFSIZE)) {
					IconLoader::set(img, buf, ICON_SIZE_HUGE);
					img_path = buf;
				} 
			}

			/* handles even the case when we are creating the new icon */
			if(!img->image()) {
				IconLoader::set(img, DEFAULT_ICON, ICON_SIZE_HUGE);
				img_path = DEFAULT_ICON;
			}
			
			name = new Fl_Input(210, 45, 215, 25, _("Name:"));
			if(d && df->name(buf, BUFSIZE)) name->value(buf);
			
			comment = new Fl_Input(210, 75, 215, 25, _("Comment:"));
			if(d && df->comment(buf, BUFSIZE)) comment->value(buf);
			
			execute = new Fl_Input(210, 105, 185, 25, _("Execute:"));
			if(d && df->exec(buf, BUFSIZE)) execute->value(buf);
			
			browse = new Fl_Button(400, 105, 25, 25, "...");
			browse->callback(file_browse_cb);
			
			icon_type = new Fl_Choice(210, 135, 215, 25, _("Type:"));
			icon_type->down_box(FL_BORDER_BOX);
			icon_type->menu(menu_items);
		g1->end();		   
		
		Fl_Group *g2 = new Fl_Group(15, 30, 420, 140, _("Details"));
		g2->hide();
		g2->begin();
			run_in_terminal = new Fl_Check_Button(195, 80, 235, 25, _("Run in terminal"));
			run_in_terminal->down_box(FL_DOWN_BOX);
			if(df) run_in_terminal->value(df->terminal());

			workdir = new Fl_Input(195, 45, 205, 25, _("Working directory:"));
			if(df && df->path(buf, BUFSIZE)) workdir->value(buf);

			Fl_Button *browsedir = new Fl_Button(405, 45, 25, 25, "...");
			browsedir->callback(dir_browse_cb);

			start_notify = new Fl_Check_Button(195, 110, 235, 25, _("Use startup notification"));
			start_notify->down_box(FL_DOWN_BOX);
			if(df) start_notify->value(df->startup_notify());
		g2->end();
	tabs->end();	

	ok = new Fl_Button(255, 185, 90, 25, _("&OK"));
	ok->callback(ok_cb, self);
	cancel = new Fl_Button(350, 185, 90, 25, _("&Cancel"));
	cancel->callback(cancel_cb);

	win->end();
	win->set_modal();
	
	delete df;
	delete buf;

	Fl::focus(name);
	win->show();
}
