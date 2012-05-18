/*
 * $Id: IconProperties.h 2366 2008-10-02 09:42:19Z karijes $
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2012 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
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
#include "ede-desktop.h"

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

static void ok_cb(Fl_Widget*, void*) {
	if(is_empty_input(name) || is_empty_input(execute) || !img->image()) {
		/* do nothing */
		win->hide();
		return;
	}

	DesktopFile df;
	df.create_new(DESK_FILE_TYPE_APPLICATION);
	df.set_name(name->value());

	if(comment->value())
		df.set_comment(comment->value());

	if(!img_path.empty() && img_path.length() > 1) {
		/* figure out basename */
		const char *s;
		char *p, *e;

		s = img_path.c_str();
		p = (char*)strrchr(s, E_DIR_SEPARATOR);

		if(p && *p++) {
			/* now remove extension */
			e = (char*)strrchr((const char*)p, '.');
			if(e) *e = '\0';

			df.set_icon(p);
		}	
	} else {
		df.set_icon(DEFAULT_ICON);
	}

	df.set_exec(execute->value());

	/* determine filename and save it */
	String file = name->value();
	const char *fp = file.c_str();

	str_tolower((unsigned char*)fp);
	file += ".desktop";

	/* go through the file and replace spaces with '_' */
	for(char *p = (char*)file.c_str(); p && *p; p++)
		if(isspace(*p)) *p = '_';

	/* TODO: let 'Desktop' (class) returns full desktop path */
	String path = build_filename(Desktop::instance()->desktop_path(), file.c_str());

	/*
	 * disable watching on folder and explicitly add file (probably as notification will be fired up faster than
	 * file will be available on that location)
	 */
	Desktop::instance()->dir_watch_off();

	if(df.save(path.c_str())) {
		/* explictly add file path */
		Desktop::instance()->add_icon_by_path(path.c_str(), NULL);
		/*
		 * wait a second; this would remove event from the queue so watched does not complain how filed
		 * does not exists
		 */
		Fl::wait(1);
		Desktop::instance()->redraw();
	} else {
		alert(_("Unable to create '%s' file. Received error is: %s\n"), path.c_str(), df.strerror());
	}

	Desktop::instance()->dir_watch_on();
	win->hide();
}

static void img_browse_cb(Fl_Widget*, void*) {
	img_path = icon_chooser(ICON_SIZE_HUGE);
	if(img_path.empty()) return;

	Fl_Image* im = Fl_Shared_Image::get(img_path.c_str());
	if(!im) return;

	img->image(im);
	img->redraw();
}	

static void file_browse_cb(Fl_Widget*, void*) {
	const char *p = fl_file_chooser(_("Choose program path to execute"), "*", 0, 0);
	if(!p) return;
	execute->value(p);
}

void icon_dialog_icon_create(void) {
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
		ok->callback(ok_cb);
		cancel = new Fl_Button(330, 135, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);
	win->end();
	win->set_modal();

	Fl::focus(name);
	win->show();
}	

void icon_dialog_icon_property(DesktopIcon *d) {
}	
