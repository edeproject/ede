/*
 * $Id$
 *
 * Volume control application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "prefs.h"
#include <malloc.h>
#include <fltk/Item.h>//#include <efltk/Fl_Item.h>
#include <fltk/filename.h>//#include <efltk/filename.h>

#include "../edelib2/NLS.h"//#include <efltk/Fl_Locale.h>
#include "../edelib2/Config.h"//#include <efltk/Fl_Config.h>


using namespace fltk;
using namespace edelib;



extern char device[1024];
extern int mixer_device;

void choice_items(char *path) {
  Item         *new_Item;
      dirent          **files;
      int             num_Files = 0;
  
      num_Files = filename_list(path, &files);
  
      if (num_Files > 0) {
  
          for (int i = 0; i < num_Files; i ++) {
              if (strcmp(files[i]->d_name, ".") != 0 &&
                  strcmp(files[i]->d_name, "..") != 0) {
  
                  char filename[PATH_MAX];
                  snprintf(filename, sizeof(filename)-1, "%s/%s", path, files[i]->d_name);
		  
		  struct stat s;
		  if (!stat(filename, &s)==0) break;
		    
                  if (!S_ISDIR(s.st_mode) && strncmp(files[i]->d_name, "mixer", 5)==0) {
                      new_Item = new Item();
  					new_Item->copy_label(filename);
                  }
              }
              free(files[i]);
          }
          free(files);
      }
}

Window* preferencesWindow;

InputBrowser* deviceNameInput;

static void cb_OK(Button*, void*) {
  Config globalConfig("EDE Team", "evolume");
  globalConfig.set("Sound mixer", "Device", deviceNameInput->value());
  snprintf(device, sizeof(device)-1, "%s", (char*)deviceNameInput->value());
  
  mixer_device = open(device, O_RDWR);  
  update_info();
  
  preferencesWindow->hide();
}

static void cb_Cancel(Button*, void*) {
  preferencesWindow->hide();
}

void PreferencesDialog(Widget *, void *) {
	Window* w;
	{Window* o = preferencesWindow = new Window(265, 290, _("Preferences"));
		w = o;
		preferencesWindow->begin();
		{TabGroup* o = new TabGroup(10, 10, 245, 240);
			o->begin();
			{Group* o = new Group(0, 25, 255, 215, _("Sound device"));
				o->align(ALIGN_TOP | ALIGN_LEFT);
				o->begin();
				{InputBrowser* o = deviceNameInput = new InputBrowser(10, 30, 155, 25, _("Device name:")); 
					o->begin();
					o->align(ALIGN_TOP | ALIGN_LEFT);

					o->text(device);
					choice_items("/dev");
					choice_items("/dev/sound");
					o->end();
				}
				o->end();
			}
			o->end();
			o->selection_color(o->color());
			o->selection_textcolor(o->textcolor());
		}
		{Button* o = new Button(65, 255, 90, 25, _("&OK"));
			o->callback((Callback*)cb_OK);
		}
		{Button* o = new Button(165, 255, 90, 25, _("&Cancel"));
			o->callback((Callback*)cb_Cancel);
		}
		o->end();
	}
	preferencesWindow->end();
	preferencesWindow->set_modal();
	preferencesWindow->show();
}
