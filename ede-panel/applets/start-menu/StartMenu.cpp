#include "Applet.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <edelib/MenuBase.h>
#include <edelib/Debug.h>
#include <edelib/Nls.h>

#include "XdgMenuReader.h"
#include "ede-icon.h"

EDELIB_NS_USING(MenuBase)

/* some of this code was ripped from Fl_Menu_Button.cxx */

EDE_PANEL_APPLET_CLASS(StartMenu, MenuBase) {
private:
	MenuItem *mcontent;
public:
	StartMenu();
	~StartMenu();

	void popup(void);
	void draw(void);
	int  handle(int e);
};

StartMenu::StartMenu() : MenuBase(0, 0, 80, 25, "EDE"), mcontent(NULL) {
	down_box(FL_NO_BOX);
	labelfont(FL_HELVETICA_BOLD);
	labelsize(14);
	image(ede_icon_image);

	tooltip(_("Click here to choose and start common programs"));

	mcontent = xdg_menu_load();

	if(mcontent) {
		/* skip the first item, since it often contains only one submenu */
		if(mcontent->submenu()) {
			MenuItem *mc = mcontent + 1;
			menu(mc);
		} else {
			menu(mcontent);
		}
	}
}

StartMenu::~StartMenu() {
	xdg_menu_delete(mcontent);
}

static StartMenu *pressed_menu_button = 0;

void StartMenu::draw(void) {
	if(!box() || type()) 
		return;

	draw_box(pressed_menu_button == this ? fl_down(box()) : box(), color());

	if(image()) {
		int X, Y, lw, lh;

		X = x() + 5;
		Y = (y() + h() / 2) - (image()->h() / 2);
		image()->draw(X, Y);

		X += image()->w() + 10;

		fl_font(labelfont(), labelsize());
		fl_color(labelcolor());
		fl_measure(label(), lw, lh, align());

		fl_draw(label(), X, Y, lw, lh, align(), 0, 0);
	} else {
		draw_label();
	}
}

void StartMenu::popup(void) {
	const MenuItem *m;

	pressed_menu_button = this;
	redraw();

	Fl_Widget *mb = this;
	Fl::watch_widget_pointer(mb);
	if(!box() || type())
		m = menu()->popup(Fl::event_x(), Fl::event_y(), label(), mvalue(), this);
	else
		m = menu()->pulldown(x(), y(), w(), h(), 0, this);

	picked(m);
	pressed_menu_button = 0;
	Fl::release_widget_pointer(mb);
}

int StartMenu::handle(int e) {
	if(!menu() || !menu()->text)
		return 0;

	switch(e) {
		case FL_ENTER:
		case FL_LEAVE:
			return (box() && !type()) ? 1 : 0;
		case FL_PUSH:
			if(!box()) {
				if(Fl::event_button() != 3) 
					return 0;
			} else if(type()) {
				if(!(type() & (1 << (Fl::event_button() - 1))))
					return 0;
			}

			if(Fl::visible_focus()) 
				Fl::focus(this);

			popup();
			return 1;

		case FL_KEYBOARD:
			if(!box()) return 0;

			/* 
			 * Win key will show the menu.
			 *
			 * In FLTK, Meta keys are equivalent to Alt keys. On other hand, eFLTK has them different, much
			 * the same as on M$ Windows. Here, I'm explicitly using hex codes, since Fl_Meta_L and Fl_Meta_R
			 * are unusable.
			 */
			if(Fl::event_key() == 0xffeb || Fl::event_key() == 0xffec) {
				popup();
				return 1;
			}

			return 0;

		case FL_SHORTCUT:
			if(Fl_Widget::test_shortcut()) { 
				popup();
				return 1;
			}

			return test_shortcut() != 0;

		case FL_FOCUS:
		case FL_UNFOCUS:
			if(box() && Fl::visible_focus()) {
				redraw();
				return 1;
			}

		default:
			return 0;
	}

	/* not reached */
	return 0;
}

EDE_PANEL_APPLET_EXPORT (
 StartMenu, 
 EDE_PANEL_APPLET_OPTION_ALIGN_LEFT,
 "Main menu",
 "0.1",
 "empty",
 "Sanel Zukan"
)
