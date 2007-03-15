#ifndef _MAINMENU_H_
#define _MAINMENU_H_

/*#include <efltk/xml/Fl_Xml.h>

#include <efltk/Fl_Menu_Button.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_WM.h>
#include <efltk/Fl_Pixmap.h>
#include <efltk/filename.h>
#include <efltk/fl_draw.h>
#include <efltk/fl_ask.h>*/

#include <efltk/xml/Fl_Xml.h>

#include <fltk/Menu_Button.h>
#include "EDE_Config.h"
#include <fltk/Divider.h>
#include <fltk/WM.h>
#include <fltk/Pixmap.h>
#include <fltk/filename.h>
#include <fltk/draw.h>
#include <fltk/ask.h>

#include <sys/stat.h>

#include "item.h"

class MainMenu : public fltk::Menu_Button
{
public:
    MainMenu();
    ~MainMenu();

    int popup(int X, int Y, int W, int H);

    void draw();
    void layout();

    void init_entries();

    char* get_item_dir(fltk::XmlNode *node);
    char* get_item_name(fltk::XmlNode *node);

    void set_exec(EItem *i, const char* exec);
    void build_menu_item(fltk::XmlNode *node);

    int calculate_height() const;

    fltk::Image *find_image(const char* icon) const;

    void scan_programitems(const char *path);
    void scan_filebrowser(const char* path);

    const char* locale() const { return m_locale; }

    static void resolve_program(char* cmd);
    static void clear_favourites();
    
    bool is_open() { return m_open; }

private:
    static inline void cb_exec_item(EItem *i, void *d) { i->menu()->resolve_program(i->exec()); }

    fltk::Image *e_image;

    char* m_locale;
    long m_modified;
    bool m_open;
};

#endif
