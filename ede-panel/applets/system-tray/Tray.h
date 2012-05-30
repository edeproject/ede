#ifndef __TRAY_H__
#define __TRAY_H__

#include <FL/Fl_Group.H>
#include <FL/x.H>
#include <edelib/List.h>
#include "Applet.h"

EDELIB_NS_USING(list)

struct WinInfo {
	Window    id;
	Fl_Window *win;
};

typedef list<WinInfo> WinList;
typedef list<WinInfo>::iterator WinListIt;

class Tray : public Fl_Group {
private:
	Atom    opcode, message_data;
	WinList win_list;
	void distribute_children(void);
public:
	Tray();
	~Tray();
	void register_notification_area(void);
	Atom get_opcode(void) const { return opcode; }
	void embed_window(Window id);
	void unembed_window(Window id);
	void configure_notify(Window id);

	void add_to_tray(Fl_Widget *w);
	void remove_from_tray(Fl_Widget *w);

	int handle(int e);
};

#endif
