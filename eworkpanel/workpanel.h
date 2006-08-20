// Equinox Desktop Environment - main panel
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

#ifndef workpanel_h
#define workpanel_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

/*#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>
#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/Fl_ProgressBar.h>
#include <efltk/Fl_Item.h>
#include <efltk/filename.h>
#include <efltk/Fl_Dial.h>
#include <efltk/Fl_Pack.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Radio_Button.h>
#include <efltk/Fl_Color_Chooser.h>
#include <efltk/Fl_Menu_Bar.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Input.h>
#include <efltk/Fl_Output.h>
#include <efltk/fl_ask.h>
#include <efltk/Fl_Tabs.h>
#include <efltk/Fl_Scroll.h>
#include <efltk/Fl_Shaped_Window.h>
#include <efltk/Fl_Overlay_Window.h>
#include <efltk/Fl_FileBrowser.h>
#include <efltk/Fl_Font.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Util.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Images.h>
#include <efltk/Fl_Locale.h>*/

#include <fltk/Fl.h>
#include <fltk/Window.h>
#include <fltk/x.h>
#include <fltk/Menu_Button.h>
#include <fltk/Item_Group.h>
#include <fltk/ProgressBar.h>
#include <fltk/Item.h>
#include <fltk/filename.h>
#include <fltk/Dial.h>
#include <fltk/Pack.h>
#include <fltk/Box.h>
#include <fltk/Divider.h>
#include <fltk/Image.h>
#include <fltk/Button.h>
#include <fltk/Radio_Button.h>
#include <fltk/Color_Chooser.h>
#include <fltk/Menu_Bar.h>
#include <fltk/Button.h>
#include <fltk/Input.h>
#include <fltk/Output.h>
#include <fltk/ask.h>
#include <fltk/Tabs.h>
#include <fltk/Scroll.h>
#include <fltk/Shaped_Window.h>
#include <fltk/Overlay_Window.h>
#include <fltk/FileBrowser.h>
#include <fltk/Font.h>
#include "EDE_Config.h"
#include <fltk/Util.h>
#include <fltk/Image.h>
#include <fltk/Images.h>
#include "NLS.h"

#include "icons/sound.xpm"
#include "icons/desktop.xpm"
#include "icons/run.xpm"
#include "icons/showdesktop.xpm"

#endif
