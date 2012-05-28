#ifndef __MEMMONITOR_H__
#define __MEMMONITOR_H__

#include <FL/Fl_Box.H>
#include "Applet.h"

EDE_PANEL_APPLET_CLASS(MemMonitor, Fl_Box) {
private:
	int mem_usedp, swap_usedp;
	
public:
	MemMonitor();
	void update_status(void);
	void draw(void);
	int  handle(int e);
};

#endif
