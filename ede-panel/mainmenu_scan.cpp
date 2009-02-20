#include "mainmenu.h"
#include "item.h"

#include <icons/file.xpm>
Fl_Pixmap file_pix(file_xpm);

extern Fl_Pixmap programs_pix;
extern Fl_Pixmap run_pix;

static void cb_file_item(EItem *item, void *)
{
    Fl_String pFavouriteFile(fl_homedir());
    pFavouriteFile += "/.ede/favourites/";
    pFavouriteFile += fl_file_filename(item->filename());

    Fl_Config pItemConfig(item->filename(), true, false);
    Fl_String cmd;
    if(!pItemConfig.get("Desktop Entry", "Exec", cmd, ""))
    {
        MainMenu::clear_favourites();
        symlink(item->filename(), pFavouriteFile);
        MainMenu::resolve_program(cmd);
    }
}

static void cb_open_dir(Fl_Widget *w, void *)
{
	EItemGroup *g = (EItemGroup *)w->parent();

	Fl_Config conf(fl_find_config_file("ede.conf", false));
	Fl_String term;
	conf.get("Terminal", "Terminal", term, 0);
	if(term.empty())
		term = "xterm";

	Fl_String cmd("cd ");
	cmd += g->dir();
	cmd += "; " + term;

	fl_start_child_process(cmd, false);
}

void MainMenu::scan_filebrowser(const Fl_String &path)
{
    EItem *i = new EItem(this, _("Open Directory.."));
    i->callback(cb_open_dir);
    i->image(run_pix);

    new Fl_Menu_Divider();

    EItemGroup *mNewGroup=0;
    struct dirent **files;
    int count = fl_filename_list(path, &files);

    int n;
    for(n=0; n<count; n++)
    {
        if( !strcmp(files[n]->d_name, ".") || !strcmp(files[n]->d_name, "..") || !strcmp(files[n]->d_name, "CVS") )
        {
            free((char*)files[n]);
            files[n] = 0;
            continue;
        }

        Fl_String filename(path);
        filename += '/';
        filename += files[n]->d_name;

        if(fl_is_dir(filename)) {
            mNewGroup = new EItemGroup(this, BROWSER_GROUP);
            mNewGroup->label(files[n]->d_name);
            mNewGroup->image(programs_pix);
            mNewGroup->dir(filename);

            mNewGroup->end();

            if(access(filename, R_OK)) {
                mNewGroup->label_color(fl_inactive(FL_RED));
                mNewGroup->access(false);
            }
            free((char*)files[n]);
            files[n] = 0;
        }
    }

    for(n=0; n<count; n++)
    {
        if(!files[n]) continue;

        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, ".."))
        {
            Fl_String filename(path + '/');
            filename += files[n]->d_name;

            EItem *mNewItem = new EItem(this);
            mNewItem->type(EItem::FILE);
            mNewItem->image(file_pix);
            mNewItem->copy_label(files[n]->d_name);

            if(access(filename, R_OK)) {
                mNewItem->label_color(fl_inactive(FL_RED));
            }
        }
        free((char*)files[n]);
    }

    if(count>0 && files) free((char**)files);
}

void MainMenu::scan_programitems(const char *path)
{
    EItemGroup *mNewGroup;
    Fl_String NameEntry;
    bool added = false;

    Fl_String localizedName;
    if(!locale().empty())
        localizedName.printf("Name[%s]", locale().c_str());
    else
        localizedName = "Name";

    struct dirent **files;
    int count = fl_filename_list(path, &files);

    int n;
    for(n=0; n<count; n++)
    {
        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, "..") && strcmp(files[n]->d_name, "CVS") )
        {
            Fl_String filename = path;
            filename += '/';
            filename += files[n]->d_name;

            if(fl_is_dir(filename))
            {
                added=true;
                mNewGroup = new EItemGroup(this, APP_GROUP);
                mNewGroup->image(programs_pix);
                mNewGroup->dir(filename);

                Fl_String locale_file(filename);
                locale_file += "/.directory";

                Fl_Config pLocConfig(locale_file, true, false);
                pLocConfig.set_section("Desktop Entry");

                if(!pLocConfig.read(localizedName, NameEntry, "")) {
                    // Localized name
                    mNewGroup->label(NameEntry);
                } else {
                    if(!pLocConfig.read("Name", NameEntry, "")) {
                        // Default name
                        mNewGroup->label(NameEntry);
                    } else {
                        // Fall back to directory name
                        mNewGroup->label(files[n]->d_name);
                    }
                }
                mNewGroup->end();

                free(files[n]);
                files[n] = 0;
            }
        }
    }

    for(n=0; n<count; n++)
    {
        if(!files[n]) continue;

        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, "..") && strstr(files[n]->d_name, ".desktop"))
        {
            Fl_String filename(path);
            filename += '/';
            filename += files[n]->d_name;

            // we check first for localised names...
            Fl_Config ItemConfig(filename, true, false);
            ItemConfig.set_section("Desktop Entry");

            bool noDisplay = false;
            ItemConfig.read("NoDisplay", noDisplay);
            if(noDisplay) continue;

            if(ItemConfig.read(localizedName, NameEntry, "")) {
                ItemConfig.read("Name", NameEntry, "");
            }

            if(!ItemConfig.error() && !NameEntry.empty())
            {
                added=true;
                EItem *mNewItem = new EItem(this);
                mNewItem->type(EItem::FILE);
                mNewItem->label(NameEntry);
                mNewItem->filename(filename);
                mNewItem->callback((Fl_Callback *)cb_file_item, 0);

                if(!ItemConfig.read("Icon", NameEntry, ""))
                    mNewItem->image(find_image(NameEntry));

                if(!mNewItem->image())
                    mNewItem->image(file_pix);

                if(!ItemConfig.read("Exec", NameEntry, ""))
                    mNewItem->exec(NameEntry);
            }
        }
        if(files[n]) free(files[n]);
    }

    if(count>0 && files) free(files);
    if(!added)
        new Fl_Divider();
}
