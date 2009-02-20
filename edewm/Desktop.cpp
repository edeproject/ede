#include "config.h"

#include "debug.h"
#include "Winhints.h"
#include "Frame.h"
#include "Windowmanager.h"
#include "Desktop.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Desktop_List desktops;

int Desktop::desktop_count_ = 0;
Desktop* Desktop::current_ = 0;

Desktop *Desktop::desktop(int num)
{
    if(num>=(int)desktops.size()) return desktops.item(desktops.size()-1);
    if(num<1) return desktops.item(0);

    for(uint n=0; n<desktops.size(); n++) {
        Desktop *d=desktops[n];
        if(d->number()==num) return d;
    }
    return desktops.item(0);
}

Desktop *Desktop::next()
{
    uint num = current()->number();
    num++;
    if(num > desktops.count())
        current(0);
    else
        current(num);
    return current();
}

Desktop *Desktop::prev()
{
    int num = current()->number();
    num--;
    if(num < 1)
        current(desktops.count());
    else
        current(num);
    return current();
}

Desktop::Desktop(const char *name)
{
    desktops.append(this);
    number_ = desktops.count();

    DBG("::Desktop %d", number_);

    if(name)
        name_ = strdup(name);
    else
        name_ = 0;
}

Desktop::~Desktop()
{
    desktops.remove(this);
    if(current_==this) {
        current(desktop(number()+1));
    }

    // put any clients onto another desktop:
    for(uint n=0; n<map_order.size(); n++)
        if(map_order[n]->desktop()==this)
            map_order[n]->desktop(current_);

    if(name_) delete []name_;
}

void Desktop::name(const char *name)
{
    if(name_) delete []name_;
    if(name) name_ = strdup(name);
}

extern Frame *menu_frame; //Frame where titlebar menu is popped up. Set only when changing window desktop.
void Desktop::current(Desktop *cur)
{
    if(cur == current_) return;

    current_ = cur;

    Frame *f = 0, *top = 0;
    for(uint n=0; n<stack_order.size(); n++) {
        f = stack_order[n];
        if(!f->desktop() && f->state()!=UNMAPPED) {
            if(f->state()==OTHER_DESKTOP) f->state(NORMAL);
            if(!f->frame_flag(SKIP_FOCUS) && !f->frame_flag(SKIP_LIST)) top=f;
            continue;
        }

        // Keep this window in 'NORMAL' state.
        if(f==menu_frame && f->state()==NORMAL) {
            top=f;
            continue;
        }

        if(f->state()!=UNMAPPED) {
            if(f->desktop()==cur) {
                if(f->state()==OTHER_DESKTOP) {
                    f->state(NORMAL);
                }
                top=f;

            } else {

                if(f->state()==NORMAL) {
                    f->state(OTHER_DESKTOP);
                }
            }
        }
    }

    if(cur!=0) {
        setProperty(fl_xid(root), _XA_NET_CURRENT_DESKTOP, XA_CARDINAL, cur->number()-1);
        if(top) {
            top->activate();
            top->raise();
        }
        DBG("Went to desktop: %d", cur->number());
    }
}

Desktop* Desktop::add(const char *name)
{
    Desktop *d = new Desktop(name);
    if(!name) {
        char buf[32]; sprintf(buf, _("Workspace %d"), d->number());
        d->name(buf);
    }
    return d;
}

// Updates viewport info, since we dont support LARGE desktops
// According to NET-WM speac these MUST be set to (0,0)
void Desktop::update_desktop_viewport()
{
    CARD32 val[2] = { 0, 0 };
    XChangeProperty(fl_display, root_win, _XA_NET_DESKTOP_VIEWPORT, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *)&val, 2);
}

void Desktop::update_desktop_geometry()
{
    CARD32 val[2] = { Fl::w(), Fl::h() };
    XChangeProperty(fl_display, root_win, _XA_NET_DESKTOP_GEOMETRY, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&val, 2);

}

void Desktop::update_desktop_workarea()
{
    CARD32 val[4];
    val[0] = root->x();
    val[1] = root->y();
    val[2] = root->w();
    val[3] = root->h();

    XChangeProperty(fl_display, root_win, _XA_NET_WORKAREA, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&val, 4);
}

void Desktop::update_desktop_names(bool send)
{
    char *workspaces[16] = { 0 };
    int cnt=0;
    for(uint n=0; n<desktops.size(); n++) {
        Desktop *d=desktops[n];
        if(!d->name()) {
            char tmp[64]; snprintf(tmp, sizeof(tmp)-1, _("Workspace %d"), cnt+1);
            workspaces[cnt] = strdup(tmp);
        } else {
            workspaces[cnt] = strdup(d->name());
        }
        cnt++;
    }

    if(send) {
        XTextProperty names;
        if(XStringListToTextProperty((char **)workspaces, cnt, &names)) {
            XSetTextProperty(fl_display, fl_xid(root), &names, _XA_NET_DESKTOP_NAMES);
            XFree(names.value);
        }
    }
}

// Updates both NET desktop count atoms
void Desktop::update_desktop_count(uint cnt, bool send)
{
    if(cnt<0) cnt=0;
    if(cnt>8) cnt=8;

    Desktop::desktop_count_ = cnt;
    if(send) {
        setProperty(fl_xid(root), _XA_NET_NUM_DESKTOPS, XA_CARDINAL, Desktop::desktop_count_);

        update_desktop_viewport();
        update_desktop_workarea();
        update_desktop_geometry();
    }

    if(desktops.count()<cnt) {
        int add = cnt-desktops.count();
        for(int a=0; a<add; a++)
            Desktop::add();
        DBG("Increased desktops to %d", desktops.count());
    } else if(desktops.count()>cnt) {
        int del = desktops.count()-cnt;
        for(int a=0; a<del; a++) {
            Desktop *d = desktops.item(desktops.size()-1);
            delete d;
        }
        DBG("Decreased desktops to %d", desktops.count());
    }
}

void Desktop::set_names()
{
    int length=0;
    char *buffer=0;

    XTextProperty names;
    // First try to get NET desktop names
    XGetTextProperty(fl_display, RootWindow(fl_display, fl_screen), &names, _XA_NET_DESKTOP_NAMES);
    buffer = (char *)names.value;
    length = names.nitems;

    if(buffer) {
        char* c = buffer;
        for (int i = 1; c < buffer+length; i++) {
            char* d = c;
            while(*d) d++;
            Desktop::desktop(i)->name(c);
            c = d+1;
        }
        XFree(names.value);

    } else {
        // No desktop names in XServer already, read from ede config file
        Fl_Config cfg(fl_find_config_file("ede.conf", 0));
        cfg.set_section("Workspaces");
        char tmp[128], name[128];
        for(uint n=1; n<=8; n++) {
            snprintf(tmp, sizeof(tmp)-1, "Workspace%d", n);
            if(!cfg.read(tmp, name, 0, sizeof(name))) {
                if(n<=desktops.count()) {
                    Desktop::desktop(n)->name(name);
                }
            }
        }
    }
}

// called at startup, read the list of desktops from the root
// window properties, or on failure make some default desktops.
void init_desktops()
{
    int current_num=0, p=0;

    p = getIntProperty(fl_xid(root), _XA_NET_NUM_DESKTOPS, XA_CARDINAL, -1);
    if(p<0) {
        // No desktop count in XServer already, read from ede config file
        Fl_Config cfg(fl_find_config_file("ede.conf", 0));
        cfg.get("Workspaces", "Count", p, 4);
    }

    DBG("Total Desks: %d", p);

    Desktop::update_desktop_count(p);
    Desktop::set_names();
    Desktop::update_desktop_names(true);

    // Try to get current NET desktop
    p = getIntProperty(fl_xid(root), _XA_NET_CURRENT_DESKTOP, XA_CARDINAL, -1);
    // If not valid number, try GNOME
    if(p >= 0 && p < Desktop::desktop_count()+1)
        current_num = p+1; // Got valid
    else
        current_num = 1;   // Not valid, use 1

    DBG("Current desktop: %d", current_num);
    Desktop::current(Desktop::desktop(current_num));

    DBG("Desktops inited!");
}

