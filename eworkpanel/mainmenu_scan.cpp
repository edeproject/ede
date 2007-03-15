#include "mainmenu.h"
#include "item.h"

#include <icons/file.xpm>
fltk::Pixmap file_pix(file_xpm);

extern fltk::Pixmap programs_pix;
extern fltk::Pixmap run_pix;

static void cb_file_item(EItem *item, void *)
{
    char* pFavouriteFile = strdup(fltk::homedir());
    pFavouriteFile = strdupcat(pFavouriteFile"/.ede/favourites/");
    pFavouriteFile = strdupcat(pFavouriteFile,fltk::file_filename(item->filename()));

    EDE_Config pItemConfig(item->filename(), true, false);
    char* cmd;
    if(!pItemConfig.get("Desktop Entry", "Exec", cmd, ""))
    {
        MainMenu::clear_favourites();
        symlink(item->filename(), pFavouriteFile);
        MainMenu::resolve_program(cmd);
    }
    free(pFavouriteFile);
}

static void cb_open_dir(fltk::Widget *w, void *)
{
	EItemGroup *g = (EItemGroup *)w->parent();

	EDE_Config conf(fltk::find_config_file("ede.conf", false));
	char* term;
	conf.get("Terminal", "Terminal", term, 0);
	if(term.empty())
		term = "xterm";

	char* cmd = strdup("cd ");
	cmd = strdupcat(cmd,g->dir());
	cmd = strdupcat("; ");
	cmd = strdupcat(term);

	fltk::start_child_process(cmd, false);
	free(term);
	free(cmd);
}

void MainMenu::scan_filebrowser(const char* path)
{
    EItem *i = new EItem(this, _("Open Directory.."));
    i->callback(cb_open_dir);
    i->image(run_pix);

    new fltk::Menu_Divider();

    EItemGroup *mNewGroup=0;
    struct dirent **files;
    int count = fltk::filename_list(path, &files);

    int n;
    for(n=0; n<count; n++)
    {
        if( !strcmp(files[n]->d_name, ".") || !strcmp(files[n]->d_name, "..") || !strcmp(files[n]->d_name, "CVS") )
        {
            free((char*)files[n]);
            files[n] = 0;
            continue;
        }

        char* filename = strdup(path);
        filename = strdupcat(filename,"/");
        filename = strdupcat(filename,files[n]->d_name);

        if(fltk::is_dir(filename)) {
            mNewGroup = new EItemGroup(this, BROWSER_GROUP);
            mNewGroup->label(files[n]->d_name);
            mNewGroup->image(programs_pix);
            mNewGroup->dir(filename);

            mNewGroup->end();

            if(access(filename, R_OK)) {
                mNewGroup->label_color(fltk::inactive(FL_RED));
                mNewGroup->access(false);
            }
            free((char*)files[n]);
            files[n] = 0;
        }
	free(filename);
    }

    for(n=0; n<count; n++)
    {
        if(!files[n]) continue;

        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, ".."))
        {
            char* filename = strdup(path);
	    filename = strdupcat(filename, "/");
            filename = strdupcat(files[n]->d_name);

            EItem *mNewItem = new EItem(this);
            mNewItem->type(EItem::FILE);
            mNewItem->image(file_pix);
            mNewItem->copy_label(files[n]->d_name);

            if(access(filename, R_OK)) {
                mNewItem->label_color(fl_inactive(FL_RED));
            }
	    free(filename);
        }
        free((char*)files[n]);
    }

    if(count>0 && files) free((char**)files);
}

void MainMenu::scan_programitems(const char *path)
{
    EItemGroup *mNewGroup;
    char* NameEntry;
    bool added = false;

    char* localizedName;
    if(strlen(locale())>0)
        sprintf(localizedName,"Name[%s]", locale());
    else
        localizedName = strdup("Name");

    struct dirent **files;
    int count = fltk::filename_list(path, &files);

    int n;
    for(n=0; n<count; n++)
    {
        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, "..") && strcmp(files[n]->d_name, "CVS") )
        {
            char* filename = strdup(path);
            filename = strdupcat(filename,"/");
            filename = strdupcat(filename,files[n]->d_name);

            if(fltk::is_dir(filename))
            {
                added=true;
                mNewGroup = new EItemGroup(this, APP_GROUP);
                mNewGroup->image(programs_pix);
                mNewGroup->dir(filename);

                char* locale_file = strdup(filename);
                locale_file = strdupcat(locale_file,"/.directory");

                EDE_Config pLocConfig(locale_file, true, false);
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
		free(locale_file);
            }
	    free(filename);
        }
    }

    for(n=0; n<count; n++)
    {
        if(!files[n]) continue;

        if( strcmp(files[n]->d_name, ".") && strcmp(files[n]->d_name, "..") && strstr(files[n]->d_name, ".desktop"))
        {
            char* filename=strdup(path);
            filename = strdupcat(filename,"/");
            filename = strdupcat(filename,files[n]->d_name);

            // we check first for localised names...
            EDE_Config ItemConfig(filename, true, false);
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
                mNewItem->callback((fltk::Callback *)cb_file_item, 0);

                if(!ItemConfig.read("Icon", NameEntry, ""))
                    mNewItem->image(find_image(NameEntry));

                if(!mNewItem->image())
                    mNewItem->image(file_pix);

                if(!ItemConfig.read("Exec", NameEntry, ""))
                    mNewItem->exec(NameEntry);
            }
	    free(filename);
        }
        if(files[n]) free(files[n]);
    }

    if(count>0 && files) free(files);
    if(!added)
        Fl_Divider *mDivider = new Fl_Divider();
	
    free(localizedName);
    free(NameEntry);
}
