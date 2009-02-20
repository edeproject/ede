#include "Titlebar.h"
#include "Windowmanager.h"
#include "Frame.h"
#include "Desktop.h"
#include "Icon.h"
#include "Theme.h"

#include <efltk/Fl_Item.h>
#include <efltk/Fl_Divider.h>
#include <efltk/fl_draw.h>
#include <efltk/Fl_Util.h>
#include <efltk/Fl_Menu_.h>
#include <efltk/Fl.h>
#include <efltk/fl_ask.h>

#include <sys/select.h>
#include <sys/time.h>

#include "config.h"
#include "debug.h"

extern bool grab();
extern void grab_cursor(bool grab);

int Titlebar::box_type		 = 0;
int Titlebar::label_align	 = FL_ALIGN_LEFT;
int Titlebar::default_height = 20;

//Fl_Config wm_config(fl_find_config_file("wmanager.conf", true));

static Fl_Menu_ *title_menu=0;
Frame *menu_frame=0; //This is set to frame,where menu were popped up

// this is called when user clicks the buttons:
void Frame::cb_button_close(Fl_Button* b)
{
	close();
}

void Frame::cb_button_kill(Fl_Button* b)
{
	kill();
}

/*
static void animate(int fx, int fy, int fw, int fh,
					int tx, int ty, int tw, int th)
{
# undef max
# define max(a,b) (a) > (b) ? (a) : (b)
	double max_steps = max( (tw-fw), (th-fh) );
	double min_steps = max( (fw-tw), (fh-th) );
	double steps = max(max_steps, min_steps);
	steps/=Frame::animate_speed;

	double sx = max( ((double)(fx-tx)/steps), ((double)(tx-fx)/steps) );
	double sy = max( ((double)(fy-ty)/steps), ((double)(ty-fy)/steps) );
	double sw = max( ((double)(fw-tw)/steps), ((double)(tw-fw)/steps) );
	double sh = max( ((double)(fh-th)/steps), ((double)(th-fh)/steps) );

	int xinc = fx < tx ? 1 : -1;
	int yinc = fy < ty ? 1 : -1;
	int winc = fw < tw ? 1 : -1;
	int hinc = fh < th ? 1 : -1;
	double rx=fx,ry=fy,rw=fw,rh=fh;

	XGrabServer(fl_display);

	while(steps-- > 0) {

		rx+=(sx*xinc);
		ry+=(sy*yinc);
		rw+=(sw*winc);
		rh+=(sh*hinc);

		draw_overlay((int)rx, (int)ry, (int)rw, (int)rh);

		Fl::sleep(10);
		XFlush(fl_display);
	}

	clear_overlay();

	XUngrabServer(fl_display);
}

void Frame::cb_button_max(Fl_Button* b)
{
	if(maximized) {
		if(Frame::animate) {
			::animate(x(), y(), w(), h(),
					  restore_x, restore_y, restore_w, restore_h);
		}

		set_size(restore_x, restore_y, restore_w, restore_h);
		maximized = false;

	} else {

		bool m = true;

		restore_x = x();
		restore_y = y();
		restore_w = w();
		restore_h = h();

		int W=root->w(), H=root->h();
		W-=offset_w;
		H-=offset_h;
		if(ICCCM::get_size(this, W, H)) {
			m=false;
		}
		W+=offset_w;
		H+=offset_h;

		if(Frame::animate) {
			::animate(x(), y(), w(), h(),
					  root->x(), root->y(), root->w(), root->h());
		}

		set_size(root->x(), root->y(), W, H);

		maximized = m;
		redraw();
	}
}
*/

void Frame::cb_button_max(Fl_Button* b)
{
	if(maximized)
		restore();
	else
		maximize();
}

void Frame::cb_button_min(Fl_Button* b) 
{
	iconize();
}

// Min/Max/Close button symbols drawing stuff:
extern int fl_add_symbol(const char *name, void (*drawit)(Fl_Color), int scalable);

#define vv(x,y) fl_vertex(x,y)
void draw_cl(Fl_Color col)
{
	fl_rotate(45);
	fl_color(col);

	vv(-0.9f,-0.12f); vv(-0.9f,0.12f); vv(0.9f,0.12f); vv(0.9f,-0.12f); fl_fill_stroke(FL_DARK3);
	vv(-0.12f,-0.9f); vv(-0.12f,0.9f); vv(0.12f,0.9f); vv(0.12f,-0.9f); fl_fill_stroke(FL_DARK3);
}

#define MAX_OF .6f
void draw_max(Fl_Color col)
{			
	fl_color(col);

	vv(-MAX_OF, -MAX_OF); vv(MAX_OF, -MAX_OF);
	vv(MAX_OF,-MAX_OF+0.4); vv(-MAX_OF,-MAX_OF+0.4);
	fl_fill();

	vv(MAX_OF,-MAX_OF); vv(MAX_OF,MAX_OF);
	vv(-MAX_OF,MAX_OF); vv(-MAX_OF,-MAX_OF);
	fl_stroke();
}

#define MIN_OF .5f
void draw_min(Fl_Color col)
{
	fl_color(col);

	vv(-MIN_OF, MIN_OF); vv(MIN_OF, MIN_OF);
	vv(MIN_OF, MIN_OF+.2f); vv(-MIN_OF, MIN_OF+.2f);
	fl_fill();
}

// static callbacks for efltk:
void button_cb_close(Fl_Widget *w, void *d)
{
	Frame *f = d ? (Frame *)d : menu_frame;
	f->cb_button_close((Fl_Button*)w);
}

void button_cb_kill(Fl_Widget *w, void *d)
{
	Frame *f = d ? (Frame *)d : menu_frame;
	f->cb_button_kill((Fl_Button*)w);
}

void button_cb_max(Fl_Widget *w, void *d)
{
	Frame *f = d ? (Frame *)d : menu_frame;
	f->cb_button_max((Fl_Button*)w);
}

void button_cb_min(Fl_Widget *w, void *d)
{
	Frame *f = d ? (Frame *)d : menu_frame;
	f->cb_button_min((Fl_Button*)w);
}

void Titlebar::cb_change_desktop(Fl_Widget *w, void *data)
{
	Desktop *d = (Desktop*)data;
	menu_frame->desktop_ = d;

	if(d && d!=Desktop::current()) {
		Desktop::current(d);
		menu_frame->raise();
	}

	menu_frame->send_desktop();
	menu_frame=0;
}

void update_desktops(Fl_Group *g)
{
	g->clear();
	g->begin();

	Fl_Item *i;

	i = new Fl_Item(_("Sticky"));
	i->type(Fl_Item::TOGGLE);
	i->callback(Titlebar::cb_change_desktop);
	if(menu_frame->desktop()) {
		i->clear_value();
		i->user_data(0);
	} else {
		i->set_value();
		i->user_data(Desktop::current());
	}


	new Fl_Divider();

	for(uint n=0; n<desktops.size(); n++) {
		Desktop *d = desktops[n];
		i = new Fl_Item(d->name());
		i->type(Fl_Item::RADIO);

		if(menu_frame->desktop()==d) i->set_value();
		else i->clear_value();

		i->callback(Titlebar::cb_change_desktop, d);
	}

	g->end();
}

#include "WMWindow.h"
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Value_Input.h>
#include <efltk/fl_ask.h>

Fl_Button *ok_button;
Fl_Value_Input *w_width, *w_height;
//Fl_Input *size;
static WMWindow *win = 0;

static void real_set_size_cb(Fl_Widget *w, void *d)
{
	Frame *f = (Frame*)d;

	int W = (int)w_width->value();
	int H = (int)w_height->value();
	ICCCM::get_size(f, W, H);

	f->set_size(f->x(), f->y(), W, H);
	f->redraw();

	win->destroy();
}

static void close_set_size_cb(Fl_Widget *w, void *d)
{
	win->destroy();
}

static void set_size_cb(Fl_Widget *w, void *d)
{
	win = new WMWindow(300, 110, _("Set size"));
	Fl_Box *b = new Fl_Box(5, 5, 295, 15, _("Set size to window:"));
	b->label_font(b->label_font()->bold());

	Fl_String tmplabel = menu_frame->label();
	if (tmplabel.length() > 50)
		tmplabel = tmplabel.sub_str(0,20) + " ... " + tmplabel.sub_str(tmplabel.length()-20,20);

	b = new Fl_Box(0, 20, 296, 15, tmplabel);

	w_width = new Fl_Value_Input(45, 45, 90, 20, _("width:"));
		w_width->step(1);
	w_height = new Fl_Value_Input(195, 45, 90, 20, _("height:"));
	 w_height->step(1);
	new Fl_Divider(5, 70, 290, 10);

	 Fl_Button *but = ok_button = new Fl_Button(60,80,85,25, _("&OK"));
	 but->callback(real_set_size_cb);

	 but = new Fl_Button(155, 80, 85, 25, _("&Cancel"));
	 but->callback(close_set_size_cb);

	win->end();

	w_width->value(menu_frame->w());
	w_height->value(menu_frame->h());
	ok_button->user_data(menu_frame);

	win->callback(close_set_size_cb);
	win->show();
}

void Titlebar::popup_menu(Frame *frame)
{
	menu_frame = frame;

	static Fl_Widget *max;
	static Fl_Widget *set_size;
	static Fl_Widget *min;
	static Fl_Group *desktop;

	if(!title_menu) {
		title_menu = new Fl_Menu_();
		max=title_menu->add(_("Maximize"), 0, button_cb_max);
		min=title_menu->add(_("Minimize"), 0, button_cb_min);
		set_size=title_menu->add(_("Set size"), 0, set_size_cb, 0, FL_MENU_DIVIDER);
		desktop=(Fl_Group*)title_menu->add(_("To Desktop"), 0, 0, 0, FL_SUBMENU|FL_MENU_DIVIDER);
		title_menu->add(_("Kill"), 0, button_cb_kill);
		title_menu->add(_("Close"), 0, button_cb_close);
		title_menu->end();

	}

	update_desktops(desktop);

	if(menu_frame->maximized) max->label(_("Restore"));
	else max->label(_("Maximize"));


	// we don't want animation for dialogs and utils frames
	// MWM hints can set MAXIMIZE and MINIMIZE options separately
	// so we must check them each
	if(menu_frame->window_type() == TYPE_NORMAL)
	{
		if(menu_frame->decor_flag(MAXIMIZE))
		{
			max->activate();
			set_size->activate();
		}
		if(menu_frame->decor_flag(MINIMIZE))
			min->activate();
	}
	else
	{
		max->deactivate();
		set_size->deactivate();
		min->deactivate();
	}

	title_menu->Fl_Group::focus(-1);
	title_menu->popup(Fl::event_x_root(), Fl::event_y_root());

	menu_frame = 0;
}

Titlebar_Button::Titlebar_Button(int type)
: Fl_Button(0,0,0,0), m_type(type)
{
	focus_box(FL_NO_BOX);
	label_type(FL_SYMBOL_LABEL);

	switch(m_type) {
	case TITLEBAR_MAX_UP: label("@mx"); break;
	case TITLEBAR_MIN_UP: label("@ii"); break;
	case TITLEBAR_CLOSE_UP: label("@xx"); break;
	};
}

void Titlebar_Button::draw()
{
	int idx = m_type;
	if(flags() & FL_VALUE) idx++;

	Fl_Image *i = Theme::image(idx);
	if(i) {
		Fl_Flags scale = 0;
		if(i->height()!=h()) scale = FL_ALIGN_SCALE;
		i->draw(0,0,w(),h(), scale);
	} else {
		Fl_Button::draw();
	}
}

Titlebar::Titlebar(int x,int y,int w,int h,const char *l)
: Fl_Window(x,y,w,h,0),
_close(TITLEBAR_CLOSE_UP), _max(TITLEBAR_MAX_UP), _min(TITLEBAR_MIN_UP)
{
	f = (Frame *)parent();
	title_icon = 0;
	text_w=0;

	static bool init = false;
	if(!init) {
		fl_add_symbol("xx", draw_cl, 1);
		fl_add_symbol("mx", draw_max, 1);
		fl_add_symbol("ii", draw_min, 1);
		init = true;
	}

	setting_changed();
	end();
}

Titlebar::~Titlebar()
{
}

void Titlebar::setting_changed()
{
	_close.callback(button_cb_close, f);
	_max.callback(button_cb_max, f);
	_min.callback(button_cb_min, f);

	if(Titlebar::default_height != h()) {
		h(Titlebar::default_height);
		f->updateBorder();
	}

	layout();
	redraw();
}

void Titlebar::show()
{
	if(!shown()) create();

	XMapWindow(fl_display, fl_xid(this));
	XRaiseWindow(fl_display, fl_xid(this));
}

void Titlebar::hide()
{
	if(shown())
		XUnmapWindow(fl_display, fl_xid(this));
}

#define set_box(b) if(box()!=b) box(b); break

void Titlebar::layout()
{
	if(Theme::use_theme()) {
		if(box()!=FL_FLAT_BOX) box(FL_FLAT_BOX);
	} else {
		switch(box_type) {
		default:
		case 0: set_box(FL_FLAT_BOX);
		case 1: set_box(FL_HOR_SHADE_FLAT_BOX);
		case 2: set_box(FL_THIN_DOWN_BOX);
		case 3: set_box(FL_UP_BOX);
		case 4: set_box(FL_DOWN_BOX);
		case 5: set_box(FL_PLASTIC_BOX);
		}
	}

	int W = w()-box()->dx();

	int lsize = h()/2+2;
	label_size(lsize);

	// Try to detect what buttons are showed
	if(!f->func_flag(MINIMIZE)) _min.deactivate(); else _min.activate();
	if(!f->func_flag(MAXIMIZE)) _max.deactivate(); else _max.activate();

	if(!f->decor_flag(MINIMIZE) || f->frame_flag(KEEP_ASPECT)) _min.hide();
	if(!f->decor_flag(MAXIMIZE) || f->frame_flag(KEEP_ASPECT)) _max.hide();

	if(f->size_hints->min_width==f->size_hints->max_width || f->transient_for_xid) {
		_min.hide();
		_max.hide();
	} else {
		_min.show();
		_max.show();
	}

	int offset=0;
	int s = h();
	int mid = 0;
	if(!Theme::use_theme()) {
		s -= 4;
		mid = 2;
		offset=2;
	}
	int bx = W-s-offset;

	_close.resize(bx, mid, s, s);
	if(_close.visible()) bx -= s+offset;

	_max.resize(bx, mid, s, s);
	if(_max.visible()) bx -= s+offset;

	_min.resize(bx, mid, s, s);

	text_w = bx - (f->decor_flag(SYSMENU)?h():0) - 10;

	fl_font(label_font(), label_size());
	if(!f->label().empty()) {
		Fl_Widget::label(fl_cut_line(f->label(), text_w));
		if(strcmp(label(), f->label())) {
			tooltip(f->label());
		} else {
			tooltip("");
		}
	} else {
		label("");
		tooltip("");
	}

	// Reset layout flags
	Fl_Widget::layout();
}

void Titlebar::draw()
{
	DBG("Titlebar::draw(): %s", label().c_str());

	if(Theme::use_theme() && Theme::image(TITLEBAR_BG)) {
		int X=0, Y=0, W=w(), H=h();
		Theme::image(TITLEBAR_BG)->draw(X,Y,W,H,FL_ALIGN_SCALE);
	} else {
		draw_box();
	}

	int s=h()-4;

	// Resize and set image & mask
	if(f->icon() && f->decor_flag(SYSMENU)) {
		title_icon = f->icon()->get_icon(s, s);
	} else
		title_icon = 0;

	int tx = box()->dx()+1;

	if(title_icon) {
		title_icon->draw(2,2, s, s);
		tx += s + 5;	// Separate text a few pixels from icon
	}

	draw_label(tx, 0, text_w, h(), Titlebar::label_align);

	draw_child(_close);
	draw_child(_max);
	draw_child(_min);
}


int Titlebar::handle(int event)
{
	static bool grabbed=false;
	static int dx,dy,nx,ny;

	switch(event)
	{
	case FL_PUSH: {
		if(grabbed) return 1;

		dx = Fl::event_x_root()-f->x();
		dy = Fl::event_y_root()-f->y();
		nx = Fl::event_x_root()-dx;
		ny = Fl::event_y_root()-dy;

		// Send event to buttons...
		for(int i = children(); i--;) {
			Fl_Widget& o = *child(i);
			int mx = Fl::event_x() - o.x() - 2;
			int my = Fl::event_y() - o.y() - 2;
			if (mx >= 0 && mx < o.w() && my >= 0 && my < o.h()+4)
				if(child(i)->send(event)) {
					extern bool handle_title;
					handle_title = false;
					return 1;
				}
		}

		if(Fl::event_clicks() && Fl::event_button()==1) {
			// we don't want animation for dialogs and utils frames
			if(f->window_type() == TYPE_NORMAL && f->decor_flag(MAXIMIZE))
				f->cb_button_max(0);

			Fl::event_clicks(0);
			return 1;
		} else if (Fl::event_button()==3) {
			Titlebar::popup_menu(f);
			return 1;
		}

		return 1;
	}

	case FL_RELEASE: {
		if(Fl::event_state(FL_BUTTON1)) return 1;

		if(grabbed) {
			if(!Frame::do_opaque) {
				clear_overlay();
				f->set_size(nx, ny, f->w(), f->h());
				XUngrabServer(fl_display);
			}
			grab_cursor(false);
			grabbed = false;
		}

		if(root->get_cursor()==FL_CURSOR_MOVE)
			root->set_default_cursor();

		if (Fl::event_is_click()) {
			f->activate();
			f->raise();
		}
		return 1;
	}


	case FL_DRAG: {
		if(Fl::event_is_click()) return 1; // don't drag yet
		if(!Fl::event_state(FL_BUTTON1)) return 0;

		// Change to MOVE cursor
		if(root->get_cursor()!=FL_CURSOR_MOVE) {
			root->set_cursor(FL_CURSOR_MOVE, FL_WHITE, FL_BLACK);
		}

		// We need to grab server while moving,
		// since if underlying window redraws our overlay will fuck up...
		if(!grabbed) {
			if(!Frame::do_opaque) XGrabServer(fl_display);
			grab_cursor(true);
			grabbed=true;
		}

		nx = Fl::event_x_root()-dx;
		ny = Fl::event_y_root()-dy;
		f->handle_move(nx, ny);

		return 1;
	}

//	default:
//		return 1;
	}

	return 0;
}

