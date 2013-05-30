/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2006-2013 Sanel Zukan
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

#include <FL/Fl_Window.H>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>

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

EDELIB_NS_USING_LIST(10, (str_tolower, icon_chooser, dir_home, build_filename, alert,
						  ICON_SIZE_HUGE, String, IconLoader, DesktopFile, DESK_FILE_TYPE_APPLICATION))

#define DEFAULT_ICON "empty"

/* it is safe to be globals */
static Fl_Window *win;
static Fl_Button *img, *browse, *ok, *cancel;
static Fl_Input  *name, *comment, *execute;
static Fl_Choice *icon_type;
static String    img_path;

/* the only supported type for now is application */
static Fl_Menu_Item menu_items[] = {
	{_("Application"), 0, 0, 0},
	{0}
};

static bool is_empty(const char *str) {
	if(!str) return true;
	const char *p = str;

	while(*p++)
		if(!isspace(*p)) return false;

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

	/* determine filename and save it */
	String file = name->value();
	const char *fp = file.c_str();

	str_tolower((unsigned char*)fp);
	file += ".desktop";

	/* go through the file and replace spaces with '_' */
	for(String::size_type i = 0; i < file.length(); i++)
		if(isspace(file[i])) file[i] = '_';

	String path = build_filename(self->desktop_path(), file.c_str());

	if(df.save(path.c_str())) {
		DesktopIcon *ic = self->read_desktop_file(path.c_str(), file.c_str());
		if(ic) self->add(ic);
		self->redraw();
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
	if(!p) return;
	execute->value(p);
}

void icon_dialog_icon_create(Desktop *self) {
	win = new Fl_Window(430, 170, _("Create desktop icon"));
		img = new Fl_Button(10, 10, 75, 75);
		img->callback(img_browse_cb);
		img->tooltip(_("Click to select icon"));
		IconLoader::set(img, DEFAULT_ICON, ICON_SIZE_HUGE);
		name = new Fl_Input(205, 10, 215, 25, _("Name:"));
		comment = new Fl_Input(205, 40, 215, 25, _("Comment:"));
		execute = new Fl_Input(205, 70, 185, 25, _("Execute:"));
		browse = new Fl_Button(395, 70, 25, 25, "...");
		browse->callback(file_browse_cb);
		icon_type = new Fl_Choice(205, 100, 215, 25, _("Type:"));
		icon_type->down_box(FL_BORDER_BOX);
		icon_type->menu(menu_items);

		ok = new Fl_Button(235, 135, 90, 25, _("&OK"));
		ok->callback(ok_cb, self);
		cancel = new Fl_Button(330, 135, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);
	win->end();
	win->set_modal();

	Fl::focus(name);
	win->show();
}	

void icon_dialog_icon_property(DesktopIcon *d) {
}	
