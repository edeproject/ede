/*
 * $Id$
 *
 * Desktop configuration tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2014 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/x.H>

#include <edelib/Color.h>
#include <edelib/Window.h>
#include <edelib/WindowUtils.h>
#include <edelib/Resource.h>
#include <edelib/Debug.h>
#include <edelib/Util.h>
#include <edelib/FontChooser.h>
#include <edelib/Directory.h>
#include <edelib/ForeignCallback.h>
#include <edelib/Ede.h>
#include <edelib/Netwm.h>

EDELIB_NS_USING(netwm_workarea_get_size)

#define EDE_DESKTOP_CONFIG "ede-desktop"

Fl_Menu_Item mode_menu[] = {
	{_("Center"), 0, 0},
	{_("Stretch"), 0, 0},
	{_("Tile"), 0, 0},
	{0}
};

/* make sure this part matches array positions in mode_menu[] */
#define IMG_CENTER  0
#define IMG_STRETCH 1
#define IMG_TILE    2

Fl_Button*       browse;
Fl_Button*       desk_background_color;
Fl_Input*        desk_background;
Fl_Choice*       desk_background_mode;
Fl_Check_Button* desk_use_wallpaper;

Fl_Box*          wallpaper;
Fl_Box*          wallpaper_img;

Fl_Button*       icon_background_color;
Fl_Button*       icon_label_color;
Fl_Check_Button* icon_show_background_color;
Fl_Check_Button* icon_show_label;

int              icon_font;
int              icon_font_size;
Fl_Value_Slider* icon_label_width;
Fl_Input*        icon_font_txt;

Fl_Check_Button* engage_with_one_click;

#define PIXEL_POS(x, y, w, d) ((((y) * (w)) + (x)) * (d))

/* TODO: the same function exists in ede-desktop: move this to edelib */
static Fl_RGB_Image* create_tile(Fl_Image *orig, int X, int Y, int W, int H) {
	/* don't tile large image */
	if(orig->w() >= W && orig->h() >= H)
		return (Fl_RGB_Image*)orig;

	int iw = orig->w();
	int ih = orig->h();
	int idepth = orig->d();
	int tx = X - (X % iw);
	int ty = Y - (Y % ih);
	int tw = W + tx;
	int th = H + ty;

	unsigned char* dest = new unsigned char[tw * th * orig->d()];
	unsigned char* destptr = dest;
	unsigned char* src = (unsigned char*)orig->data()[0];
	int ppos = 0;
	/* for bounds checks */
	int imax = iw * ih * idepth;

	if(idepth == 3 || idepth == 4) {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
				*destptr++ = src[ppos + 1];
				*destptr++ = src[ppos + 2];

				if(idepth == 4)
					*destptr++ = src[ppos + 3];
			}
		}
	} else if(idepth == 2) {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
				*destptr++ = src[ppos + 1];
			}
		}
	} else {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
			}
		}
	}

	Fl_RGB_Image* c = new Fl_RGB_Image(dest, tw, th, idepth, orig->ld());
	c->alloc_array = 1;
	return c;
}

static void set_wallpaper(const char* path) {
	if(!path)
		return;

	// fill input if not given
	const char* old = desk_background->value();
	if(!old || strcmp(old, path) != 0)
		desk_background->value(path);

	Fl_Image* img = Fl_Shared_Image::get(path);
	if(!img)
		return;

	int area_x = wallpaper->x() + 2;
	int area_y = wallpaper->y() + 2;
	int area_w = wallpaper->w() - 4;
	int area_h = wallpaper->h() - 4;

	/*
	 * Before doing anything with the image, first scale it relative to wallpaper box,
	 * which is relative to the screen sizes.
	 */
	int X, Y, W, H;
	if(!netwm_workarea_get_size(X, Y, W, H))
		Fl::screen_xywh(X, Y, W, H);

	int scale_w_factor = (W > area_w) ? W / area_w : 1;
	int scale_h_factor = (H > area_h) ? H / area_h : 1;

	Fl_Image* rel_img = img->copy(img->w() / scale_w_factor, img->h() / scale_h_factor);

	/*
	 * Now all transformations will be applied on relative image
	 */
	int img_w = rel_img->w();
	int img_h = rel_img->h();

	wallpaper_img->position(area_x, area_y);

	switch(desk_background_mode->value()) {
		case IMG_STRETCH: {
			Fl_Image* transformed = rel_img->copy(area_w, area_h);
			wallpaper_img->size(area_w, area_h);
			wallpaper_img->image(transformed);
			break;
		}

		case IMG_TILE: {
			wallpaper_img->size(area_w, area_h);
			wallpaper_img->image(create_tile(rel_img, 0, 0, area_w, area_h));
			break;
		}

		case IMG_CENTER:
		default: // fallback
			if(img_w > wallpaper->w() && img_h > wallpaper->h()) {
				wallpaper_img->size(area_w, area_h);
				wallpaper_img->image(rel_img);
			} else {
				int pos_x = wallpaper_img->x();
				int pos_y = wallpaper_img->y();

				if(rel_img->w() < area_w)
					pos_x = pos_x + (area_w / 2) - (rel_img->w() / 2);
					
				if(rel_img->h() < area_h)
					pos_y = pos_y + (area_h / 2) - (rel_img->h() / 2);

				wallpaper_img->position(pos_x, pos_y);

				int sw = (rel_img->w() > area_w) ? area_w : rel_img->w();
				int sh = (rel_img->h() > area_h) ? area_h : rel_img->h();

				wallpaper_img->size(sw, sh);
				wallpaper_img->image(rel_img);
			}

			break;
	}
}

static void disable_wallpaper(bool doit) {
	if(doit) {
		desk_use_wallpaper->value(0);
		desk_background_mode->deactivate();
		desk_background->deactivate();
		wallpaper->color(desk_background_color->color());
		browse->deactivate();
		// hide image
		wallpaper_img->hide();
	} else {
		desk_use_wallpaper->value(1);
		desk_background_mode->activate();
		desk_background->activate();
		browse->activate();
		wallpaper_img->show();

		if(!wallpaper_img->image())
			set_wallpaper(desk_background->value());
	}

	wallpaper->redraw();
}

static void close_cb(Fl_Widget*, void* w) {
	edelib::Window* win = (edelib::Window*)w;
	win->hide();
}

static void color_cb(Fl_Widget* btn, void*) {
	unsigned char r, g, b;

	edelib::color_fltk_to_rgb(btn->color(), r, g, b);

	if(fl_color_chooser(_("Choose color"), r, g, b)) {
		btn->color(edelib::color_rgb_to_fltk(r, g, b));
		btn->redraw();
	}
}

static void wallpaper_color_cb(Fl_Widget*, void* w) {
	unsigned char r, g, b;

	edelib::color_fltk_to_rgb(desk_background_color->color(), r, g, b);

	if(fl_color_chooser(_("Choose color"), r, g, b)) {
		Fl_Color c = (Fl_Color)edelib::color_rgb_to_fltk(r, g, b);
		desk_background_color->color(c);
		desk_background_color->redraw();
		wallpaper->color(c);
		wallpaper_img->redraw();
		wallpaper->redraw();
	}
}

static void browse_cb(Fl_Widget*, void*) {
	char* ret = fl_file_chooser(_("Background image"), "*.jpg\t*.png", desk_background->value());
	if(!ret)
		return;

	set_wallpaper(ret);
	wallpaper_img->redraw();
	wallpaper->redraw();
}

static void wallpaper_use_cb(Fl_Widget*, void*) {
	if(desk_use_wallpaper->value()) {
		disable_wallpaper(false);
		set_wallpaper(desk_background->value());
	} else
		disable_wallpaper(true);
}

static void choice_cb(Fl_Widget*, void*) {
	set_wallpaper(desk_background->value());
	wallpaper_img->redraw();
	wallpaper->redraw();
}

static void apply_cb(Fl_Widget*, void* w) {
	edelib::Resource conf;
	conf.set("Desktop", "color", (int)desk_background_color->color());
	conf.set("Desktop", "wallpaper_use", desk_use_wallpaper->value());
	conf.set("Desktop", "wallpaper_mode", desk_background_mode->value());

	if(desk_background->value())
		conf.set("Desktop", "wallpaper", desk_background->value());

	conf.set("Icons", "label_background", (int)icon_background_color->color());
	conf.set("Icons", "label_foreground", (int)icon_label_color->color());
	conf.set("Icons", "label_font",       icon_font);
	conf.set("Icons", "label_fontsize",   icon_font_size);
	conf.set("Icons", "label_maxwidth",   icon_label_width->value());
	conf.set("Icons", "label_transparent", !icon_show_background_color->value());
	conf.set("Icons", "label_visible", icon_show_label->value());
	conf.set("Icons", "one_click_exec", engage_with_one_click->value());

	if(conf.save(EDE_DESKTOP_CONFIG))
		edelib::foreign_callback_call("ede-desktop");
}

static void ok_cb(Fl_Widget*, void* w) {
	edelib::Window* win = (edelib::Window*)w;
	apply_cb(0, win);
	/* a hack so ede-desktop-conf can send a message before it was closed */
	usleep(120000); 
	win->hide();
}

static void browse_fonts_cb(Fl_Widget*, void* w) {
	Fl_Input* in = (Fl_Input*)w;
	int retsz;
	const char* font_name = Fl::get_font_name((Fl_Font)icon_font, 0);
	int font = edelib::font_chooser(_("Choose font"), "iso8859-2", retsz, font_name, icon_font_size);
	if(font == -1)
		return;

	font_name = Fl::get_font_name((Fl_Font)font, 0);
	in->value(font_name);

	icon_font = font;
	icon_font_size = retsz;
}

static void load_settings(void) {
	int b_mode = 0;
	bool d_wp_use = false;
	int d_background_color = fl_rgb_color(73, 64, 102); /* keep it in sync with default color in ede-desktop */
	int i_background_color = FL_BLUE;
	int i_label_color = FL_WHITE;
	int i_show_background_color = 0;
	int i_show_label = 1;
	int i_label_width = 55;
	bool one_click = false;

	edelib::Resource conf;
	char wpath[256];
	bool wpath_found = false;

	if(conf.load(EDE_DESKTOP_CONFIG)) {
		conf.get("Desktop", "color", d_background_color, d_background_color); 
		conf.get("Desktop", "wallpaper_use", d_wp_use, d_wp_use);
		conf.get("Desktop", "wallpaper_mode", b_mode, b_mode);

		if(conf.get("Desktop", "wallpaper", wpath, sizeof(wpath)))
			wpath_found = true;
		else
			wpath_found = false;

		conf.get("Icons", "label_background", i_background_color, i_background_color);
		conf.get("Icons", "label_foreground", i_label_color, i_label_color);
		conf.get("Icons", "label_font", icon_font, 1);
		conf.get("Icons", "label_fontsize",   icon_font_size, 12);
		conf.get("Icons", "label_maxwidth",   i_label_width, i_label_width);
		conf.get("Icons", "label_transparent",i_show_background_color, i_show_background_color);
		conf.get("Icons", "label_visible",    i_show_label, i_show_label);
		conf.get("Icons", "one_click_exec",     one_click, one_click);
	}

	desk_background_color->color(d_background_color);
	wallpaper->color(d_background_color);
	desk_background_mode->value(b_mode);
	desk_use_wallpaper->value(d_wp_use);

	if(d_wp_use && wpath_found)
		set_wallpaper(wpath);
	else
		disable_wallpaper(true);

	icon_background_color->color(i_background_color);
	icon_label_color->color(i_label_color);
	icon_show_background_color->value(!i_show_background_color);
	icon_show_label->value(i_show_label);
	icon_label_width->value(i_label_width);
	engage_with_one_click->value(one_click);

	//printf("%i %s\n", icon_font, Fl::get_font_name((Fl_Font)icon_font, 0));

	icon_font_txt->value(Fl::get_font_name((Fl_Font)icon_font, 0));
}

int main(int argc, char** argv) {
	int show_group = 1;
	if(argc > 1) {
		if(strcmp(argv[1], "--desktop") == 0)
			show_group = 1;
		else if(strcmp(argv[1], "--icons") == 0)
			show_group = 2;
		else if(strcmp(argv[1], "--icons-behaviour") == 0)
			show_group = 3;
	}

	EDE_APPLICATION("ede-desktop-conf");

	edelib::Window* win = new edelib::Window(550, 285, _("Desktop options"), edelib::WIN_INIT_IMAGES);
	win->begin();
		Fl_Tabs* tabs = new Fl_Tabs(10, 10, 530, 230);
		tabs->begin();

			Fl_Group* g1 = new Fl_Group(20, 30, 510, 200, _("Background"));
			if(show_group != 1)
				g1->hide();

			g1->begin();
				Fl_Box* b1 = new Fl_Box(85, 196, 100, 15);
				b1->box(FL_THIN_UP_BOX);

				Fl_Box* b2 = new Fl_Box(30, 43, 210, 158);
				b2->box(FL_THIN_UP_BOX);

				/* box size is intentionaly odd so preserve aspect ratio */
				wallpaper = new Fl_Box(43, 53, 184, 138);
				wallpaper->box(FL_DOWN_BOX);
				wallpaper->color(FL_BLACK);

				/* 
				 * Real image, since box will be resized as image size, but not
				 * larger than wallpaper box. wallpaper_img will be resized in set_wallpaper()
				 */
				wallpaper_img = new Fl_Box(wallpaper->x(), wallpaper->y(), 0, 0);
				wallpaper_img->box(FL_NO_BOX);
				wallpaper_img->align(FL_ALIGN_CLIP);

				/*
				Fl_Box* b4 = new Fl_Box(60, 206, 145, 14);
				b4->box(FL_THIN_UP_BOX);
				*/

				desk_background = new Fl_Input(295, 80, 190, 25, _("Image:"));

				browse = new Fl_Button(490, 80, 25, 25, "...");
				browse->callback(browse_cb);

				desk_background_mode = new Fl_Choice(295, 116, 220, 24, _("Mode:"));
				desk_background_mode->down_box(FL_BORDER_BOX);
				desk_background_mode->menu(mode_menu);
				desk_background_mode->callback(choice_cb);

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
			if(show_group != 2)
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

				icon_label_width = new Fl_Value_Slider(30, 155, 220, 25, _("Label width"));
				icon_label_width->type(5);
				icon_label_width->step(1);
				icon_label_width->maximum(200);
				icon_label_width->align(FL_ALIGN_TOP_LEFT);

				icon_font_txt = new Fl_Input(260, 155, 190, 25, _("Label font"));
				icon_font_txt->align(FL_ALIGN_TOP_LEFT);

				Fl_Button* icon_font_browse_button = new Fl_Button(455, 155, 25, 25, "...");
				icon_font_browse_button->callback(browse_fonts_cb, icon_font_txt);

			g2->end();

			Fl_Group* g3 = new Fl_Group(20, 30, 510, 195, _("Icons behaviour"));
			if(show_group != 3)
				g3->hide();

			g3->begin();
				engage_with_one_click = new Fl_Check_Button(30, 50, 220, 25, _("Engage with just one click"));
				engage_with_one_click->down_box(FL_DOWN_BOX);
			g3->end();
		tabs->end();

		/* Fl_Button* ok = new Fl_Button(260, 250, 90, 25, _("&OK"));
		ok->callback(ok_cb, win); */
		Fl_Button* ok = new Fl_Button(355, 250, 90, 25, _("&OK"));
		ok->callback(ok_cb, win);
		Fl_Button* cancel = new Fl_Button(450, 250, 90, 25, _("&Cancel"));
		cancel->callback(close_cb, win);
	win->end();
	load_settings();
	edelib::window_center_on_screen(win);
	win->show(argc, argv);
	return Fl::run();
}
