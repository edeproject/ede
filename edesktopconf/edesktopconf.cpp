/*
 * $Id$
 *
 * Desktop configuration tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <edelib/Nls.h>
#include <edelib/Color.h>
#include <edelib/Window.h>
#include <edelib/Debug.h>

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Color_Chooser.h>
#include <FL/Fl_File_Chooser.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/Fl_Menu_Button.h>

Fl_Menu_Item mode_menu[] = {
	{_("Center"), 0, 0},
	{_("Stretch"), 0, 0},
	{_("Stretch (aspect)"), 0, 0},
	{_("Tiled"), 0, 0},
	{0}
};

Fl_Button*       browse;
Fl_Button*       desk_background_color;
Fl_Input*        desk_background;
Fl_Choice*       desk_background_mode;
Fl_Check_Button* desk_use_wallpaper;

Fl_Box* wallpaper;

Fl_Button*       icon_background_color;
Fl_Button*       icon_label_color;
Fl_Check_Button* icon_show_background_color;
Fl_Check_Button* icon_show_label;
Fl_Value_Slider* icon_font_size;
Fl_Value_Slider* icon_label_width;

Fl_Check_Button* engage_with_one_click;


void set_wallpaper(const char* path) {
	if(!path)
		return;

	const char* old = desk_background->value();
	if(!old || strcmp(old, path) != 0)
		desk_background->value(path);

	Fl_Image* img = Fl_Shared_Image::get(path);
	if(!img)
		return;

	Fl_Image* scaled = img->copy(wallpaper->w() - 2, wallpaper->h() - 2);
	wallpaper->image(scaled);
	wallpaper->redraw();
}

void disable_wallpaper(bool doit) {
	if(doit) {
		desk_use_wallpaper->value(0);
		desk_background_mode->deactivate();
		desk_background->deactivate();
		browse->deactivate();
		wallpaper->color(desk_background_color->color());

		if(wallpaper->image()) {
			wallpaper->image(NULL);
			wallpaper->redraw();
		}
	} else {
		desk_use_wallpaper->value(1);
		desk_background_mode->activate();
		desk_background->activate();
		browse->activate();
		set_wallpaper(desk_background->value());
	}
}

void set_rgb_color(Fl_Button* btn, unsigned char r, unsigned char g, unsigned char b) {
	Fl_Color c = (Fl_Color)edelib::color_rgb_to_fltk(r, g, b);
	btn->color(c);
}

void close_cb(Fl_Widget*, void* w) {
	edelib::Window* win = (edelib::Window*)w;
	win->hide();
}

void color_cb(Fl_Button* btn, void*) {
	unsigned char r, g, b;

	edelib::color_fltk_to_rgb(btn->color(), r, g, b);

	if(fl_color_chooser(_("Choose color"), r, g, b)) {
		btn->color(edelib::color_rgb_to_fltk(r, g, b));
		btn->redraw();
	}
}

void wallpaper_color_cb(Fl_Widget*, void* w) {
	unsigned char r, g, b;

	edelib::color_fltk_to_rgb(desk_background_color->color(), r, g, b);

	if(fl_color_chooser(_("Choose color"), r, g, b)) {
		Fl_Color c = (Fl_Color)edelib::color_rgb_to_fltk(r, g, b);
		desk_background_color->color(c);
		wallpaper->color(c);
		wallpaper->redraw();
	}
}

void browse_cb(Fl_Widget*, void*) {
	char* ret = fl_file_chooser(_("Background image"), "*.jpg\t*.png", NULL);
	if(!ret)
		return;

	set_wallpaper(ret);
}

void wallpaper_use_cb(Fl_Widget*, void*) {
	if(desk_use_wallpaper->value()) {
		disable_wallpaper(false);
		set_wallpaper(desk_background->value());
	} else
		disable_wallpaper(true);
}

void apply_cb(Fl_Widget*, void* w) {
	unsigned char r, g, b;
	edelib::Window* win = (edelib::Window*)w;

	win->xsettings()->set("EDE/Desktop/Background/Wallpaper", desk_background->value());
	win->xsettings()->set("EDE/Desktop/Background/WallpaperMode", desk_background_mode->value());
	win->xsettings()->set("EDE/Desktop/Background/WallpaperShow", desk_use_wallpaper->value());

	edelib::color_fltk_to_rgb(desk_background_color->color(), r, g, b);
	win->xsettings()->set("EDE/Desktop/Background/Color", r, g, b, 0);

	edelib::color_fltk_to_rgb(icon_background_color->color(), r, g, b);
	win->xsettings()->set("EDE/Desktop/Icons/BackgroundColor", r, g, b, 0);

	edelib::color_fltk_to_rgb(icon_label_color->color(), r, g, b);
	win->xsettings()->set("EDE/Desktop/Icons/LabelColor", r, g, b, 0);

	win->xsettings()->set("EDE/Desktop/Icons/BackgroundColorShow", icon_show_background_color->value());
	win->xsettings()->set("EDE/Desktop/Icons/LabelShow", icon_show_label->value());
	win->xsettings()->set("EDE/Desktop/Icons/FontSize", (int)icon_font_size->value());
	win->xsettings()->set("EDE/Desktop/Icons/LabelWidth", (int)icon_label_width->value());

	win->pause_xsettings_callback();
	win->xsettings()->manager_notify();
	win->restore_xsettings_callback();
}

void okXX_cb(Fl_Widget*, void* w) {
	edelib::Window* win = (edelib::Window*)w;
	apply_cb(0, win);
	//win->xsettings()->clear();
	win->hide();
}

bool xsettings_cb(const char* name, edelib::XSettingsAction action, const edelib::XSettingsSetting* setting, void* data) {
	if(strcmp(name, "EDE/Desktop/Background/Wallpaper") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_STRING)
			disable_wallpaper(true);
		else 
			set_wallpaper(setting->data.v_string);

	} else if(strcmp(name, "EDE/Desktop/Background/Color") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_COLOR)
				desk_background_color->color(FL_BLACK);
		else set_rgb_color(icon_label_color,
					setting->data.v_color.red,
					setting->data.v_color.green,
					setting->data.v_color.blue);
	} else if(strcmp(name, "EDE/Desktop/Background/WallpaperMode") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			desk_background_mode->value(0);
		else {
			int m = setting->data.v_int;
			if(m > 3) {
				EWARNING(ESTRLOC ": Got wrong value from %s (%i), zeroing...\n", setting->name, setting->data.v_int);
				m = 0;
			}
			desk_background_mode->value(m);
		}
	} else if(strcmp(name, "EDE/Desktop/Background/WallpaperShow") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			disable_wallpaper(true);
		else {
			if(setting->data.v_int)
				disable_wallpaper(false);
			else
				disable_wallpaper(true);
		}
	} else if(strcmp(name, "EDE/Desktop/Icons/BackgroundColor") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_COLOR)
			icon_background_color->color(FL_BLUE);
		else
			set_rgb_color(icon_background_color, 
					setting->data.v_color.red,
					setting->data.v_color.green,
					setting->data.v_color.blue);
	} else if(strcmp(name, "EDE/Desktop/Icons/LabelColor") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_COLOR)
			icon_label_color->color(FL_WHITE);
		else
			set_rgb_color(icon_label_color,
					setting->data.v_color.red,
					setting->data.v_color.green,
					setting->data.v_color.blue);
	} else if(strcmp(name, "EDE/Desktop/Icons/BackgroundColorShow") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			icon_show_background_color->value(0);
		else {
			if(setting->data.v_int)
				icon_show_background_color->value(1);
			else
				icon_show_background_color->value(0);
		}
	} else if(strcmp(name, "EDE/Desktop/Icons/LabelShow") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			icon_show_label->value(0);
		else {
			if(setting->data.v_int)
				icon_show_label->value(1);
			else
				icon_show_label->value(0);
		}
	} else if(strcmp(name, "EDE/Desktop/Icons/FontSize") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			icon_font_size->value(12);
		else
			icon_font_size->value(setting->data.v_int);
	} else if(strcmp(name, "EDE/Desktop/Icons/LabelWidth") == 0) {
		if(action == edelib::XSETTINGS_ACTION_DELETED || setting->type != edelib::XSETTINGS_TYPE_INT)
			icon_label_width->value(55);
		else
			icon_label_width->value(setting->data.v_int);
	}

	return false;
}

int main() {
	edelib::Window* win = new edelib::Window(550, 285, _("Desktop options"));
	win->xsettings_callback(xsettings_cb, NULL);

	win->begin();
		Fl_Tabs* tabs = new Fl_Tabs(10, 10, 530, 230);
		tabs->begin();
			Fl_Group* g1 = new Fl_Group(20, 30, 510, 200, _("Background"));
			g1->begin();
				Fl_Box* b1 = new Fl_Box(85, 196, 100, 15);
				b1->box(FL_BORDER_BOX);

				Fl_Box* b2 = new Fl_Box(30, 43, 210, 158);
				b2->box(FL_THIN_UP_BOX);

				/* box size is intentionaly odd so preserve aspect ratio */
				wallpaper = new Fl_Box(43, 53, 184, 138);
				wallpaper->box(FL_DOWN_BOX);
				wallpaper->color(FL_BLACK);

				Fl_Box* b4 = new Fl_Box(60, 206, 145, 14);
				b4->box(FL_THIN_UP_BOX);

				desk_background = new Fl_Input(295, 80, 190, 25, _("Image:"));

				browse = new Fl_Button(490, 80, 25, 25, "...");
				browse->callback(browse_cb);

				desk_background_mode = new Fl_Choice(295, 116, 220, 24, _("Mode:"));
				desk_background_mode->down_box(FL_BORDER_BOX);
				desk_background_mode->menu(mode_menu);

				desk_background_color = new Fl_Button(295, 161, 25, 24, _("Background color"));
				desk_background_color->color(FL_BLACK);
				desk_background_color->align(FL_ALIGN_RIGHT);
				desk_background_color->callback(wallpaper_color_cb);

				desk_use_wallpaper = new Fl_Check_Button(295, 45, 220, 25, _("Use wallpaper"));
				desk_use_wallpaper->down_box(FL_DOWN_BOX);
				desk_use_wallpaper->value(1);
				desk_use_wallpaper->callback(wallpaper_use_cb);
			g1->end();

			Fl_Group* g2 = new Fl_Group(20, 30, 510, 195, _("Icons"));
			g2->hide();
			g2->begin();
				icon_background_color = new Fl_Button(30, 51, 25, 24, _("Background color"));
				icon_background_color->color(FL_BLUE);
				icon_background_color->align(FL_ALIGN_RIGHT);
				icon_background_color->callback((Fl_Callback*)color_cb);

				icon_label_color = new Fl_Button(30, 91, 25, 24, _("Label color"));
				icon_label_color->color(FL_WHITE);
				icon_label_color->align(FL_ALIGN_RIGHT);
				icon_label_color->callback((Fl_Callback*)color_cb);

				icon_show_background_color = new Fl_Check_Button(260, 50, 220, 25, _("Show background color"));
				icon_show_background_color->down_box(FL_DOWN_BOX);

				icon_show_label = new Fl_Check_Button(260, 90, 220, 25, _("Show label"));
				icon_show_label->down_box(FL_DOWN_BOX);

				icon_font_size = new Fl_Value_Slider(30, 155, 220, 20, _("Font size"));
				icon_font_size->type(5);
				icon_font_size->step(1);
				icon_font_size->maximum(48);
				icon_font_size->align(FL_ALIGN_TOP_LEFT);

				icon_label_width = new Fl_Value_Slider(260, 155, 220, 20, _("Label width"));
				icon_label_width->type(5);
				icon_label_width->step(1);
				icon_label_width->maximum(200);
				icon_label_width->align(FL_ALIGN_TOP_LEFT);
			g2->end();

			Fl_Group* g3 = new Fl_Group(20, 30, 510, 195, _("Icons behaviour"));
			g3->hide();
			g3->begin();
				engage_with_one_click = new Fl_Check_Button(30, 50, 220, 25, _("Engage with just one click"));
				engage_with_one_click->down_box(FL_DOWN_BOX);
			g3->end();
		tabs->end();

		Fl_Button* ok = new Fl_Button(260, 250, 90, 25, _("&OK"));
		ok->callback(okXX_cb, win);
		Fl_Button* apply = new Fl_Button(355, 250, 90, 25, _("&Apply"));
		apply->callback(apply_cb, win);
		Fl_Button* cancel = new Fl_Button(450, 250, 90, 25, _("&Cancel"));
		cancel->callback(close_cb, win);
	win->end();

	win->init(edelib::WIN_INIT_IMAGES);
	win->show();
	return Fl::run();
}
