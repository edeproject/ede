#ifndef _TASKBUTTON_H_
#define _TASKBUTTON_H_

#include <efltk/Fl_Button.h>
#include <efltk/Fl_Menu_.h>
#include <efltk/Fl_WM.h>

class TaskBar;

// One task in taskabr
class TaskButton : public Fl_Button
{
public:
    // TashBar is needed for this pointer, so we can call update()
    TaskButton(Window w);

    // Menu stuff
    static Fl_Menu_ *menu;
    static TaskButton *pushed;

    void draw();

    // Speed up drawing by keeping the minimized state
    bool m_minimized;
};

/////////////////////////////////////

class TaskBar : public Fl_Group
{
public:
    TaskBar();

    // Active window ID
    static Window active;

    //Are buttons variable width
    static bool variable_width;

    // Windows from all desktops
    static bool all_desktops;

    void layout();

    void update();
    void update_name(Window win);
    void update_active(Window active);

    int max_taskwidth() const { return m_max_taskwidth; }
    
    void minimize_all();

private:
    void add_new_task(Window w);

    int m_max_taskwidth;
};

#endif
