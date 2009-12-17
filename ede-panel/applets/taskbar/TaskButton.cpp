#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/MenuItem.h>
#include <edelib/IconLoader.h>
#include <edelib/Netwm.h>

#include "TaskButton.h"
#include "Taskbar.h"
#include "icons/window.xpm"

EDELIB_NS_USING(MenuItem)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(ICON_SIZE_TINY)
EDELIB_NS_USING(netwm_window_close)
EDELIB_NS_USING(netwm_window_set_active)
EDELIB_NS_USING(netwm_window_maximize)
EDELIB_NS_USING(netwm_window_get_title)
EDELIB_NS_USING(wm_window_ede_restore)
EDELIB_NS_USING(wm_window_get_state)
EDELIB_NS_USING(wm_window_set_state)
EDELIB_NS_USING(WM_WINDOW_STATE_ICONIC)

static Fl_Pixmap image_window(window_xpm);

static void close_cb(Fl_Widget*, void*);
static void restore_cb(Fl_Widget*, void*);
static void minimize_cb(Fl_Widget*, void*);
static void maximize_cb(Fl_Widget*, void*);

static MenuItem menu_[] = {
	{_("Restore"), 0, restore_cb, 0},
	{_("Minimize"), 0, minimize_cb, 0},
	{_("Maximize"), 0, maximize_cb, 0, FL_MENU_DIVIDER},
	{_("Close"), 0, close_cb, 0},
	{0}
};

static void redraw_whole_panel(TaskButton *b) {
	/* Taskbar specific member */
	Taskbar *tb = (Taskbar*)b->parent();
	tb->panel_redraw();
}

static void close_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;
	netwm_window_close(bb->get_window_xid());
	/* no need to redraw whole panel since taskbar elements are recreated again */
}

static void restore_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;
	wm_window_ede_restore(bb->get_window_xid());

	netwm_window_set_active(bb->get_window_xid());
	redraw_whole_panel(bb);
}

static void minimize_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;

	if(wm_window_get_state(bb->get_window_xid()) != WM_WINDOW_STATE_ICONIC)
		wm_window_set_state(bb->get_window_xid(), WM_WINDOW_STATE_ICONIC);

	redraw_whole_panel(bb);
}

static void maximize_cb(Fl_Widget*, void *b) {
	TaskButton *bb = (TaskButton*)b;

	netwm_window_set_active(bb->get_window_xid());
	netwm_window_maximize(bb->get_window_xid());

	redraw_whole_panel(bb);
}

TaskButton::TaskButton(int X, int Y, int W, int H, const char *l) : Fl_Button(X, Y, W, H, l), xid(0) { 
	box(FL_UP_BOX);
	align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_CLIP);

	if(IconLoader::inited()) {
		Fl_Shared_Image *img = IconLoader::get("process-stop", ICON_SIZE_TINY);
		menu_[3].image((Fl_Image*)img);
	}

	image(image_window);
}

void TaskButton::draw(void) {
	Fl_Color col = value() ? selection_color() : color();
	draw_box(value() ? (down_box() ? down_box() : fl_down(box())) : box(), col);

	if(image()) {
		int X, Y, lw, lh;

		X = x() + 5;
		Y = (y() + h() / 2) - (image()->h() / 2);
		image()->draw(X, Y);

		X += image()->w() + 5;

		if(label()) {
			fl_font(labelfont(), labelsize());
			fl_color(labelcolor());

			lw = lh = 0;
			fl_measure(label(), lw, lh, align());

			/* use clipping so long labels do not be drawn on the right border, which looks ugly */
			fl_push_clip(x() + Fl::box_dx(box()), 
						 y() + Fl::box_dy(box()), 
						 w() - Fl::box_dw(box()) - 5, 
						 h() - Fl::box_dh(box()));

				Y = (y() + h() / 2) - (lh / 2);
				fl_draw(label(), X, Y, lw, lh, align(), 0, 0);

			fl_pop_clip();
		}
	} else {
		draw_label();
	}

	if(Fl::focus() == this)
		draw_focus();
}

void TaskButton::display_menu(void) {
	const char *t = tooltip();

	/* do not popup tooltip when the menu is on */
	tooltip(NULL);

	/* parameters for callbacks; this is done here, since menu_ is static and shared between buttons */
	menu_[0].user_data(this);
	menu_[1].user_data(this);
	menu_[2].user_data(this);
	menu_[3].user_data(this);

	const MenuItem *item = menu_->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
	if(item && item->callback())
		item->do_callback(this);

	tooltip(t);
}

void TaskButton::update_title_from_xid(void) {
	E_RETURN_IF_FAIL(xid >= 0);

	char *title = netwm_window_get_title(xid);
	if(!title) {
		label("...");
		tooltip("...");
	} else {
		copy_label(title);
		tooltip(label());
		free(title);
	}
}
