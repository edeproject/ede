#ifndef _TITLEBAR_H_
#define _TITLEBAR_H_

class Frame;
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Multi_Image.h>
#include <efltk/Fl_Double_Window.h>
#include <efltk/Fl_Group.h>
#include <efltk/x.h>

#include <efltk/Fl_Image.h>

class Titlebar_Button : public Fl_Button
{
    int m_type;
public:
    Titlebar_Button(int type);
    void draw();
};

class Titlebar : public Fl_Window {
    friend class Frame;
public:
    Titlebar(int x,int y,int w,int h,const char *l=0);
    virtual ~Titlebar();

    void setting_changed();

    void show();
    void hide();

    virtual void draw();
    virtual int handle(int event);
    virtual void layout();

    void draw_opaque(int, int, int, int);

    Fl_Button *close() { return &_close; }
    Fl_Button *min()   { return &_min; }
    Fl_Button *max()   { return &_max; }

    static void popup_menu(Frame *frame);
    static void cb_change_desktop(Fl_Widget *w, void *data);

    static int box_type;
    static int label_align;
    static int default_height;

    Frame *f;
protected:
    Fl_Image *title_icon;

    Titlebar_Button _close, _max, _min;
    int text_w;
};


#endif
