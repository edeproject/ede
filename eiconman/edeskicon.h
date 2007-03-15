/*
 * $Id$
 *
 * Desktop icons manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef edeskicon_h
#define edeskicon_h

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/*#include <efltk/Fl.h>
#include <efltk/Fl_Shaped_Window.h>
#include <efltk/Fl_Double_Window.h>
#include <efltk/Fl_Box.h>
#include <efltk/x.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Input.h>*/
#include <fltk/ask.h> //#include <efltk/fl_ask.h>
//#include <efltk/Fl_Button.h>
#include <fltk/PopupMenu.h> //#include <efltk/Fl_Menu_Button.h>
/*#include <efltk/Fl_Item.h>
#include <efltk/fl_draw.h>*/
#include <fltk/filename.h> //#include <efltk/filename.h>
//#include <efltk/Fl_File_Dialog.h>
#include "../edelib2/EDE_Config.h" //#include <efltk/Fl_Config.h>
#include <fltk/Image.h> //#include <efltk/Fl_Image.h>
#include "../edelib2/NLS.h" //#include <efltk/Fl_Locale.h>

//#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

class MovableIcon;

class Icon : public fltk::Widget
{
public:
    Icon(char *icon_config);
    ~Icon();

    virtual void draw();
    virtual void layout();
    virtual int handle(int e);

    EDE_Config *get_cfg() const { return cfg; }

    void update_icon();
    void update_all();

    fltk::Image *icon_im;

    int lwidth, lheight;

    void cb_execute_i();
    static inline void cb_execute(fltk::Item *, void *d) { ((Icon*)d)->cb_execute_i(); }

protected:
    EDE_Config *cfg;
    MovableIcon *micon;

    char* icon_file;
    char* icon_name;

    int x_position;
    int y_position;
};

class MovableIcon : public fltk::Window
{
    Icon *icon;
public:
    MovableIcon(Icon *i);
    ~MovableIcon();
};

///////////////////////////////////////

extern int label_background;
extern int label_foreground;
extern int label_fontsize;
extern int label_maxwidth;
extern int label_gridspacing;
extern bool one_click_exec;

extern void read_icons_configuration();
extern void save_icon(Icon *);
extern void delete_icon(fltk::Widget*, Icon *);
extern int create_new_icon();

extern void update_iconeditdialog(Icon *);
extern void update_property_dialog(Icon *);

extern char* get_localized_string();
extern char* get_localized_name(EDE_Config &iconConfig);

#endif 

