/*
 * $Id$
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Shared_Image.H>

#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/IconTheme.h>
#include <edelib/MimeType.h>
#include <edelib/IconChooser.h>

#include "IconProperties.h"
#include "DesktopIcon.h"

struct DesktopIconData {
	Fl_Image*   image;
	const char* label;
	const char* location;

	edelib::String time_access;
	edelib::String time_mod;
	edelib::String type;
	edelib::String size;
};

static void close_cb(Fl_Widget*, void* w) {
	Fl_Window* win = (Fl_Window*)w;
	win->hide();
}

static void ok_cb(Fl_Widget*, void* w) {
	close_cb(0, w);
}

static void icon_change_cb(Fl_Button* b, void* d) {
	DesktopIconData* data = (DesktopIconData*)d;

	edelib::String ret = edelib::icon_chooser(edelib::ICON_SIZE_HUGE);
	if(ret.empty())
		return;

	Fl_Image* img = Fl_Shared_Image::get(ret.c_str());
	if(!img)
		return;
	b->image(img);
	b->redraw();
}

static void study_and_fill(DesktopIcon* dicon, DesktopIconData* data) {
	// get image and scale it if needed
	if(dicon->image()) {
		int iw = dicon->w();
		int ih = dicon->h();

		if(iw > 64 || ih > 64) {
			data->image = dicon->image()->copy((iw > 64) ? 64 : iw, 
											   (ih > 64) ? 64 : ih);
		} else {
			data->image = dicon->image();
		}
	}

	data->label = dicon->label();

	const char* prog_path = dicon->path().c_str();
	edelib::MimeType mt;
	if(mt.set(prog_path))
		data->type = mt.comment();
	else
		data->type = "(unknown)";

	data->location = prog_path;

	// now get size, access and modification times
	struct stat st;
	if(stat(prog_path, &st) == -1) {
		data->size        = "(unknown)";
		data->time_access = "(unknown)";
		data->time_mod    = "(unknown)";
	} else {
		char buff[128];

		snprintf(buff, sizeof(buff), "%lu Bytes", st.st_size);
		data->size = buff;

		strftime(buff, sizeof(buff), "%F %H:%S", localtime(&st.st_atime));
		data->time_access = buff;

		strftime(buff, sizeof(buff), "%F %H:%S", localtime(&st.st_mtime));
		data->time_mod = buff;
	}
}

void show_icon_properties_dialog(DesktopIcon* dicon) {
	// fetch usefull data
	DesktopIconData data;
	study_and_fill(dicon, &data);

	// now construct dialog and fill it
	edelib::Window* win = new edelib::Window(315, 395, _("Icon properties"));
	win->begin();
		Fl_Tabs* tabs = new Fl_Tabs(10, 10, 295, 340);
		tabs->begin();
			Fl_Group* g1 = new Fl_Group(15, 35, 285, 310, _("General"));
			g1->begin();
				Fl_Button* icon_button = new Fl_Button(20, 50, 75, 75);
				icon_button->image(data.image);
				icon_button->callback((Fl_Callback*)icon_change_cb, &data);
				icon_button->tooltip(_("Click to change icon"));

				Fl_Input* prog_name = new Fl_Input(100, 75, 195, 25);
				prog_name->value(data.label);

				Fl_Output* prog_mime = new Fl_Output(100, 150, 195, 25, _("Type:"));
				prog_mime->box(FL_FLAT_BOX);
				prog_mime->color(FL_BACKGROUND_COLOR);
				prog_mime->value(data.type.c_str());

				Fl_Output* prog_location = new Fl_Output(100, 180, 195, 25, _("Location:"));
				prog_location->box(FL_FLAT_BOX);
				prog_location->color(FL_BACKGROUND_COLOR);
				prog_location->value(data.location);

				Fl_Output* prog_size  = new Fl_Output(100, 210, 195, 25, _("Size:"));
				prog_size->box(FL_FLAT_BOX);
				prog_size->color(FL_BACKGROUND_COLOR);
				prog_size->value(data.size.c_str());

				Fl_Output* prog_date_mod = new Fl_Output(100, 240, 195, 25, _("Modified:"));
				prog_date_mod->box(FL_FLAT_BOX);
				prog_date_mod->color(FL_BACKGROUND_COLOR);
				prog_date_mod->value(data.time_mod.c_str());

				Fl_Output* prog_date_acessed = new Fl_Output(100, 270, 195, 25, "Accessed:");
				prog_date_acessed->box(FL_FLAT_BOX);
				prog_date_acessed->color(FL_BACKGROUND_COLOR);
				prog_date_acessed->value(data.time_access.c_str());
			g1->end();

			Fl_Group* g2 = new Fl_Group(15, 35, 285, 310, _("URL"));
			g2->hide();
			g2->begin();
				Fl_Input* prog_url = new Fl_Input(20, 65, 245, 25, _("URL:"));
				prog_url->align(FL_ALIGN_TOP_LEFT);

				Fl_Button* prog_browse = new Fl_Button(270, 65, 25, 25);
				prog_browse->tooltip(_("Browse location"));

				// find icon for browse button
				edelib::String ii = edelib::IconTheme::get("document-open", edelib::ICON_SIZE_TINY);
				if(ii.empty())
					prog_browse->label("...");
				else {
					Fl_Image* ii_img = Fl_Shared_Image::get(ii.c_str());
					if(ii_img)
						prog_browse->image(ii_img);
					else
						prog_browse->label("...");
				}

			g2->end();
		tabs->end();

		Fl_Button* ok_button = new Fl_Button(120, 360, 90, 25, _("&OK"));
		ok_button->callback(ok_cb, win);
		Fl_Button* cancel_button = new Fl_Button(215, 360, 90, 25, _("&Cancel"));
		cancel_button->callback(close_cb, win);
    win->end();

	win->init(edelib::WIN_INIT_ALL);
	win->show();

	while(win->shown())
		Fl::wait();
}
