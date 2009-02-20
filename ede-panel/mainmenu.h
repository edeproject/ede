#ifndef _MAINMENU_H_
#define _MAINMENU_H_

#include <efltk/xml/Fl_Xml.h>

#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_WM.h>
#include <efltk/Fl_Pixmap.h>
#include <efltk/filename.h>
#include <efltk/fl_draw.h>
#include <efltk/fl_ask.h>

#include <sys/stat.h>

#include "item.h"

class MainMenu : public Fl_Menu_Button
{
public:
    MainMenu();
    ~MainMenu();

    int popup(int X, int Y, int W, int H);

    void draw();
    int handle(int event);
    void layout();

    void init_entries();

    Fl_String get_item_dir(Fl_XmlNode *node);
    Fl_String get_item_name(Fl_XmlNode *node);

    void set_exec(EItem *i, const Fl_String &exec);
    void build_menu_item(Fl_XmlNode *node);

    int calculate_height() const;

    Fl_Image *find_image(const Fl_String &icon) const;

    void scan_programitems(const char *path);
    void scan_filebrowser(const Fl_String &path);

    const Fl_String &locale() const { return m_locale; }

    static void resolve_program(Fl_String cmd);
    static void clear_favourites();
    
    bool is_open() { return m_open; }

private:
    static inline void cb_exec_item(EItem *i, void *d) { i->menu()->resolve_program(i->exec()); }

    Fl_Image *e_image;

    Fl_String m_locale;
    long m_modified;
    bool m_open;
};

#endif
