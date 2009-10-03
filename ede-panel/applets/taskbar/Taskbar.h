#ifndef __TASKBAR_H__
#define __TASKBAR_H__

#include <FL/Fl_Group.H>

class TaskButton;
class Panel;

class Taskbar : public Fl_Group {
public:
	TaskButton *curr_active, *prev_active;
	Panel      *panel;

public:
	Taskbar();
	~Taskbar();

	void create_task_buttons(void);

	void resize(int X, int Y, int W, int H);
	void layout_children(void);

	void update_active_button(int xid = -1);
	void activate_window(TaskButton *b);
	void update_child_title(Window xid);

	void panel_redraw(void);
};

#endif
