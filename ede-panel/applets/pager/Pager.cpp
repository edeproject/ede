#include "Applet.h"

#include <string.h>
#include <FL/Fl_Group.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <edelib/Debug.h>
#include <edelib/Netwm.h>

#include "PagerButton.h"

EDELIB_NS_USING(netwm_workspace_change)
EDELIB_NS_USING(netwm_workspace_get_current)
EDELIB_NS_USING(netwm_workspace_get_count)
EDELIB_NS_USING(netwm_workspace_get_names)
EDELIB_NS_USING(netwm_workspace_free_names)
EDELIB_NS_USING(netwm_callback_add)
EDELIB_NS_USING(netwm_callback_remove)
EDELIB_NS_USING(NETWM_CHANGED_CURRENT_WORKSPACE)

class Pager : public Fl_Group {
public:
	Pager();
	~Pager();
	void init_workspace_boxes(void);
	void workspace_changed(void);
};

static void box_cb(PagerButton *pb, void *p) {
	/* because workspaces in button labels are increased */
	int s = pb->get_workspace_label() - 1;
	netwm_workspace_change(s);
}

static void net_event_cb(int action, Window xid, void *data) {
	E_RETURN_IF_FAIL(data != NULL);

	if(action == NETWM_CHANGED_CURRENT_WORKSPACE) {
		Pager *p = (Pager*)data;
		p->workspace_changed();
	}
}

Pager::Pager() : Fl_Group(0, 0, 25, 25, NULL) { 
	box(FL_DOWN_BOX);
	/* we will explicitly add elements */
	end();

	init_workspace_boxes();
	netwm_callback_add(net_event_cb, this);
}

Pager::~Pager() {
	netwm_callback_remove(net_event_cb);
}

void Pager::init_workspace_boxes(void) {
	int X, Y, H;

	/* prepare the sizes */
	X = x() + Fl::box_dx(box());
	Y = y() + Fl::box_dy(box());
	H = h() - Fl::box_dh(box());

	int  nworkspaces, curr_workspace;
	char **names = 0;

	nworkspaces    = netwm_workspace_get_count();
	curr_workspace = netwm_workspace_get_current();
	netwm_workspace_get_names(names);

	/* 
	 * Resize group before childs, or they will be resized too, messing things up.
	 * Here, each child is 25px wide plus 1px for delimiter between the childs .
	 */
	int gw = nworkspaces * (25 + 1);
	gw += Fl::box_dw(box());
	/* last child do not ends with 1px wide delimiter */
	gw -= 1;

	size(gw, h());

	for(int i = 0; i < nworkspaces; i++) {
		/* let box width be fixed */
		PagerButton *bx = new PagerButton(X, Y, 25, H);
		bx->box(FL_FLAT_BOX);

		if(i == curr_workspace)
			bx->select_it(1);
		else
			bx->select_it(0);

		/* workspaces are started from 0 */
		bx->set_workspace_label(i + 1);

		if(names)
			bx->copy_tooltip(names[i]);

		bx->callback((Fl_Callback*)box_cb, this);

		add(bx);
		/* position for the next box */
		X = bx->x() + bx->w() + 1;
	}

	netwm_workspace_free_names(names);
}

void Pager::workspace_changed(void) {
	int c = netwm_workspace_get_current();
	E_RETURN_IF_FAIL(c < children());

	PagerButton *pb;

	for(int i = 0; i < children(); i++) {
		pb = (PagerButton*)child(i);
		pb->select_it(0);
	}

	pb = (PagerButton*)child(c);
	pb->select_it(1);
}

EDE_PANEL_APPLET_EXPORT (
 Pager, 
 EDE_PANEL_APPLET_OPTION_ALIGN_LEFT,
 "Desktop switcher",
 "0.1",
 "empty",
 "Sanel Zukan"
)
