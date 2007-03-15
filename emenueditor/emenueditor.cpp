// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include <efltk/Fl_Util.h>
#include <efltk/Fl_Main_Window.h>
#include <efltk/Fl_Images.h>
#include <edeconf.h>

#include "icons/up.xpm"
#include "icons/folder.xpm"
#include "icons/item.xpm"

#include "emenueditor.h"

static Fl_Image item_pix = *Fl_Image::read_xpm(0, (const char**)item_xpm);
static Fl_Image folder_pix = *Fl_Image::read_xpm(0, (const char**)folder_xpm);

Fl_Window 	*edit_window = 0;
Fl_FileBrowser  *programs_browser;
Fl_Input        *filename_field, *name_field, *command_field, *icon_field;
int              selected, submenu_selected = 0;
Fl_Input        *filename_field_e, *name_field_e, *command_field_e,*icon_field_e;

void            cb_change_dir(Fl_Widget *, void*);
void            cb_directory_up(Fl_Button *, void*);
void            cb_new_submenu(Fl_Button *, void*);
void            cb_delete_submenu(Fl_Button *, void*);
void            cb_new_item(Fl_Button *, void*);
void            cb_delete_item(Fl_Button *, void*);
void            cb_edit_item(Fl_Button *, void*);
void            cb_about_menu_editor(Fl_Widget*, void*);
void            cbCloseWindow(Fl_Widget*, Fl_Window*);
void            Exit_Editor(Fl_Widget*, void*);
int             SomethingInDir(char *);

void cb_browse(Fl_Widget *, Fl_Input *input)
{
    char *file_types = _("Executables (*.*), *, All files (*.*), *");
    const char *f = fl_select_file(input->value(), file_types, _("File selection ..."));
    if (f) input->value(f);
}


int main(int argc, char **argv)
{
    Fl_String m_programsdir = fl_homedir() + "/.ede/programs";
    fl_init_locale_support("emenueditor", PREFIX"/share/locale");
    fl_init_images_lib();

    Fl_Main_Window *menu_edit_window = new Fl_Main_Window(480, 370, _("Menu editor"));
    
    Fl_Menu_Bar *menubar = new Fl_Menu_Bar(0, 0, 480, 25);
    menubar->begin();
    Fl_Item_Group *file = new Fl_Item_Group(_("&File"));
    Fl_Item *quit_item = new Fl_Item(_("&Quit"));
    quit_item->shortcut(0x40071);
    quit_item->x_offset(18);
    quit_item->callback(Exit_Editor, menu_edit_window);
    
    file->end();
    menubar->end();

        programs_browser = new Fl_FileBrowser(5, 40, 275, 313, _("Programs:"));
        programs_browser->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
        programs_browser->tooltip(_("Click on the submenu or on the item you want"));
        programs_browser->callback(cb_change_dir);
	programs_browser->end();
	programs_browser->directory(m_programsdir);

        Fl_Button *new_submenu = new Fl_Button(315, 50, 125, 23, _("New submenu"));
        new_submenu->callback( (Fl_Callback*) cb_new_submenu );

        Fl_Button *del_submenu =  new Fl_Button(315, 80, 125, 23, _("Delete submenu"));
        del_submenu->callback( (Fl_Callback*) cb_delete_submenu );

        Fl_Button *new_item = new Fl_Button(315, 125, 125, 23, _("New item"));
        new_item->callback( (Fl_Callback*) cb_new_item );

        Fl_Button *del_item = new Fl_Button(315, 185, 125, 23, _("Delete item"));
        del_item->callback( (Fl_Callback*) cb_delete_item );

        Fl_Button *edit_item = new Fl_Button(315, 155, 125, 23, _("Edit Item"));
        edit_item->callback( (Fl_Callback*) cb_edit_item );
    
    menu_edit_window->menu(menubar);
    menu_edit_window->resizable(menu_edit_window);
    menu_edit_window->end();
    menu_edit_window->show();

    Fl::run();

    if(edit_window)
        delete edit_window;

    return 0;
}


char* get_localized_name(char *cfg)
{
    char *icon_name = 0;
    Fl_Config iconConfig(cfg);
    iconConfig.set_section("Desktop Entry");

    char *alocale = strdup(setlocale(LC_ALL, NULL));
    char *tmp = strrchr(alocale, '_');
    if(tmp)
    {
        *tmp = '\0';
    }
    char localName[1024];
    snprintf(localName, sizeof(localName)-1, "Name[%s]", alocale);
    iconConfig.read((const char *)localName, icon_name);
    delete [] alocale;

    if (!icon_name)
    {
        iconConfig.read("Name", icon_name, "None");
    }

    return icon_name;

}

char* get_localized_string()
{
    char *localname = 0;
    char *alocale = strdup(setlocale(LC_MESSAGES, NULL));
//          -- language_country is perfectly valid according to FD.o
/*    char *tmp = strrchr(alocale, '_');
    if(tmp)
    {
        *tmp = '\0';
    } */
    localname = fl_strdup_printf("Name[%s]", alocale);
    delete [] alocale;

    if (!localname) localname = strdup("Name");
    return localname;
}


void cb_save_item_e(Fl_Widget *, Fl_Window *w)
{
    Fl_String item; 
    Fl_String dir = programs_browser->directory();

    Fl_String name = name_field_e->value();
    if (name.empty())
    {
        fl_alert(_("Please, enter the name of the menu item."));
        return;
    }

    Fl_String filename = filename_field_e->value();
    if (!filename.empty())
	item = filename;
    else
	item = name + ".desktop";

    Fl_String path_and_item = dir + slash + item;
    char *lname = get_localized_string();

    Fl_Config flconfig(path_and_item);
    flconfig.set_section("Desktop Entry");
    flconfig.write(lname, name_field_e->value());
    flconfig.write("Name", name_field_e->value());	// fallback
    flconfig.write("Exec", command_field_e->value());
    flconfig.write("Icon", fl_file_filename(icon_field_e->value()));

    delete [] lname;

    programs_browser->directory(dir);
    programs_browser->relayout();
    w->hide();
}


void cb_browse_icon(Fl_Widget *, Fl_Input *input)
{
    char *file_types = _("Icons (*.png), *.png, All files (*.*), *");
    const char *f = fl_select_file(PREFIX"/share/ede/icons/16x16", file_types, _("Choose icon file..."));
    if (f)
    {
        input->value(f);
    }
}


void Menu_Edit_Dialog(int edit)
{
    if(!edit_window) 
    {
        edit_window = new Fl_Window(370, 250, _("Edit item"));

        filename_field_e = new Fl_Output(5, 25, 195, 23, _("Filename:"));
        filename_field_e->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

        name_field_e = new Fl_Input(5, 80, 195, 23, _("Name in the menu:"));
        name_field_e->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

        command_field_e = new Fl_Input(5, 125, 195, 23, _("Command to execute:"));
        command_field_e->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

        {
            Fl_Button *browse_button_c = new Fl_Button(210, 125, 80, 23, _("Bro&wse..."));
            browse_button_c->callback( (Fl_Callback*) cb_browse, command_field_e );
        }

        icon_field_e = new Fl_Input(5, 215, 195, 23, _("Icon filename:"));
        icon_field_e->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

        {
            Fl_Button *save_button = new Fl_Button(285, 25, 80, 23, _("&Save"));
            save_button->callback( (Fl_Callback*) cb_save_item_e, edit_window );
        }
        {
            Fl_Button *cancel_button = new Fl_Button(285, 60, 80, 23, _("&Cancel"));
            cancel_button->callback( (Fl_Callback*) cbCloseWindow, edit_window );
        }
        {
            Fl_Button *browse_button_i = new Fl_Button(210, 215, 80, 23, _("&Browse..."));
            browse_button_i->callback( (Fl_Callback*) cb_browse_icon, icon_field_e );
        }
    }

    filename_field_e->value("");
    command_field_e->value("xterm");
    name_field_e->value(_("New folder"));
    icon_field_e->value("item.png");

    if (edit)
    {
        Fl_String c_file = programs_browser->filename_full();
	Fl_String name = programs_browser->filename();
	
	if (!name.empty())
	{
	    char *this_value = 0;
	    filename_field_e->value(name);
	    
            const char *tfield = filename_field_e->value();

            Fl_Config flconfig(c_file);
            flconfig.set_section("Desktop Entry");

	    this_value = get_localized_name(c_file);
            if(!flconfig.error() && this_value)
	    {
    	        name_field_e->value(this_value);
        	delete [] this_value;
            }
	    flconfig.read("Exec", this_value);
    	    if(!flconfig.error() && this_value)
    	    {
        	command_field_e->value(this_value);
        	delete [] this_value;
    	    }
    	    flconfig.read("Icon", this_value);
    	    if (!flconfig.error() && this_value)
    	    {
        	icon_field_e->value(this_value);
        	delete [] this_value;
    	    }
	}    
    }

    edit_window->end();
    edit_window->exec();
}

void cb_new_submenu(Fl_Button *, void *)
{
    Fl_String m_progdir = programs_browser->directory();
    Fl_String m_submenu = fl_input(_("Please enter name of the new submenu:"));
    
    if (!m_submenu.empty())
    {
	Fl_String path = m_progdir + slash + m_submenu;
        if (mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR))
        {
            fl_alert(_("Cannot create submenu!"));
            return;
        }
    }
    programs_browser->directory(m_progdir);
    programs_browser->relayout();
    programs_browser->redraw();    
}

void cb_change_dir(Fl_Widget *w, void  *)
{
    if(Fl::event_clicks() || Fl::event_key() == FL_Enter) {
        Fl_String path_name(programs_browser->filename_full());

        if(path_name.empty()) {
            programs_browser->up();
            return;
        }

        if(fl_is_dir(path_name)) {

            programs_browser->load(path_name);
            programs_browser->redraw();

        } else {

            Menu_Edit_Dialog(1);

        }
    }
}

void cb_directory_up(Fl_Button *, void *)
{
    programs_browser->up();
}

int SomethingInDir(char *path)
{
    dirent **files;
    int numberOfFiles = 0;

    numberOfFiles = fl_filename_list(path, &files);

    if (numberOfFiles > 2)       // . | .. | +
    {
        for (int i = 0; i < numberOfFiles; i ++)
        {
            free(files[i]);
        }
        free(files);
        return numberOfFiles;
    }
    else
    {
        return 0;
    }
}

void cb_delete_submenu(Fl_Button *, void*)
{
    Fl_String submenu_path(programs_browser->filename_full());
    if (fl_file_exists(submenu_path) && fl_is_dir(submenu_path))
    {
        if (SomethingInDir(submenu_path))
        {
            fl_alert(_("You should delete all the items from the submenu, before you can delete it!"));
            return;
        }
        rmdir(submenu_path);
        programs_browser->directory(programs_browser->directory());
        programs_browser->redraw();
    }
}

void cb_delete_item(Fl_Button *, void *)
{
    Fl_String submenu_path(programs_browser->filename_full());
    if(fl_file_exists(submenu_path) && !fl_is_dir(submenu_path))
    {
        unlink(submenu_path);
        programs_browser->directory(programs_browser->directory());
        programs_browser->redraw();
    }
}

void cb_new_item(Fl_Button *, void *)
{
    Menu_Edit_Dialog(0);
    programs_browser->redraw();
}

void cb_edit_item(Fl_Button *, void *)
{
    Fl_String submenu_path(programs_browser->filename_full());
    if(!fl_is_dir(submenu_path))
    {
       Menu_Edit_Dialog(1);
       programs_browser->redraw();
    }
}

void cbCloseWindow(Fl_Widget *, Fl_Window *windowToClose)
{
    windowToClose->hide();
}

void Exit_Editor(Fl_Widget *w, void *d)
{
    Fl_Window *t = (Fl_Window*) d;
    t->hide();
}
