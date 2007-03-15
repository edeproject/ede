#include "item.h"
#include "mainmenu.h"

void layout_menu(EItemGroup *g, void *) {
    g->add_items();
}

EItemGroup::EItemGroup(MainMenu *menu, int type, const char *name)
: fltk::Item_Group(name)
{
    m_modified = 0;
    m_menu = menu;
    m_gtype = type;
    about_to_show = (fltk::Callback*)layout_menu;
    m_access = true;
}

void EItemGroup::add_items()
{
    struct stat s;
    if(lstat(dir(), &s) == 0) {
        if(!m_modified) {
            m_modified = s.st_mtime;
        }
        if(m_modified != s.st_mtime) {
            //dir has changed..
            m_modified = s.st_mtime;
            clear();
        }
    }

    if(!children() && access()) {
        begin();
        if(group_type()==BROWSER_GROUP)
            menu()->scan_filebrowser(dir());
        else if(group_type()==APP_GROUP)
            menu()->scan_programitems(dir());
        end();
    }
}

static fltk::Menu_Button *popupMenu=0;
static const char *dir = 0;

void cb_menu(fltk::Widget *wid, long user_data)
{
	if(!dir) return;
	char cmd[FL_PATH_MAX];

	EDE_Config pGlobalConfig(find_config_file("ede.conf", false));

    // we can't use Fl_String here, because gcc3.2.3 bug, so we will use
    // plain char with stupid FL_PATH_MAX
	switch(user_data) {
	case 1: {
		char term[FL_PATH_MAX];
		pGlobalConfig.get("Terminal", "Terminal", term, 0, sizeof(term));
		if(pGlobalConfig.error() && !term[0] || (strlen(term) == 0)) 
			strncpy(term, "xterm", sizeof(term));

		snprintf(cmd, sizeof(cmd)-1, "cd %s; %s\n", dir, term);
	}
	break;

	case 2: {
		char browser[FL_PATH_MAX];
		pGlobalConfig.get("Web", "Browser", browser, 0, sizeof(browser));
		if(pGlobalConfig.error() && !browser[0] || (strlen(browser) == 0)) 
			strncpy(browser, "mozilla", sizeof(browser));
        
		snprintf(cmd, sizeof(cmd)-1, "%s %s\n", browser, dir);
	}
	break;

	case 0:
		fltk::exit_modal();
	default:
		return;
	}

	fltk::start_child_process(cmd, false);
}

int popup_menu()
{
    if(!popupMenu) {
        popupMenu = new fltk::Menu_Button(0,0,0,0,0);
        popupMenu->parent(0);
        popupMenu->type(fltk::Menu_Button::POPUP3);

        popupMenu->add(_("Open with terminal..."),0,(fltk::Callback*)cb_menu,(void*)1);
        popupMenu->add(_("Open with browser..."),0,(fltk::Callback*)cb_menu,(void*)2);
        popupMenu->add(new fltk::Menu_Divider());
        popupMenu->add(_("Close Menu"),0,(fltk::Callback*)cb_menu,(void*)0);
    }
    return popupMenu->popup();
}

int EItemGroup::handle(int event)
{
    if(event == FL_RELEASE) {
        if( fltk::event_button() == 3) {
            ::dir = this->dir();
            int ret = popup_menu();
            ::dir = 0;
            if(ret) return 0;
        }
        return 1;
    }
    return fltk::Item_Group::handle(event);
}

int EItem::handle(int event)
{
    if(event==FL_RELEASE) {
        if(type()==FILE) {
            if(fltk::event_button() == 3) {
                ::dir = this->dir();
                int ret = popup_menu();
                ::dir = 0;
                return 1;
            }
            if(((EItemGroup*)parent())->group_type()==BROWSER_GROUP) return 1;
        }
    }
    return fltk::Item::handle(event);
}
