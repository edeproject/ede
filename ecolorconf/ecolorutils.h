// Colors settings for EDE
// Copyright (C) 2000-2002 Martin Pekar
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef ecolorutils_h
#define ecolorutils_h

/*#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>
#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Item_Group.h>*/
#include <fltk/Item.h> //#include <efltk/Fl_Item.h>
#include <fltk/filename.h> //#include <efltk/filename.h>
/*#include <efltk/Fl_Value_Output.h>
#include <efltk/Fl_Pack.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Radio_Button.h>
#include <efltk/Fl_Color_Chooser.h>
#include <efltk/Fl_Menu_Bar.h>*/
#include <fltk/Button.h> //#include <efltk/Fl_Button.h>
//#include <efltk/Fl_Input.h>
//#include <efltk/Fl_Output.h>
#include <fltk/ask.h> //#include <efltk/fl_ask.h>
//#include <efltk/Fl_Value_Input.h>
#include <fltk/Font.h> //#include <efltk/Fl_Font.h>
//#include <efltk/Fl_Input_Browser.h>
#include "../edelib2/Config.h" //#include <efltk/Fl_Config.h>
#include "../edelib2/NLS.h" //#include <efltk/Fl_Locale.h>

// this struct can hold slightly more information than Fl_Font
extern typedef struct {
	fltk::Font* font;
	char* encoding;
	int size;
	bool defined;
} EDEFont;



extern void updateFontAll();
extern void getSchemeColors();
extern void saveActiveScheme();
extern void saveSchemeAs();
extern void applyColors();
extern void fillItems();
extern void loadEfltkConfig();
extern void saveEfltkConfig();

// font stuff
const char* font_nice_name(EDEFont);
extern void labelfont_cb();
extern void textfont_cb();

#endif
