#include "taskbutton.h"

#include <stdlib.h>
#include <stdio.h>

/*#include <efltk/fl_draw.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Value_Input.h>
#include <efltk/Fl_Divider.h>*/
#include <fltk/draw.h>
#include <fltk/Image.h>
#include <fltk/Fl.h>
#include "EDE_Config.h"
#include <fltk/Box.h>
#include <fltk/Value_Input.h>
#include <fltk/Divider.h>

#include "../edewm/Windowmanager.h"

fltk::Menu_ *TaskButton::menu = 0;
TaskButton *TaskButton::pushed = 0;

int calculate_height(fltk::Menu_ *m)
{
    fltk::Style *s = fltk::Style::find("Menu");
    int Height = s->box->dh();
    for(int n=0; n<m->children(); n++)
    {
        fltk::Widget *i = m->child(n);
        if(!i) break;
        if(!i->visible()) continue;
        fltk::font(i->label_font(), i->label_size());
        Height += i->height()+s->leading;
    }
    return Height;
}

void task_button_cb(TaskButton *b, Window w)
{
    if(fltk::event_button()==fltk::RIGHT_MOUSE) {
        TaskButton::pushed = b;

        TaskButton::menu->color(b->color());
        TaskButton::menu->popup(fltk::event_x(), fltk::event_y()-calculate_height(TaskButton::menu));

    } else {
        if(TaskBar::active==w) {
            XIconifyWindow(fltk::display, w, fltk::screen);
            XSync(fltk::display, True);
            TaskBar::active = 0;
        } else {
            Fl_WM::set_active_window(w);
        }
    }
}

#define CLOSE 1
#define KILL  2
#define MIN   3
//#define MAX   4
#define SET_SIZE 5
#define RESTORE 6

void menu_cb(fltk::Menu_ *menu, void *)
{
    // try to read information how much window can be maximized
    EDE_Config wm_config(find_config_file("wmanager.conf", true));

//pGlobalConfig.get("Panel", "RunHistory", historyString,"");
    Window win = TaskButton::pushed->argument();
    int ID = menu->item()->argument();
    int x, y, width, height, title_height;

     wm_config.get("TitleBar", "Height",title_height);

    switch(ID) {

    case CLOSE:
        if(Fl_WM::close_window(win))
            break;
        // Fallback to kill..

    case KILL:
        XKillClient(fl_display, win);
        break;

    case MIN:
        XIconifyWindow(fltk::display, win, fltk::screen);
        XSync(fltk::display, True);
        TaskBar::active = 0;
        break;

    /*case MAX:
	   // This will come in next version
	   Fl_WM::get_workarea(x, y, width, height);

	   // y koord. poveca za title_height
	   y = y + title_height;

	   XMoveResizeWindow(fl_display, win, x, y, width, height);
	   XSync(fl_display, True);
        break;*/
/*
	case SET_SIZE:
	   {
        	Fl_Window *win = new Fl_Window(300, 110, _("Set Size"));
        	win->begin();

        	Fl_Box *b = new Fl_Box(_("Set size to window:"), 20);
        	b->label_font(b->label_font()->bold());
        	//b = new Fl_Box(menu_frame->label(), 20); //here goes title of window

        	Fl_Group *g = new Fl_Group("", 23);

        	Fl_Value_Input *w_width = new Fl_Value_Input(_("Width:"), 70, FL_ALIGN_LEFT, 60);
        	w_width->step(1);
        	Fl_Value_Input *w_height = new Fl_Value_Input(_("Height:"), 70, FL_ALIGN_LEFT, 60);
        	w_height->step(1);

        	g->end();

        	Fl_Divider *div = new Fl_Divider(10, 15);
        	div->layout_align(FL_ALIGN_TOP);

        	g = new Fl_Group("", 50, FL_ALIGN_CLIENT);

        	//Fl_Button *but = ok_button = new Fl_Button(40,0,100,20, _("&OK"));
        	//but->callback(real_set_size_cb);

        	but = new Fl_Button(155,0,100,20, _("&Cancel"));
        	//but->callback(close_set_size_cb);

        	g->end();

        	win->end();

    		//w_width->value(menu_frame->w());
    		//w_height->value(menu_frame->h());
    		//ok_button->user_data(menu_frame);

    		//win->callback(close_set_size_cb);
    		win->show();

    		}
	   break;*/


    case RESTORE:
        Fl_WM::set_active_window(win);
        break;
    }

    fltk::redraw();
}

TaskButton::TaskButton(Window win) : fltk::Button(0,0,0,0)
{
    layout_align(fltk::ALIGN_LEFT);
    callback((fltk::Callback1*)task_button_cb, win);

    if(!menu) {
        fltk::Group *saved = fltk::Group::current();
        fltk::Group::current(0);

        menu = new fltk::Menu_();
        menu->callback((fltk::Callback*)menu_cb);

        //Fl_Widget* add(const char*, int shortcut, Fl_Callback*, void* = 0, int = 0);
	   
        menu->add(_(" Close   "), 0, 0, (void*)CLOSE, fltk::MENU_DIVIDER);
	   new fltk::Divider(10, 15);
        menu->add(_(" Kill"), 0, 0, (void*)KILL, fltk::MENU_DIVIDER);
	   new fltk::Divider(10, 15);

	   //Comes in next version
        //menu->add(" Maximize", 0, 0, (void*)MAX);
        menu->add(_(" Minimize"), 0, 0, (void*)MIN);
        menu->add(_(" Restore"), 0, 0, (void*)RESTORE);
	   //menu->add(" Set size", 0, 0, (void*)SET_SIZE, FL_MENU_DIVIDER);
        //-----

        fltk::Group::current(saved);
    }
}

/////////////////////////
// Task bar /////////////
/////////////////////////

#include "icons/tux.xpm"
static fltk::Image default_icon(tux_xpm);

// Forward declaration
static int GetState(Window w);

Window TaskBar::active = 0;
bool   TaskBar::variable_width = true;

TaskBar::TaskBar()
: fltk::Group(0,0,0,0)
{
    m_max_taskwidth = 150;

    layout_align(fltk::ALIGN_CLIENT);
    layout_spacing(2);
    
    EDE_Config pConfig(find_config_file("ede.conf", true));
    pConfig.get("Panel", "VariableWidthTaskbar",variable_width,true);

    update();
    end();
}

void TaskBar::layout()
{
    if(!children()) return;

    int maxW = w()-layout_spacing()*2;
    int avgW = maxW / children();
    int n;

    int buttonsW = layout_spacing()*2;
    for(n=0; n<children(); n++) {
        int W = child(n)->width(), tmph;

        if(TaskBar::variable_width) {
            child(n)->preferred_size(W, tmph);
            W += 10;
        } else
            W = avgW;

        if(W > m_max_taskwidth) W = m_max_taskwidth;

        child(n)->w(W);
        buttonsW += W+layout_spacing();
    }

    float scale = 0.0f;
    if(buttonsW > maxW)
        scale = (float)maxW / (float)buttonsW;

    int X=layout_spacing();
    for(n=0; n<children(); n++) {
        fltk::Widget *widget = child(n);
        int W = widget->w();
        if(scale>0.0f)
            W = (int)((float)W * scale);

        widget->resize(X, 0, W, h());
        X += widget->w()+layout_spacing();
    }

    fltk::Widget::layout();
}

void TaskBar::update()
{
    Fl_WM::clear_handled();

    int n;

    // Delete all icons:
    for(n=0; n<children(); n++)
    {
        fltk::Image *i = child(n)->image();
        if(i!=&default_icon)
            delete i;
    }
    clear();

    Window *wins=0;
    int num_windows = Fl_WM::get_windows_mapping(wins);

    if(num_windows<=0)
        return;

    fltk::Int_List winlist;
    int current_workspace = Fl_WM::get_current_workspace();
    for(n=0; n<num_windows; n++) {
        if(current_workspace == Fl_WM::get_window_desktop(wins[n]) && GetState(wins[n])>0)
            winlist.append(wins[n]);
    }

    if(winlist.size()>0) {
        for(n=0; n<(int)winlist.size(); n++) {
            add_new_task(winlist[n]);
        }
    }
    delete []wins;

    relayout();
    redraw();
    parent()->redraw();
}

void TaskBar::update_active(Window active)
{
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *w = child(n);
        Window win = w->argument();

        if(GetState(win) == IconicState)
            w->label_color(fltk::inactive(fltk::BLACK));
        else
            w->label_color(fltk::Button::default_style->label_color);

        if(active==win) {
            TaskBar::active = win;
            w->set_value();
            w->color(fltk::lighter(fltk::Button::default_style->color));
            w->highlight_color(fltk::lighter(fltk::Button::default_style->color));
        } else {
            w->clear_value();
            w->color(fltk::Button::default_style->color);
            w->highlight_color(fltk::Button::default_style->highlight_color);
        }
    }
    redraw();
}

void TaskBar::update_name(Window win)
{
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *w = child(n);
        Window window = w->argument();

        if(window==win) {
            char *name = 0;
            bool ret = Fl_WM::get_window_icontitle(win, name);
            if(!ret || !name) ret = Fl_WM::get_window_title(win, name);

            if(ret && name) {
                w->label(name);
                w->tooltip(name);
                free(name);
            } else {
                w->label("...");
                w->tooltip("...");
            }

            // Update icon also..
            fltk::Image *icon = w->image();
            if(icon!=&default_icon) delete icon;

            if(Fl_WM::get_window_icon(win, icon, 16, 16))
                w->image(icon);
            else
                w->image(default_icon);

            w->redraw();
            break;
        }
    }
    redraw();
}

void TaskBar::minimize_all()
{
    Window *wins=0;
    int num_windows = Fl_WM::get_windows_mapping(wins);

    int current_workspace = Fl_WM::get_current_workspace();
    for(int n=0; n<num_windows; n++) {
        if(current_workspace == Fl_WM::get_window_desktop(wins[n]) && GetState(wins[n])>0)
            XIconifyWindow(fltk::display, wins[n], fltk::screen);
    }
    XSync(fl_display, True);
    active = 0;
}

void TaskBar::add_new_task(Window w)
{
    // Add to Fl_WM module handled windows.
    Fl_WM::handle_window(w);

    TaskButton *b;
    char *name = 0;
    fltk::Image *icon = 0;

    if (!w) return;

    begin();

    b = new TaskButton(w);

    bool ret = Fl_WM::get_window_icontitle(w, name);
    if(!ret || !name) ret = Fl_WM::get_window_title(w, name);

    if(ret && name) {
        b->label(name);
        b->tooltip(name);
        free(name);
    } else {
        b->label("...");
        b->tooltip("...");
    }

    if(Fl_WM::get_window_icon(w, icon, 16, 16)) {
        b->image(icon);
    } else {
        b->image(default_icon);
    }

    b->accept_focus(false);
    b->align(fltk::ALIGN_LEFT | fltk::ALIGN_INSIDE | fltk::ALIGN_CLIP);

    if(Fl_WM::get_active_window()==w) {
        b->value(1);
        active = w;
    }

    if(GetState(w) == IconicState)
        b->label_color(fltk::inactive(fltk::BLACK));

    end();
}

//////////////////////////

static void* getProperty(Window w, Atom a, Atom type, unsigned long *np, int *ret)
{
    Atom realType;
    int format;
    unsigned long n, extra;
    int status;
    uchar *prop=0;
    status = XGetWindowProperty(fltk::display, w, a, 0L, 0x7fffffff,
                                False, type, &realType,
                                &format, &n, &extra, (uchar**)&prop);
    if(ret) *ret = status;
    if (status != Success) return 0;
    if (!prop) { return 0; }
    if (!n) { XFree(prop); return 0; }
    if (np) *np = n;
    return prop;
}

static int getIntProperty(Window w, Atom a, Atom type, int deflt, int *ret)
{
    void* prop = getProperty(w, a, type, 0, ret);
    if(!prop) return deflt;
    int r = int(*(long*)prop);
    XFree(prop);
    return r;
}

// 0 ERROR
// Otherwise state
static int GetState(Window w)
{
    static Atom _XA_WM_STATE = 0;
    if(!_XA_WM_STATE) _XA_WM_STATE = XInternAtom(fltk::display, "WM_STATE", False);

    int ret=0;
    int state = getIntProperty(w, _XA_WM_STATE, _XA_WM_STATE, 0, &ret);
    if(ret!=Success) return 0;
    return state;
}
