#ifndef _THEME_H_
#define _THEME_H_

#include <efltk/Fl_Image.h>
#include <efltk/Fl_Image_List.h>
#include <efltk/Fl_Multi_Image.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_String.h>

enum {
    TITLEBAR_BG = 0,
    TITLEBAR_CLOSE_UP, TITLEBAR_CLOSE_DOWN,
    TITLEBAR_MAX_UP,   TITLEBAR_MAX_DOWN,
    TITLEBAR_MIN_UP,   TITLEBAR_MIN_DOWN,
    IMAGES_LAST
};

namespace Theme {

extern bool use_theme();
extern void use_theme(bool val);

extern bool load_theme(const Fl_String &file);
extern void unload_theme();

extern Fl_Image *image(int which);
extern Fl_Color frame_color();

}

#endif
