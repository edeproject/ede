#ifndef _DOCK_H_
#define _DOCK_H_

#include <efltk/Fl_Window.h>
#include <efltk/Fl_Group.h>
#include <efltk/x.h>
//#include <vector>

/*using namespace std;


struct trayitem
{
	Window child;
	Window container;
};

typedef vector<trayitem> Tray;*/

class Dock : public Fl_Group
{
public:
	Dock();
	~Dock();
	
	void add_to_tray(Fl_Widget *w);
	void remove_from_tray(Fl_Widget *w);
	static int handle_x11_event(int e);
	void embed_window(Window id);
	void register_notification_area();

	int handle(int event);

	Atom opcode;
	Atom message_data;
	Atom winstate;
//    Tray tray;
private:
//    int get_free_item();

};

#endif
