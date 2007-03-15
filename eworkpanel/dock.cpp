#include "dock.h"
//#include <efltk/Fl.h>
#include <fltk/Fl.h>

Dock::Dock()
    : fltk::Group(0,0,0,0)
{
    //box(FL_THIN_DOWN_BOX);
    color(FL_INVALID_COLOR); //Draw with parent color

    layout_align(FL_ALIGN_RIGHT);
    layout_spacing(1);

    end();
}

void Dock::add_to_tray(Fl_Widget *w)
{
    insert(*w, 0);

    w->layout_align(FL_ALIGN_LEFT);
    w->show();

    int new_width = this->w() + w->width() + layout_spacing();
    this->w(new_width);

    parent()->relayout();
    fltk::redraw();
}

void Dock::remove_from_tray(Fl_Widget *w)
{
    remove(w);

    int new_width = this->w() - w->width() - layout_spacing();
    this->w(new_width);

    parent()->relayout();
    fltk::redraw();
}
