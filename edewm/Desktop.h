/*
 * $Id: Desktop.h 1653 2006-06-09 13:08:58Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <efltk/Fl_PtrList.h>

class Desktop {
public:
    Desktop(const char *name);
    ~Desktop();

    const char* name() const { return name_; }
    void name(const char *name);
    int number() const { return number_; }

    static Desktop *desktop(int num);

    static Desktop *next();
    static Desktop *prev();

    static Desktop *current() { return current_; }
    static int current_num()  { return current_ ? current_->number() : -1; }

    static Desktop* add(const char *name=0);

    static void current(Desktop *cur);
    static void current(int cur) { current(desktop(cur)); }

    static int desktop_count() { return desktop_count_; }

    static void update_desktop_viewport();
    static void update_desktop_workarea();
    static void update_desktop_geometry();

    static void update_desktop_count(uint cnt, bool send=true);
    static void update_desktop_names(bool send=true);

    static void set_names();

    int junk; // for temporary storage by menu builder

private:
    static Desktop* current_;
    static int desktop_count_;

    const char* name_;
    int number_;
};

class Desktop_List : public Fl_Ptr_List {
public:
    Desktop_List() : Fl_Ptr_List() { }

    void append(Desktop *item) { Fl_Ptr_List::append((void *)item); }
    void prepend(Desktop *item) { Fl_Ptr_List::prepend((void *)item); }
    void insert(uint pos, Desktop *item) { Fl_Ptr_List::insert(pos, (void *)item); }
    void replace(uint pos, Desktop *item) { Fl_Ptr_List::replace(pos, (void *)item); }
    void remove(uint pos) { Fl_Ptr_List::remove(pos); }
    bool remove(Desktop *item) { return Fl_Ptr_List::remove((void *)item); }
    int index_of(const Desktop *w) const { return Fl_Ptr_List::index_of((void*)w); }
    Desktop *item(uint index) const { return (Desktop*)Fl_Ptr_List::item(index); }

    Desktop **data() { return (Desktop**)items; }

    Desktop *operator [](uint ind) const { return (Desktop *)items[ind]; }
};

extern Desktop_List desktops;

#endif
