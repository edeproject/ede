/*
 * $Id: Windowmanager.h 1711 2006-07-25 09:51:50Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef _WINDOWMANAGER_H_
#define _WINDOWMANAGER_H_

#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/x.h>
#include <efltk/fl_draw.h>
#include <efltk/filename.h>
#include <efltk/Fl_Ptr_List.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Locale.h>

#include "Cursor.h"

#ifdef _DEBUG
#include <map>
#endif

#include <list>

/*
class Frame_List : public Fl_Ptr_List {
public:
    Frame_List() : Fl_Ptr_List() { }

    void append(Frame *item) { Fl_Ptr_List::append((void *)item); }
    void prepend(Frame *item) { Fl_Ptr_List::prepend((void *)item); }
    void insert(uint pos, Frame *item) { Fl_Ptr_List::insert(pos, (void *)item); }
    void replace(uint pos, Frame *item) { Fl_Ptr_List::replace(pos, (void *)item); }
    void remove(uint pos) { Fl_Ptr_List::remove(pos); }
    bool remove(Frame *item) { return Fl_Ptr_List::remove((void *)item); }
    int index_of(const Frame *w) const { return Fl_Ptr_List::index_of((void*)w); }
    Frame *item(uint index) const { return (Frame*)Fl_Ptr_List::item(index); }

    Frame **data() { return (Frame**)items; }

    Frame *operator [](uint ind) const { return (Frame *)items[ind]; }
};
*/

struct WindowManagerConfig
{
	Fl_Color title_active_color; 
	Fl_Color title_active_color_text;
	Fl_Color title_normal_color; 
	Fl_Color title_normal_color_text;

	int      title_label_align;
	int      title_height;
	int      title_box_type;

	bool     frame_do_opaque;
	bool     frame_animate;
	int      frame_animate_speed;

	bool     use_theme;
};


// The WindowManager class looks like a window to efltk but is actually the
// screen's root window.  This is done by setting xid to "show" it
// rather than have efltk create the window. Class handles all root
// windows X events

struct Hints;
class Frame;
class CursorHandler;
class SoundSystem;
typedef std::list<Frame*> FrameList;

class WindowManager : public Fl_Window
{
	private:
		bool is_running;
		static WindowManager* pinstance;
		WindowManagerConfig* wm_conf;
		Fl_Rect wm_area;
		Window root_win;
		Hints* hint_stuff;

		CursorHandler* cur;
		SoundSystem* sound_system;

		bool app_starting;

		WindowManager();
		~WindowManager();
		WindowManager(const WindowManager&);
		WindowManager& operator=(WindowManager&);
		void register_protocols(void);
		void init_internals(void);
		void init_clients(void);

	public:
		static void init(int argc, char** argv);
		static void shutdown(void);
		static WindowManager* instance();
		void read_configuration(void);
		void read_xset_configuration(void);
		void notify_clients(void);
		void show(void);
		void hide(void) { } // prevent efltk from root window hiding
		void draw(void);
		int handle(int event);
		int handle(XEvent* e);
		void update_client_list(void);
		bool running(void)       { return is_running; }
		Window root_window(void) { return root_win;   }
		void idle(void);
		Hints* hints(void)       { return hint_stuff; }
		void exit(void);
		int x(void)              { return wm_area.x(); }
		int y(void)              { return wm_area.y(); }
		int w(void)              { return wm_area.w(); }
		int h(void)              { return wm_area.h(); }

		bool query_best_position(int* x, int* y, int w, int h);
		Frame* find_xid(Window win);
		void restack_windows(void);
		void clear_focus_windows(void);

		void play_sound(short event);

		const Cursor         root_cursor(void);
		const CursorHandler* cursor_handler(void);
		void  set_cursor(Frame* f, CursorType t);

		//FrameList<Frame*> window_list;
		//FrameList<Frame*> remove_list;

		// list of mapped windows (it is not changed excep window is destroyed)
		FrameList window_list;
		// stacking list of ordinary windows
		FrameList stack_list;
		// list of destroyed windows (cleared in WindowManager::idle() phase)
		FrameList remove_list;
		// list of always on top windows (transients etc.)
		FrameList aot_list;
	
	
#ifdef _DEBUG
		std::map<Atom, const char*> atom_map;
		std::map<int,  const char*> xevent_map;
		std::map<int,  const char*> efltkevent_map;

		void register_events(void);
#endif
};


bool ValidateDrawable(Drawable d);

#endif
