/*
 * $Id$
 *
 * Eiconman, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __DESKTOPCONFIG_H__
#define __DESKTOPCONFIG_H__

#include <fltk/Window.h>
#include <fltk/Button.h>
#include <fltk/Input.h>
#include <fltk/InvisibleBox.h>
#include <fltk/Choice.h>
#include <fltk/CheckButton.h>
#include <edelib/String.h>

class PreviewBox : public fltk::InvisibleBox {
	public:
		PreviewBox(int x, int y, int w, int h, const char* l = 0);
		~PreviewBox();
		virtual void draw(void);
};

class DesktopConfig : public fltk::Window {
	private:
		fltk::Input*        img_path;
		fltk::Button*       img_browse;
		fltk::Choice*       img_choice;
		fltk::CheckButton*  use_wp;
		PreviewBox*         preview;
		fltk::Button*       color_box;

		edelib::String      wp_path;

	public:
		DesktopConfig();
		~DesktopConfig();

		void run(void);
		void disable_wp(void);
		void set_preview_color(unsigned int c);
		void set_preview_image(const char* path);
		void set_color(unsigned int c);
		unsigned int bkg_color(void) { return color_box->color(); }
};

#endif
