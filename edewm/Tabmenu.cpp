//nothing yet!

#include <efltk/Fl_Box.h>
#include <efltk/Fl_Pack.h>

#include "Icon.h"
#include "Frame.h"
#include "Desktop.h"
#include "debug.h"
#include "Windowmanager.h"

class TabWin : public Fl_Window
{
public:
    TabWin(Frame_List &list, int dir) : Fl_Window(Fl::w()/2-100, Fl::h()/2-40, 200, 96, "tabwin")
    {
        direction = dir;
        selected=-1;
        box(FL_UP_BOX);

        label = new Fl_Box(10,50,180,20);
        label->box(FL_THIN_DOWN_BOX);
        label->align(FL_ALIGN_CLIP|FL_ALIGN_INSIDE|FL_ALIGN_LEFT);

        p = new Fl_Pack(0,0,200,50);
        p->type(Fl_Pack::HORIZONTAL);
        p->layout_spacing(3);
        p->begin();

        for(uint n=0; n<list.size(); n++) {
            Frame *f = list[n];
            Fl_Box *b = new Fl_Box(0,0,50,50,f->label());
            b->color(FL_GRAY);
            b->layout_align(FL_ALIGN_LEFT);
            b->user_data(f);
            b->label_type(FL_NO_LABEL);
            b->box(FL_BORDER_BOX);
            if(f->icon()) {
                b->image(f->icon()->get_icon(48,48));
            }
            if(f->active())
            {
                selected = n;
                b->set_selected();
                b->color(FL_RED);
                b->box(FL_THIN_DOWN_BOX);
                label->label(f->label());
            }
        }
        p->end();

        layout();
        create();
    }

    void next() {
        if(selected==-1) selected=0;
        if(p->child(selected)->selected())
            p->child(selected)->clear_selected();

        p->child(selected)->color(FL_GRAY);
        p->child(selected)->box(FL_BORDER_BOX);
        selected += direction;
        if(direction==1 && selected>=p->children()) {
            selected = 0;
        }
        else if(direction==-1 && selected<0) {
            selected = p->children()-1;
        }

        p->child(selected)->set_selected();
        p->child(selected)->color(p->selection_color());
        p->child(selected)->box(FL_THIN_DOWN_BOX);

        label->label(p->child(selected)->label());

        redraw(FL_DAMAGE_ALL);
    }
    Frame *selected_frame() {
        if(selected==-1) return 0;
        return (Frame *)p->child(selected)->user_data();
    }

    int handle(int ev);
    void layout();

    void draw();

    Fl_Pack *p;
    Fl_Box *label;
    int selected;
    int direction;
};

TabWin *win=0;

int TabWin::handle(int ev)
{
    return Fl_Window::handle(ev);
}

void TabWin::draw()
{
    if(damage()&FL_DAMAGE_ALL || damage()&FL_DAMAGE_EXPOSE) {
        Fl_Window::draw();
    }
}

void TabWin::layout()
{
    p->layout();
    int w = p->w()+20;
    if(w<300) w=300;

    int h=100;

    int x = Fl::w()/2-(w/2);
    int y = Fl::h()/2-(h/2);

    resize(x, y, w, h);
    p->resize(10, 10, p->w(), 50);

    label->resize(10, h-30, w-20, 20);

    Fl_Window::layout();
}

static void timeout(void *data) {

    if(!Fl::get_key_state(FL_Alt_L))
    {
        Frame *f = win->selected_frame();
        if(f) {
            f->activate();
            f->raise();
        }

        win->destroy();
        delete win;
        win=0;

    } else
        Fl::repeat_timeout(0.1, timeout, 0);
}



void show_tabmenu(int direction)
{
    if(!win)
    {
        Frame_List list;
        for(uint n=stack_order.size(); n--;) {
            Frame *f = stack_order[n];
            if( (f->state()==NORMAL || f->state()==ICONIC) && (f->desktop()==Desktop::current() || !f->desktop()) && !f->frame_flag(SKIP_LIST) && !f->frame_flag(SKIP_FOCUS) && !f->frame_flag(CLICK_TO_FOCUS) ) {
                list.append(f);
            }
        }
        if(list.count()<1) return;
        if(list.count()==1) {
            list[0]->activate();
            list[0]->raise();
            return;
        }

        win = new TabWin(list, direction);

        XSetWindowAttributes attr;
        attr.border_pixel = 0;
        attr.colormap = fl_colormap;
        attr.bit_gravity = 0;
        attr.event_mask = ExposureMask | StructureNotifyMask
            | KeyPressMask | KeyReleaseMask | KeymapStateMask | FocusChangeMask
            | ButtonPressMask | ButtonReleaseMask
            | EnterWindowMask | LeaveWindowMask
            | PointerMotionMask;
        attr.override_redirect = 1;
        attr.save_under = 0;

        int mask = CWBorderPixel|CWColormap|CWEventMask|CWBitGravity|CWOverrideRedirect|CWSaveUnder;
        Window xid = XCreateWindow(fl_display,
                                   fl_xid(root),
                                   win->x(), win->y(), win->w(), win->h(),
                                   0, // borderwidth
                                   fl_visual->depth,
                                   InputOutput,
                                   fl_visual->visual,
                                   mask, &attr);

        XDestroyWindow(fl_display, fl_xid(win));
        Fl_X::i(win)->xid = xid;

        win->show();
        XMapWindow(fl_display, xid);

        Fl::add_timeout(0.1, timeout, 0);
    }

    win->next();
}

