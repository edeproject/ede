#include "Theme.h"
#include <efltk/Fl.h>

namespace Theme {

Fl_Image *images[IMAGES_LAST];
Fl_Color _frame_color;
Fl_Image_List loaded;
bool use;

bool use_theme() { return use; }
void use_theme(bool val) { use = val; }

static Fl_Image *load_image(const char *name, const Fl_String &path, Fl_Config &cfg)
{
    Fl_String filename;
    cfg.read(name, filename, 0);
    Fl_String img_path(path + filename);
    return Fl_Image::read(img_path);
}

bool load_theme(const Fl_String &file)
{
    if(!fl_file_exists(file))
        return false;

    int pos = file.rpos('/');
    if(pos==-1) return false;

    unload_theme();

    Fl_String path(file.sub_str(0, pos+1));
    Fl_String filename;

    Fl_Config themeconf(file);
    themeconf.set_section("Theme");

    themeconf.read("Frame color", _frame_color, FL_NO_COLOR);

    images[TITLEBAR_BG] = load_image("Title image", path, themeconf);
    if(images[TITLEBAR_BG])
        loaded.append(images[TITLEBAR_BG]);

    Fl_Image *up   = load_image("Close image up", path, themeconf);
    Fl_Image *down = load_image("Close image down", path, themeconf);
    if(up && down) {
        images[TITLEBAR_CLOSE_UP] = up;
        images[TITLEBAR_CLOSE_DOWN] = down;
        up->state_effect(false);
        down->state_effect(false);
        loaded.append(up);
        loaded.append(down);
    } else {
        if(up)   delete up;
        if(down) delete down;
    }

    up   = load_image("Maximize image up", path, themeconf);
    down = load_image("Maximize image down", path, themeconf);
    if(up && down) {
        images[TITLEBAR_MAX_UP] = up;
        images[TITLEBAR_MAX_DOWN] = down;
        up->state_effect(false);
        down->state_effect(false);
        loaded.append(up);
        loaded.append(down);
    } else {
        if(up)   delete up;
        if(down) delete down;
    }

    up   = load_image("Minimize image up", path, themeconf);
    down = load_image("Minimize image down", path, themeconf);
    if(up && down) {
        images[TITLEBAR_MIN_UP] = up;
        images[TITLEBAR_MIN_DOWN] = down;
        up->state_effect(false);
        down->state_effect(false);
        loaded.append(up);
        loaded.append(down);
    } else {
        if(up)   delete up;
        if(down) delete down;
    }

    return true;
}

void unload_theme()
{
    for(unsigned n=0; n<loaded.size(); n++) {
        delete loaded[n];
    }
    loaded.clear();

    for(int n=0; n<IMAGES_LAST; n++)
        images[n] = 0;

    _frame_color = FL_NO_COLOR;
}

Fl_Image *image(int which)
{
    if(which<0 || which>=IMAGES_LAST) {
        Fl::warning("Invalid theme image index: %d", which);
        return 0;
    }
    return images[which];
}

Fl_Color frame_color()
{
    return _frame_color;
}

}; /* namespace Theme */

