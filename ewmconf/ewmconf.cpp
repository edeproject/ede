/*
 * $Id$
 *
 * edewm (EDE Window Manager) settings
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "ewmconf.h"
#include "ewm.h"

#include <fltk/file_chooser.h>
#include <fltk/run.h>
#include <fltk/ask.h>

#include "../edelib2/NLS.h"

fltk::Button* titlebarLabelColorButton;
fltk::Button* titlebarColorButton;
fltk::Button* titlebarActiveLabelColorButton;
fltk::Button* titlebarActiveColorButton;
fltk::Choice* titlebarDrawGrad;
fltk::CheckButton* useThemeButton;
fltk::Input* themePathInput;
fltk::Button* browse_btn;
fltk::CheckButton* animateButton;
fltk::ValueSlider* animateSlider;
fltk::CheckButton* opaqueResize;

fltk::Button* applyButton;

bool conf_changed = false;


// Functions called by callbacks

void changeBoxColor(fltk::Button *box) {
	fltk::Button *colorBox = box;
	fltk::Color oldColor = colorBox->color();
	fltk::Color defColor = oldColor;
	fltk::color_chooser("Choose color", defColor);
	if ( defColor != oldColor ) {
		colorBox->color(defColor);
		colorBox->redraw();
	}
}

void confChanged() {
	if (conf_changed) return;
	conf_changed=true;
	applyButton->activate();
}


// Callback functions

static void cb_Text(fltk::Choice* o, void*) {
	title_align = (int)o->value();
	confChanged();
}

static void cb_Height(fltk::ValueInput* o, void*) {
	title_height = (int)o->value();
	confChanged();
}

static void cb_titlebarLabelColorButton(fltk::Button*, void*) {
	changeBoxColor(titlebarLabelColorButton);
	title_normal_color_text = (int) titlebarLabelColorButton->color();
	confChanged();
}

static void cb_titlebarColorButton(fltk::Button*, void*) {
	changeBoxColor(titlebarColorButton);
	title_normal_color = (int) titlebarColorButton->color();
	confChanged();
}

static void cb_titlebarActiveLabelColorButton(fltk::Button*, void*) {
	changeBoxColor(titlebarActiveLabelColorButton);
	title_active_color_text = (int) titlebarActiveLabelColorButton->color();
	confChanged();
}

static void cb_titlebarActiveColorButton(fltk::Button*, void*) {
	changeBoxColor(titlebarActiveColorButton);
	title_active_color  = (int) titlebarActiveColorButton->color();
	confChanged();
}

static void cb_titlebarDrawGrad(fltk::Choice*, void*) {
	title_draw_grad = titlebarDrawGrad->value();
	confChanged();
}

static void cb_useThemeButton(fltk::CheckButton*, void*) {
	if (useThemeButton->value()) {
		themePathInput->activate();
		browse_btn->activate();
		titlebarDrawGrad->deactivate();
		titlebarColorButton->deactivate();
		titlebarActiveColorButton->deactivate();
	} else {
		themePathInput->deactivate();
		browse_btn->deactivate();
		titlebarDrawGrad->activate();
		titlebarColorButton->activate();
		titlebarActiveColorButton->activate();
	}
	
	use_theme = useThemeButton->value();
	confChanged();
}

static void cb_themePathInput(fltk::Input*, void*) {
	theme_path = (char*)realloc(theme_path, strlen(themePathInput->value()));
	strcpy(theme_path, themePathInput->value());
	confChanged();
}

static void cb_browse_btn(fltk::Button*, void*) {
	char *file_types = _("Themes (*.theme), *.theme, All files (*.*), *");
	const char *fileName = fltk::file_chooser( "Themes...", file_types, themePathInput->value());    
	if (fileName) { 
		themePathInput->value(fileName);
		strncpy(theme_path, fileName, strlen(fileName));
	}
	confChanged();
}

static void cb_animateButton(fltk::CheckButton*, void*) {
	if (animateButton->value()) 
		animateSlider->activate();
	else 
		animateSlider->deactivate();
	animate = animateButton->value();
	confChanged();
}

static void cb_animateSlider(fltk::ValueSlider*, void*) {
	animate_speed = (int)animateSlider->value();
	confChanged();
}

static void cb_opaqueResize(fltk::CheckButton*, void*) {
	opaque_resize = opaqueResize->value();
	confChanged();
}

/*static void cb_OK(fltk::Button*, void*) {
	writeConfiguration();
	applyConfiguration();
	exit(0);
}*/

static void cb_Apply(fltk::Button*, void*) {
	writeConfiguration();
	applyConfiguration();
	conf_changed = false;
	applyButton->deactivate();
}

static void cb_Close(fltk::Button*, void*) {
	if (conf_changed) {
		int answer = fltk::choice_alert(_("You have unsaved changes in this window!\nDo you want to close it anyway?"), 0, _("Go &Back"), _("&Discard Changes"));
		if (answer == 1) return;
	}
	exit(0);
}


// Main window design

int main (int argc, char **argv) {

  fltk::Window* w;
  //fl_init_locale_support("ewmconf", PREFIX"/share/locale");
  readConfiguration();
   {fltk::Window* o = new fltk::Window(325, 385, _("Window manager settings"));
    w = o;
    o->set_vertical();
    o->begin();
     {fltk::TabGroup* o = new fltk::TabGroup(10, 10, 305, 330);
      o->selection_color(o->color());
      o->selection_textcolor(o->textcolor());
      o->box(fltk::THIN_UP_BOX);
      o->begin();
       {fltk::Group* o = new fltk::Group(0, 25, 305, 305, _("&Titlebar"));
        o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
        o->begin();
         {fltk::Choice* o = new fltk::Choice(35, 30, 125, 25, _("Text align:"));
          o->callback((fltk::Callback*)cb_Text);
          o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
          o->begin();
          new fltk::Item(_("Left"));
          new fltk::Item(_("Right"));
          new fltk::Item(_("Center"));
          o->end();
          o->value(title_align);
          o->tooltip(_("Where will window title be placed on the title bar?"));
        }
         {fltk::ValueInput* o = new fltk::ValueInput(205, 30, 60, 25, _("Height:"));
          o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
          o->minimum(10);
          o->maximum(50);
          o->step(1);
          o->value(20);
          o->callback((fltk::Callback*)cb_Height);
          o->value(title_height);
          o->tooltip(_("Height of titlebar (in pixels)"));
        }
         {fltk::Button* o = titlebarLabelColorButton = new fltk::Button(205, 75, 60, 20, _("Label color: "));
          o->callback((fltk::Callback*)cb_titlebarLabelColorButton);
          o->align(fltk::ALIGN_LEFT);
          o->color((fltk::Color)title_normal_color_text);
          o->tooltip(_("Text color of window title"));
        }
         {fltk::Button* o = titlebarActiveLabelColorButton = new fltk::Button(205, 105, 60, 20, _("Active label color: "));
          o->callback((fltk::Callback*)cb_titlebarActiveLabelColorButton);
          o->align(fltk::ALIGN_LEFT);
          o->color((fltk::Color) title_active_color_text);
          o->tooltip(_("Title text color of active (foremost) window"));
        }
         {fltk::Button* o = titlebarColorButton = new fltk::Button(205, 135, 60, 20, _("Titlebar color: "));
          o->callback((fltk::Callback*)cb_titlebarColorButton);
          o->align(fltk::ALIGN_LEFT);
          o->color((fltk::Color) title_normal_color);
          o->tooltip(_("Color of title bar"));
        }
         {fltk::Button* o = titlebarActiveColorButton = new fltk::Button(205, 165, 60, 20, _("Active titlebar color: "));
          o->callback((fltk::Callback*)cb_titlebarActiveColorButton);
          o->align(fltk::ALIGN_LEFT);
          o->color((fltk::Color)title_active_color);
          o->tooltip(_("Titlebar color of active (foremost) window"));
        }
         {fltk::Choice* o = titlebarDrawGrad = new fltk::Choice(120, 200, 145, 25, _("Effect type:"));
          o->callback((fltk::Callback*)cb_titlebarDrawGrad);
          o->align(fltk::ALIGN_LEFT|fltk::ALIGN_WRAP);
          o->begin();
          new fltk::Item(_("Flat"));
          new fltk::Item(_("Horizontal shade"));
          new fltk::Item(_("Thin down"));
          new fltk::Item(_("Up box"));
          new fltk::Item(_("Down box"));
          new fltk::Item(_("Plastic"));
          o->end();
          o->value(title_draw_grad);
          o->tooltip(_("Effect that will be used when drawing titlebar"));
        }
         {fltk::Divider* o = new fltk::Divider();
          o->resize(10,235,285,2);
         {fltk::CheckButton* o = useThemeButton = new fltk::CheckButton(10, 245, 300, 25, _("&Use theme"));
          o->callback((fltk::Callback*)cb_useThemeButton);
          o->value(use_theme);
          o->tooltip(_("Choose titlebar theme below (some options will be disabled)"));
        }
         {fltk::Input* o = themePathInput = new fltk::Input(65, 270, 195, 25, _("Path:"));
          o->callback((fltk::Callback*)cb_themePathInput);
          o->deactivate();
          themePathInput->value(theme_path);
          o->tooltip(_("Enter filename for file where theme is stored"));
        }
         {fltk::Button* o = browse_btn = new fltk::Button(270, 270, 25, 25, "...");
          o->callback((fltk::Callback*)cb_browse_btn);
          o->deactivate();
          o->tooltip(_("Click here to choose theme"));
        }
//         {fltk::Divider* o = new fltk::Divider();
//          o->resize(10,100,300,2);
//        }
        }
        o->end();
      }
       {fltk::Group* o = new fltk::Group(0, 25, 305, 305, "&Resizing");
        o->align(fltk::ALIGN_TOP|fltk::ALIGN_LEFT);
	o->hide();
        o->begin();
         {fltk::CheckButton* o = animateButton = new fltk::CheckButton(10, 15, 295, 25, _("Animate size changes"));
          o->set();
          o->callback((fltk::Callback*)cb_animateButton);
          o->value(animate);
          o->tooltip(_("If you enable this option, maximize/minimize operations will be animated"));
        }
         {fltk::ValueSlider* o = animateSlider = new fltk::ValueSlider(70, 40, 225, 25, _("Speed:"));
          o->type(fltk::ValueSlider::TICK_ABOVE);
          o->box(fltk::DOWN_BOX);
          o->textsize(10);
          o->minimum(5);
          o->maximum(20);
          o->step(1);
          o->value(14);
          o->slider_size(8);
          o->callback((fltk::Callback*)cb_animateSlider);
          o->align(fltk::ALIGN_LEFT);
          o->value(animate_speed);
          if(animate) o->activate(); else o->deactivate();
          o->tooltip(_("Set speed for animation when maximizing / minimizing windows"));
        }
         {fltk::Divider* o = new fltk::Divider();
          o->resize(10,75,285,2);
        }
         {fltk::CheckButton* o = opaqueResize = new fltk::CheckButton(10, 85, 285, 25, "Show window content while resizing");
          o->callback((fltk::Callback*)cb_opaqueResize);
          o->value(opaque_resize);
          o->tooltip(_("Enable if you want contents of windows to be redrawn as you resize window"));
        }
        o->end();
      }
      o->end();
    }
//     {fltk::Button* o = new fltk::Button(67, 337, 80, 25, "&OK");
//      o->shortcut(0xff0d);
//      o->callback((fltk::Callback*)cb_OK);
//    }
     {fltk::Button* o = applyButton = new fltk::Button(125, 350, 90, 25, _("&Apply"));
      o->callback((fltk::Callback*)cb_Apply);
      o->tooltip(_("Apply changes"));
    }
     {fltk::Button* o = new fltk::Button(225, 350, 90, 25, _("&Close"));
      o->shortcut(0xff1b);
      o->callback((fltk::Callback*)cb_Close);
      o->tooltip(_("Close this window"));
    }
    o->end();
  }
  
  // Make sure that "Use theme" is active and standalone buttons inactive
  // if theme is set - and vice versa
  if (!theme_path || strlen(theme_path) < 2)  // possibly just 1 space
  	useThemeButton->value(false);
  else
  	useThemeButton->value(true);
  cb_useThemeButton(useThemeButton, 0);
  
  // above will activate Apply button, so we need to change it back
  applyButton->deactivate();
  conf_changed=false;
  
  //useThemeButton->do_callback(FL_DIALOG_BTN);
  w->show(argc, argv);
  return  fltk::run();
}

