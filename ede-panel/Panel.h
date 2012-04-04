#ifndef __PANEL_H__
#define __PANEL_H__

#include <edelib/Window.h>
#include "AppletManager.h"

#define EDE_PANEL_CAST_TO_PANEL(obj)   ((Panel*)(obj))
#define EDE_PANEL_GET_PANEL_OBJECT(w)  (EDE_PANEL_CAST_TO_PANEL(w->parent()))

EDELIB_NS_USING_AS(Window, PanelWindow)

enum {
	PANEL_POSITION_TOP,
	PANEL_POSITION_BOTTOM
};

class Panel : public PanelWindow {
private:
	Fl_Widget *clicked;
	int        vpos;
	int        sx, sy;
	int        screen_x, screen_y, screen_w, screen_h, screen_h_half;
	int        width_perc; /* user specified panel width */
	bool       can_move_widgets;

	AppletManager mgr;

	void read_config(void);
	void save_config(void);
	void do_layout(void);

public:
	Panel();

	void show(void);
	void hide(void);
	int  handle(int e);
	void load_applets(void);

	int panel_w(void) { return w(); }
	int panel_h(void) { return h(); }

	void relayout(void) { do_layout(); }
};

#endif
