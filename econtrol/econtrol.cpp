/*
 * $Id$
 *
 * The EDE control center
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */
#include <stdlib.h>
#include <fltk/run.h>

#include "econtrol.h"

#include "../edelib2/Run.h"
#include "../edelib2/about_dialog.h"
#include "../edelib2/NLS.h"
#include "../edelib2/Icon.h"

#include "../edeconf.h"

using namespace fltk;
using namespace edelib;



// Widgets

Window *configPanelWindow=(Window *)0;
MenuBar *main_menubar=(MenuBar *)0;
StatusBarGroup *status_bar=(StatusBarGroup *)0;
ScrollGroup *scroll=(ScrollGroup *)0;
Group *main_view=(Group *)0;
InvisibleBox *help_window=(InvisibleBox *)0;


// Callbacks

static void cb_Quit(Item*, void*)
{
	exit(0);
}

static void cb_About(Item*, void*)
{
	about_dialog("Control panel", "1.99");
}


static void cb_Icons(Button*, void*)
{
	help_window->label(_("Change behaviour of your desktop icons."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("eiconsconf &");
}


static void cb_Colors(Button*, void*)
{
	help_window->label(_("Configure global colors and fonts used by EDE applications."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("ecolorconf &");
}

static void cb_Screen(Button*, void*)
{
	help_window->label(_("Configure the screensaver."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("esvrconf &");
}

static void cb_Window(Button*, void*)
{
	help_window->label(_("Setup window decorations and other options."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("ewmconf &");
}

static void cb_Display(Button*, void*) 
{
	help_window->label(_("Set various options for the X windowing system."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("edisplayconf &");
}

static void cb_Install(Button*, void*)
{
	help_window->label(_("This utility helps you to install new software."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("einstaller",false);
}

static void cb_Time(Button*, void*)
{
	help_window->label(_("Show and set computer time and date."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("etimedate",false);
}

static void cb_Panel(Button*, void*) 
{
	help_window->label(_("Change panel behaviour and choose applets."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("epanelconf",false);
}

static void cb_Keyboard(Button*, void*) 
{
	help_window->label(_("Configure keyboard shortcuts and other options."));
	help_window->redraw();
	//if (Fl::event_clicks())
	run_program("ekeyconf",false);
}

static void cb_Color(Item* i, void*) 
{
	const char* label = i->label();
	if (strcmp(label,"White")==0) { scroll->color(WHITE); }
	if (strcmp(label,"Gray")==0) { scroll->color(GRAY75); }
	if (strcmp(label,"Black")==0) { scroll->color(BLACK); }
	if (strcmp(label,"Red")==0) { scroll->color(RED); }
	if (strcmp(label,"Blue")==0) { scroll->color(BLUE); }
	scroll->redraw();
}


// Main program

int main (int argc, char **argv) {

//fl_init_locale_support("econtrol", PREFIX"/share/locale");
{Window* configPanelWindow = new Window(450, 310, _("Control panel"));
configPanelWindow->begin();
{main_menubar = new MenuBar(0, 0, 450, 26);
	main_menubar->begin();
	{ItemGroup* o = new ItemGroup(_("&File"));
		o->begin();
		{Item* o = new Item(_("&Quit"));
			o->shortcut(0x40071);
			o->callback((Callback*)cb_Quit);
			//o->x_offset(18);
		}
		o->end();
	}
	{ItemGroup* o = new ItemGroup(_("&Color"));
		o->align(ALIGN_CENTER);
		o->begin();
		{Item* o = new Item("White");
			o->callback((Callback*)cb_Color);
			o->type(Item::RADIO);
			o->set();
		}
		{Item* o = new Item("Gray");
			o->callback((Callback*)cb_Color);
			o->type(Item::RADIO);
		}
		{Item* o = new Item("Black");
			o->callback((Callback*)cb_Color);
			o->type(Item::RADIO);
		}
		{Item* o = new Item("Red");
			o->callback((Callback*)cb_Color);
			o->type(Item::RADIO);
		}
		{Item* o = new Item("Blue");
			o->callback((Callback*)cb_Color);
			o->type(Item::RADIO);
		}
		o->end();
	}
	{ItemGroup* o = new ItemGroup(_("&Help"));
		o->align(ALIGN_CENTER);
		o->begin();
		{Item* o = new Item(_("&About"));
			o->shortcut(0x40061);
			o->callback((Callback*)cb_About);
			//o->x_offset(18);
		}
		o->end();
	}
	main_menubar->end();
}
{main_view = new Group(0, 26, 450, 280);
	main_view->begin();
	{InvisibleBox* o = new InvisibleBox(0, 0, 450, 41, _("Control panel"));
		o->box(FLAT_BOX);
		o->labeltype(SHADOW_LABEL);
		o->color((Color)0xd089d00);
		o->buttoncolor((Color)0xe6e7e600);
		o->labelcolor((Color)0xffffff00);
		o->highlight_color((Color)0xe6e7e600);
		o->labelsize(20);
		o->textsize(29);
		o->align(ALIGN_INSIDE);
	}
	{TiledGroup* o = new TiledGroup(0, 40, 450, 215);
		o->box(FLAT_BOX);
		o->color((Color)0xd93b4300);
		o->begin();
		{help_window = new InvisibleBox(0, 0, 110, 195, _("Welcome to the control panel. Here you can setup most things on your computer."));
			o->set_vertical();
			o->box(DOWN_BOX);
			o->color((Color)0xfff9e400);
			o->labelcolor((Color)32);
			o->align(ALIGN_WRAP);
			//o->set_value();
		}
		{scroll = new ScrollGroup(110, 0, 340, 195);
			o->box(DOWN_BOX);
			o->color(WHITE);
			o->align(ALIGN_CENTER);
			o->begin();
			{Button* o = new Button(10, 15, 60, 75, _("Icons"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/behaviour.xpm"));
			//            o->image(EDE_Icon::get("desktop-mdk",EDE_Icon::SMALL));
			//            o->image(EDE_Icon::get("kappfinder",EDE_Icon::SMALL));
				o->image(Icon::get("desktop",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->buttonbox(NO_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Icons);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Icons settings"));
			}
			{Button* o = new Button(75, 15, 60, 75, _("Colors and Fonts"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/themes.xpm"));
				o->image(Icon::get("fonts",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Colors);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Colors settings"));
			}
			{Button* o = new Button(140, 15, 60, 75, _("Screen Saver"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/screensaver.xpm"));
				o->image(Icon::get("terminal",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Screen);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Screensaver configuration"));
			}
			{Button* o = new Button(10, 95, 60, 75, _("Window Manager"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/windowmanager.xpm"));
			//            o->image(EDE_Icon::get("kcmkwm",EDE_Icon::SMALL));
			//            o->image(EDE_Icon::get("kpersonalizer",EDE_Icon::SMALL));
				o->image(Icon::get("window_list",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Window);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Window manager settings"));
			}
			{Button* o = new Button(75, 95, 60, 75, _("Display"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/display.xpm"));
			//            o->image(EDE_Icon::get("kcmx",EDE_Icon::SMALL));
			//            o->image(EDE_Icon::get("randr",EDE_Icon::SMALL));
				o->image(Icon::get("looknfeel",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Display);
				o->tooltip(_("Display settings"));
			}
			{Button* o = new Button(140, 95, 60, 75, _("Install New Software"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/newsoft.xpm"));
			//            o->image(EDE_Icon::get("ark",EDE_Icon::SMALL));
				o->image(Icon::get("package",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Install);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Software installation."));
			}
			{Button* o = new Button(205, 95, 60, 75, _("Time and Date"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/timedate.xpm"));
				o->image(Icon::get("date",Icon::SMALL));
			//            o->image(EDE_Icon::get("karm",EDE_Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Time);
				o->align(ALIGN_WRAP);
				o->tooltip(_("Time and date settings."));
			}
			{Button* o = new Button(205, 15, 60, 75, _("Panel"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/panel.xpm"));
				o->image(Icon::get("kcmkicker",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Panel);
				o->tooltip(_("Panel configuration."));
			}
			{Button* o = new Button(265, 15, 60, 75, _("Keyboard"));
				o->set_vertical();
			//            o->image(SharedImage::get("icons/keyboard.xpm"));
			//            o->image(EDE_Icon::get("kxkb",EDE_Icon::SMALL));
				o->image(Icon::get("keyboard",Icon::SMALL));
				o->box(HIGHLIGHT_DOWN_BOX);
				o->buttonbox(NO_BOX);
				o->color((Color)7);
				o->highlight_color((Color)7);
				o->highlight_textcolor((Color)32);
				o->labelsize(10);
				o->callback((Callback*)cb_Keyboard);
				o->tooltip(_("Keyboard settings"));
			}
			scroll->end();
			//o->edge_offset(10);
		}
		o->end();
		o->parent()->resizable(o);
		Group::current()->resizable(o);
	}
	main_view->end();
	Group::current()->resizable(main_view);
}
{status_bar = new StatusBarGroup(0, 281, 450, 24, _("Ready"));
	status_bar->box(DOWN_BOX);
}

configPanelWindow->end();
configPanelWindow->size_range(configPanelWindow->w(), configPanelWindow->h());
}

	//  configPanelWindow->menu(main_menubar);
	//  configPanelWindow->view(main_view);
	//  configPanelWindow->status(status_bar);
	configPanelWindow->show(argc, argv);
	return  run();
}
