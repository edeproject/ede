#ifndef _ICON_H_
#define _ICON_H_

#include "Frame.h"

#include <efltk/Fl_Image.h>
#include <efltk/Fl_Renderer.h>
#include <efltk/Fl_Map.h>

typedef Fl_String_Ptr_Map ImageMap;

class Icon
{
public:
    Icon(XWMHints *wm_hints);
    ~Icon();

    Fl_Image *get_icon(int W, int H);

private:
    ImageMap images;
    Fl_Image *image, *mask;
};

#endif

