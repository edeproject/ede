#ifndef _DOCK_H_
#define _DOCK_H_

//#include <efltk/Fl_Group.h>
#include <fltk/Group.h>

class Dock : public fltk::Group
{
public:
    Dock();

    void add_to_tray(fltk::Widget *w);
    void remove_from_tray(fltk::Widget *w);
};

#endif
