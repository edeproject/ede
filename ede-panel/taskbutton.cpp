#include "taskbutton.h"

#include <stdlib.h>
#include <stdio.h>

#include <efltk/fl_draw.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Value_Input.h>
#include <efltk/Fl_Divider.h>
#include "../edewm/Windowmanager.h"

// Forward declaration
static int GetState(Window w);
static bool IsWindowManageable(Window w);

Fl_Menu_ *TaskButton::menu = 0;
TaskButton *TaskButton::pushed = 0;

int calculate_height(Fl_Menu_ *m)
{
	Fl_Style *s = Fl_Style::find("Menu");
	int Height = s->box->dh();
	for(int n=0; n<m->children(); n++)
	{
		Fl_Widget *i = m->child(n);
		if(!i) break;
		if(!i->visible()) continue;
		fl_font(i->label_font(), i->label_size());
		Height += i->height()+s->leading;
	}
	return Height;
}

void task_button_cb(TaskButton *b, Window w)
{
	if(Fl::event_button()==FL_RIGHT_MOUSE)
	{
		// Window will lose focus so lets reflect this on taskbar
		TaskBar::active = 0;

		// 'pushed' lets the menu_cb() know which button was pressed
		TaskButton::pushed = b;

		// Create menu
		TaskButton::menu->color(b->color());
		TaskButton::menu->popup(Fl::event_x(), Fl::event_y()-calculate_height(TaskButton::menu));

	}
	else
	{
		if(TaskBar::active==w)
		{
			// Click on active window will minimize it
			XIconifyWindow(fl_display, w, fl_screen);
			XSync(fl_display, True);
			TaskBar::active = 0;
			b->m_minimized=true;
		}
		else
		{
			// Else give focus to window
			Fl_WM::set_active_window(w);
			TaskBar::active = w;
			b->m_minimized=false;
		}
	}
}

#define CLOSE   1
#define KILL    2
#define MIN 3
#define MAX 4
//#define SET_SIZE 5
#define RESTORE 6

void menu_cb(Fl_Menu_ *menu, void *)
{
	// try to read information how much window can be maximized
	//int title_height;
	//Fl_Config wm_config(fl_find_config_file("wmanager.conf", true));
	//wm_config.get("TitleBar", "Height",title_height,20);

	//int frame_width=3; // pixels

	Window win = TaskButton::pushed->argument();
	int ID = menu->item()->argument();
	//int x, y, width, height;

	Atom _XA_NET_WM_STATE = XInternAtom(fl_display, "_NET_WM_STATE", False);
	Atom _XA_EDE_WM_ACTION= XInternAtom(fl_display, "_EDE_WM_ACTION", False);

	// use _NET_WM_STATE_MAXIMIZED_HORZ or _NET_WM_ACTION_MAXIMIZE_HORZ ???
	Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	Atom _XA_NET_WM_STATE_MAXIMIZED_VERT = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	Atom _XA_EDE_WM_RESTORE_SIZE = XInternAtom(fl_display, "_EDE_WM_RESTORE_SIZE", False);
	XClientMessageEvent evt;

	switch(ID)
	{

		case CLOSE:
			// TODO: closing should be handled by edewm
			if(Fl_WM::close_window(win))
				break;

		case KILL:
			// TODO: killing should be handled by edewm
			XKillClient(fl_display, win);
			break;

		case MIN:
			XIconifyWindow(fl_display, win, fl_screen);
			XSync(fl_display, True);
			TaskBar::active = 0;
			TaskButton::pushed->m_minimized=true;
			break;

		case MAX:
			memset(&evt, 0, sizeof(evt));
			evt.type = ClientMessage;
			evt.window = win;
			evt.message_type = _XA_NET_WM_STATE;
			evt.format = 32;
			evt.data.l[0] = 0;
			evt.data.l[1] = _XA_NET_WM_STATE_MAXIMIZED_HORZ;
			evt.data.l[2] = _XA_NET_WM_STATE_MAXIMIZED_VERT;
			XSendEvent(fl_display, RootWindow(fl_display, DefaultScreen(fl_display)), False, SubstructureNotifyMask,
				(XEvent*)& evt);

			TaskButton::pushed->m_minimized=false;
			TaskBar::active = win;

			/*
			Fl_WM::get_workarea(x, y, width, height);

			// leave room for widgets
			y += title_height+frame_width;
			x += frame_width;
			height -= title_height+frame_width*2;
			width -= frame_width*2;

			// resize window
			XMoveResizeWindow(fl_display, win, x, y, width, height);
			XSync(fl_display, True);

			// window lost focus so lets be nice and give it back
			Fl_WM::set_active_window(win);
			TaskBar::active = win;
			TaskButton::pushed->m_minimized=false;
			*/

			break;
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
			//Fl_WM::set_active_window(win);
			memset(&evt, 0, sizeof(evt));
			evt.type = ClientMessage;
			evt.window = win;
			evt.message_type = _XA_EDE_WM_ACTION;
			evt.format = 32;
			evt.data.l[0] = _XA_EDE_WM_RESTORE_SIZE;
			XSendEvent(fl_display, RootWindow(fl_display, DefaultScreen(fl_display)), False, SubstructureNotifyMask,
				(XEvent*)& evt);
			TaskBar::active = win;
			TaskButton::pushed->m_minimized=false;
			break;
	}

	Fl::redraw();
}

TaskButton::TaskButton(Window win) : Fl_Button(0,0,0,0)
{
	layout_align(FL_ALIGN_LEFT);
	callback((Fl_Callback1*)task_button_cb, win);

	if(!menu)
	{
		Fl_Group *saved = Fl_Group::current();
		Fl_Group::current(0);

		menu = new Fl_Menu_();
		menu->callback((Fl_Callback*)menu_cb);

		//Fl_Widget* add(const char*, int shortcut, Fl_Callback*, void* = 0, int = 0);

		menu->add(_(" Close   "), 0, 0, (void*)CLOSE, FL_MENU_DIVIDER);
		new Fl_Divider(10, 15);
		menu->add(_(" Kill"), 0, 0, (void*)KILL, FL_MENU_DIVIDER);
		new Fl_Divider(10, 15);

		menu->add(_(" Maximize        "), 0, 0, (void*)MAX);
		menu->add(_(" Minimize"), 0, 0, (void*)MIN);
		menu->add(_(" Restore"), 0, 0, (void*)RESTORE);
		//menu->add(" Set size", 0, 0, (void*)SET_SIZE, FL_MENU_DIVIDER);
		//-----

		Fl_Group::current(saved);
	}
}

void TaskButton::draw()
{
	// Draw method contains most of the logic of taskbar button
	// appearance

	// TODO: There seems to be no X event when window get minimized
	// so taskbar might not reflect it until next update() call

	// TODO: See how we can make minimized icons gray...

	Fl_Color color = Fl_Button::default_style->color;
	Fl_Color lcolor = Fl_Button::default_style->label_color;

	Window win = this->argument();
	this->color(color);
	this->highlight_color(fl_lighter(color));
	this->label_color(lcolor);
	this->highlight_label_color(lcolor);
	this->box(FL_UP_BOX);

	if (win == TaskBar::active)
	{
		this->box(FL_DOWN_BOX);
		this->color(fl_lighter(color));
		this->highlight_color(fl_lighter(fl_lighter(color)));
	}
	else if (m_minimized)
	{
		this->label_color(fl_lighter(fl_lighter(lcolor)));
		this->highlight_label_color(fl_lighter(fl_lighter(lcolor)));
	}
	Fl_Button::draw();
}

/////////////////////////
// Task bar /////////////
/////////////////////////

#include "icons/tux.xpm"
static Fl_Image default_icon(tux_xpm);

Window TaskBar::active = 0;
bool   TaskBar::variable_width = true;
bool   TaskBar::all_desktops = false;

TaskBar::TaskBar()
: Fl_Group(0,0,0,0)
{
	m_max_taskwidth = 150;

	layout_align(FL_ALIGN_CLIENT);
	layout_spacing(2);

	Fl_Config pConfig(fl_find_config_file("ede.conf", true));
	pConfig.get("Panel", "VariableWidthTaskbar",variable_width,true);
	pConfig.get("Panel", "AllDesktops",all_desktops,false);

	update();
	end();
}

// Logic for button sizes

void TaskBar::layout()
{
	if(!children()) return;

	int maxW = w()-layout_spacing()*2;
	int avgW = maxW / children();
	int n;

	int buttonsW = layout_spacing()*2;
	for(n=0; n<children(); n++)
	{
		int W = child(n)->width(), tmph;

		if(TaskBar::variable_width)
		{
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
	for(n=0; n<children(); n++)
	{
		Fl_Widget *widget = child(n);
		int W = widget->w();
		if(scale>0.0f)
			W = (int)((float)W * scale);

		widget->resize(X, 0, W, h());
		X += widget->w()+layout_spacing();
	}

	Fl_Widget::layout();
}

// Recreate all buttons in taskbar
// Called... occasionally ;)

void TaskBar::update()
{
	Fl_WM::clear_handled();

	int n;

	// Delete all icons:
	for(n=0; n<children(); n++)
	{
		Fl_Image *i = child(n)->image();
		if(i && i!=&default_icon)
			delete i;
	}
	clear();

	Window *wins=0;
	int num_windows = Fl_WM::get_windows_mapping(wins);

	if(num_windows<=0)
		return;

	// Cleared window list
	Fl_Int_List winlist;
	int current_workspace = Fl_WM::get_current_workspace();
	for(n=0; n<num_windows; n++)
	{
		if ((all_desktops
			|| current_workspace == Fl_WM::get_window_desktop(wins[n]))
			&& IsWindowManageable(wins[n]))
		{
			winlist.append(wins[n]);
		}
	}

	if(winlist.size()>0)
	{
		for(n=0; n<(int)winlist.size(); n++)
		{
			add_new_task(winlist[n]);
		}
	}
	free(wins);

	// Probably redundant
	relayout();
	redraw();
	parent()->redraw();
}

// Called when another window becomes active

void TaskBar::update_active(Window active)
{
	if (active)
		if (IsWindowManageable(active)) TaskBar::active=active;
	redraw();
}

// A somewhat lighter version of update() when window
// just changes name
// Some apps like to do it a lot

void TaskBar::update_name(Window win)
{
	//printf ("+update_name\n");
	for(int n=0; n<children(); n++)
	{
		Fl_Widget *w = child(n);
		Window window = w->argument();

		if(window==win)
		{
			char *name = 0;
			bool ret = Fl_WM::get_window_icontitle(win, name);
			if(!ret || !name) ret = Fl_WM::get_window_title(win, name);

			if(ret && name)
			{
				w->label(name);
				w->tooltip(name);
				free(name);
			}
			else
			{
				w->label("...");
				w->tooltip("...");
			}

			// Update icon also..
			Fl_Image *icon = w->image();
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

// Callback for show desktop button

void TaskBar::minimize_all()
{
	Window *wins=0;
	int num_windows = Fl_WM::get_windows_mapping(wins);

	int current_workspace = Fl_WM::get_current_workspace();
	for(int n=0; n<num_windows; n++)
	{
		if(current_workspace == Fl_WM::get_window_desktop(wins[n]))
			XIconifyWindow(fl_display, wins[n], fl_screen);
	}
	XSync(fl_display, True);
	active = 0;
}

// Called by update() to add a single button

void TaskBar::add_new_task(Window w)
{
	if (!w) return;

	// Add to Fl_WM module handled windows.
	Fl_WM::handle_window(w);

	TaskButton *b;
	char *name = 0;
	Fl_Image *icon = 0;

	begin();

	b = new TaskButton(w);

	bool ret = Fl_WM::get_window_icontitle(w, name);
	if(!ret || !name) ret = Fl_WM::get_window_title(w, name);

	if(ret && name)
	{
		b->label(name);
		b->tooltip(name);
		free(name);
	}
	else
	{
		b->label("...");
		b->tooltip("...");
	}

	if(Fl_WM::get_window_icon(w, icon, 16, 16))
	{
		b->image(icon);
	}
	else
	{
		b->image(default_icon);
	}

	b->accept_focus(false);
	b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	if(Fl_WM::get_active_window()==w)
	{
		TaskBar::active = w;
	}

	if(GetState(w) == IconicState)
		b->m_minimized=true;
	else
		b->m_minimized=false;

	end();
}

//////////////////////////

// Below are window management functions that aren't present in Fl_WM
// We can't add them to Fl_WM because we change api, and efltk will be
// dropped in EDE 2.0.... So this functions are just duct tape...
// ugly, non-generic, non-optimized...

static void* getProperty(Window w, Atom a, Atom type, unsigned long *np, int *ret)
{
	Atom realType;
	int format;
	unsigned long n, extra;
	int status;
	uchar *prop=0;
	status = XGetWindowProperty(fl_display, w, a, 0L, 0x7fffffff,
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
// 3 - IconicState
// 1 - NormalState
// 2 - WithdrawnState
static int GetState(Window w)
{
	static Atom _XA_WM_STATE = 0;
	if(!_XA_WM_STATE) _XA_WM_STATE = XInternAtom(fl_display, "WM_STATE", False);

	int ret=0;
	int state = getIntProperty(w, _XA_WM_STATE, _XA_WM_STATE, 0, &ret);
	if(ret!=Success) return 0;
	return state;
}

// Returns false if window w is of type DESKTOP or DOCK
// Not generic enough!!

static bool IsWindowManageable(Window w)
{

	static Atom _XA_NET_WM_WINDOW_TYPE = 0;
	if(!_XA_NET_WM_WINDOW_TYPE) _XA_NET_WM_WINDOW_TYPE = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);
	static Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP = 0;
	if(!_XA_NET_WM_WINDOW_TYPE_DESKTOP) _XA_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	static Atom _XA_NET_WM_WINDOW_TYPE_DOCK = 0;
	if(!_XA_NET_WM_WINDOW_TYPE_DOCK) _XA_NET_WM_WINDOW_TYPE_DOCK = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	static Atom _XA_NET_WM_WINDOW_TYPE_SPLASH = 0;
	if(!_XA_NET_WM_WINDOW_TYPE_SPLASH) _XA_NET_WM_WINDOW_TYPE_SPLASH = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_SPLASH", False);

	Atom window_type = None;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_return = NULL;

	if(Success == XGetWindowProperty(fl_display, w, _XA_NET_WM_WINDOW_TYPE, 0L, sizeof(Atom),
		False, XA_ATOM, &actual_type,
		&actual_format, &nitems, &bytes_after,
		&prop_return) && prop_return)
	{
		window_type = *(Atom *)prop_return;
		XFree(prop_return);
	}

	if(window_type == _XA_NET_WM_WINDOW_TYPE_DESKTOP
		|| window_type == _XA_NET_WM_WINDOW_TYPE_DOCK
		|| window_type == _XA_NET_WM_WINDOW_TYPE_SPLASH)
		return false;
	else return true;
}
