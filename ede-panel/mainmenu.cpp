#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mainmenu.h"
#include "menu.h"

#include <unistd.h>
#include <pwd.h>
#include <locale.h>

#include <icons/ede-small.xpm>
#include <icons/find.xpm>
#include <icons/run.xpm>
#include <icons/programs.xpm>

Fl_Pixmap ede_pix((const char **)ede_small2_xpm);
Fl_Pixmap programs_pix((const char **)programs_xpm);
Fl_Pixmap find_pix((const char **)find_xpm);
Fl_Pixmap run_pix((const char **)run_xpm);

///////////////////////////
extern Fl_Config pGlobalConfig;

MainMenu::MainMenu()
    : Fl_Menu_Button(0,0,0,0, "Start")
{
    layout_align(FL_ALIGN_LEFT);

    label_font(label_font()->bold());
    label_size(label_size()+2);

    m_modified = 0;
    e_image = 0;
    m_open = false;

    bool showusername;
    pGlobalConfig.get("Panel", "ShowUsernameOnMenu", showusername, false);
    struct passwd *PWD;
    /* Search for an entry with a matching user ID.  */
    PWD = getpwuid(getuid());
    if(showusername && PWD && PWD->pw_name && *PWD->pw_name) {
        label(PWD->pw_name);
    } else {
        label("EDE");
    }
    
    tooltip(_("Welcome to the Equinox Desktop Environment."));
}

MainMenu::~MainMenu()
{
    if(e_image)
        delete e_image;
}

int MainMenu::calculate_height() const
{
    Fl_Style *s = Fl_Style::find("Menu");
    int menuheight = s->box->dh();
    for(int n=0; n<children(); n++)
    {
        Fl_Widget *i = child(n);
        if(!i) break;
        if(!i->visible()) continue;

        fl_font(i->label_font(), i->label_size());
        menuheight += i->height()+s->leading;
    }
    return menuheight;
}

int MainMenu::handle(int e) {
	switch (e) {
		case FL_ENTER:
		case FL_LEAVE:
			redraw();
	}
	return Fl_Menu_Button::handle(e);
}

void MainMenu::draw()
{
    Fl_Boxtype box = this->box();
    Fl_Flags flags;
    Fl_Color color = this->color();
    Fl_Color lcolor = label_color();

    if (!active_r())
        flags = FL_INACTIVE;
    else if (m_open)
        flags = FL_VALUE;
    else
        flags = 0;

    if (belowmouse()) {
        flags = flags|FL_HIGHLIGHT;
        color = fl_lighter(color);
        lcolor = fl_lighter(label_color());
        if(!color) color = this->color();
        if(!lcolor) color = this->label_color();
    } 
    box->draw(0, 0, this->w(), this->h(), color, flags);

    int X=0, Y=0, W=w(), H=h();
    box->inset(X,Y,W,H);

    if(image()) {
        int imY = (h()/2)-(image()->height()/2);
        image()->draw(6, imY, image()->width(), image()->height(), flags);
        X+=image()->width()+6;
    } else {
        X += 4;
        W -= 4;
    }

    fl_font(label_font(), label_size());
    label_type()->draw(label(), X, Y, W-X, H, lcolor, flags|FL_ALIGN_LEFT);
}

void MainMenu::layout()
{
    fl_font(label_font(), label_size());
    int W = int(fl_width(label())) + 12;
    int H = h();
    int im_size = H-6;

    if(!e_image || (e_image && e_image->height()!=im_size)) {
        if(e_image) delete e_image;
        if(ede_pix.height()==im_size) {
            e_image=0;
            image(ede_pix);
        }
        else {
            e_image = ede_pix.scale(im_size, im_size);
            image(e_image);
        }
    }
    if(image()) W+=image()->width();

    w(W);
    Fl_Menu_Button::layout();
}

void MainMenu::clear_favourites()
{
    static Fl_String favourites;
    if(favourites.empty()) {
        favourites = fl_homedir();
        favourites += "/.ede/favourites/";

        if(!fl_file_exists(favourites)) {
            mkdir( favourites, 0777 );
        }
    }

    dirent **files;
    int pNumFiles = fl_filename_list(favourites, &files);

    if (pNumFiles > 10)
    {
        for (int i=0; i<(pNumFiles-10); i++) {
            if (strcmp(files[i]->d_name, ".") != 0 && strcmp(files[i]->d_name, "..") != 0 ) {
                Fl_String filename(favourites);
                filename += files[i]->d_name;
                unlink(filename);
            }
        }
    }

    for(int i = 0; i < pNumFiles; i++)
        free(files[i]);

    if(pNumFiles && files)
        free(files);
}

/*
 %f a single file name, even if multiple files are selected.
    The system reading the Desktop Entry should recognize that
    the program in question cannot handle multiple file arguments,
    and it should should probably spawn and execute multiple copies
    of a program for each selected file if the program is not
    able to handle additional file arguments. If files are not on
    the local file system (i.e. HTTP or FTP locations), the files will
    be copied to the local file system and %f will be expanded to point
    at the temporary file. Used for programs that do not understand URL syntax.

 %F a list of files. Use for apps that can open several local files at once.
 %u a single URL.
 %U a list of URLs.
 %d the directory of the file to open.
 %D a list of directories
 %n a single filename (without path)
 %N a list of filenames (without path)
 %i the icon associated with the desktop entry
 %m the mini-icon associated with the desktop entry
 %c the comment associated with the desktop entry
 %k the name of the desktop file
 %v the name of the Device entry in the desktop file
 */
 
// This function is now implemented in elauncher
void MainMenu::resolve_program(Fl_String cmd)
{
    char pRun[FL_PATH_MAX];

    snprintf(pRun, sizeof(pRun)-1, "elauncher \"%s\"", cmd.c_str());
    fl_start_child_process(pRun, false);
}

Fl_Image *MainMenu::find_image(const Fl_String &icon) const
{
    Fl_String iconpath( fl_file_expand(icon) );

    Fl_Image *im=0;

    if(fl_file_exists(iconpath))
        im = Fl_Image::read(iconpath);

    if(!im) {
        iconpath = PREFIX"/share/ede/icons/16x16/"+icon;
        im = Fl_Image::read(iconpath);
    }

    if(im && (im->width()!=16 || im->height()!=16)) {
        Fl_Image *scaled = im->scale(16, 16);
        if(scaled) {
            delete im;
            im = scaled;
        }
    }

    return im;
}

/*
g->dir(PREFIX"/share/ede/programs");
i->exec("ehelpbook "PREFIX"/share/ede/doc/index.html");
*/
enum {
    ITEM_NONE = 0,
    ITEM_EXEC,
    ITEM_APPDIR,
    ITEM_SUBDIR,
    ITEM_FILEBROWSER,
    ITEM_DIVIDER
};

int str_to_type(Fl_String &str)
{
    if(str=="Exec") return ITEM_EXEC;
    else if(str=="AppDir") return ITEM_APPDIR;
    else if(str=="SubDir") return ITEM_SUBDIR;
    else if(str=="FileBrowser") return ITEM_FILEBROWSER;
    else if(str=="Divider") return ITEM_DIVIDER;
    return ITEM_NONE;
}

bool is_group_item(int t) { return (t==ITEM_APPDIR || t==ITEM_SUBDIR || t==ITEM_FILEBROWSER); }

Fl_String MainMenu::get_item_name(Fl_XmlNode *node)
{
    Fl_String name;
    for(uint n=0; n<node->children(); n++) {
        Fl_XmlNode *np = node->child(n);
        if(np->is_element() && np->name()=="Name") {
            Fl_String &lang = np->get_attribute("Lang");
            if(lang=="" && name.length()==0) {
                name.clear();
                np->text(name);
            } else if(lang==locale()) {
                name.clear();
                np->text(name);
                break;
            }
        }
    }
    return name;
}

Fl_String MainMenu::get_item_dir(Fl_XmlNode *node)
{
    Fl_String dir( node->get_attribute("Dir") );

    if(dir=="$DEFAULT_PROGRAMS_DIR")
        dir = Fl_String(PREFIX"/share/ede/programs");

    return fl_file_expand(dir);
}

// THIS MUST BE CHANGED ASAP!
#include "logoutdialog.h"
#include "aboutdialog.h"
void MainMenu::set_exec(EItem *i, const Fl_String &exec)
{
    if(exec=="$LOGOUT")
        i->callback((Fl_Callback*)LogoutDialog);
    else if(exec=="$ABOUT")
        i->callback( (Fl_Callback *)AboutDialog);
    else
        i->exec(exec);
}

void MainMenu::build_menu_item(Fl_XmlNode *node)
{
    if(!node) return;

    int type = str_to_type(node->get_attribute("Type"));
    if(type==ITEM_NONE) return;

    Fl_Widget *w=0;
    EItemGroup *g=0;
    EItem *i=0;

    switch(type) {
    case ITEM_EXEC:
        i = new EItem(this);
        i->callback((Fl_Callback*)cb_exec_item);
        set_exec(i, node->get_attribute("Exec"));
        i->image(run_pix);
        w = (Fl_Widget *)i;
        break;

    case ITEM_APPDIR:
        g = new EItemGroup(this, APP_GROUP);
        g->image(programs_pix);
        g->dir(get_item_dir(node));
        break;

    case ITEM_SUBDIR:
        g = new EItemGroup(this, APP_GROUP);
        g->image(programs_pix);
        break;

    case ITEM_FILEBROWSER:
        g = new EItemGroup(this, BROWSER_GROUP);
        g->dir(get_item_dir(node));
        g->image(find_pix);
        break;

    case ITEM_DIVIDER:
        w = (Fl_Widget *)new Fl_Menu_Divider();
        break;
    }

    if(g) {
        g->begin();
        w = (Fl_Widget*)g;
    }

    Fl_Image *im=0;
    if(node->has_attribute("Icon")) {
        im = find_image(node->get_attribute("Icon"));
    } else {
        Fl_String im_path(node->get_attribute("Exec"));
        im_path += ".png";
        im = find_image(im_path);
    }
    if(im) w->image(im);

    Fl_String label = get_item_name(node);
    w->label(label);

    for(uint n=0; n<node->children(); n++) {
        Fl_XmlNode *np = node->child(n);
        if((np->is_element() || np->is_leaf()) && np->name()=="Item")
            build_menu_item(np);
    }

    if(w->is_group())
        ((Fl_Group*)w)->end();
}

void MainMenu::init_entries()
{
    // Update locale
    m_locale = setlocale(LC_ALL, NULL);
    int pos = m_locale.rpos('_');
    if(pos>0) m_locale.sub_delete(pos, m_locale.length()-pos);
    if(m_locale=="C" || m_locale=="POSIX") m_locale.clear();

    const char *file = fl_find_config_file("ede_mainmenu.xml", true);

    struct stat s;
    if(lstat(file, &s) == 0) {
        if(!m_modified) m_modified = s.st_mtime;
        if(m_modified != s.st_mtime) {
            //file has changed..
            m_modified = s.st_mtime;
            clear();
        }
    }

    if(children()>0) return;

    FILE *fp = fopen(file, "r");
    if(!fp) {
        Fl::warning("Menu not found, creating default..");
        try {
            Fl_Buffer buf;
            buf.append(default_menu, strlen(default_menu));
            buf.save_file(file);
        } catch(Fl_Exception &e) {
            Fl::warning(e.text());
        }
        fp = fopen(file, "r");
        if(!fp) Fl::fatal("Cannot write default menu.");
    }
    Fl_XmlLocator locator;

    Fl_XmlDoc *doc=0;
    if(fp) {
        try {
            doc = Fl_XmlParser::create_dom(fp, &locator, false);
        } catch(Fl_XmlException &exp) {
            Fl_String error(exp.text());
            error += "\n\n";
            error += Fl_XmlLocator::error_line(file, *exp.locator());
            error += '\n';
            Fl::warning(error);
        }
    }

	fclose(fp);

    if(!doc) {
        // One more try!
        try {
            Fl_Buffer buf;
            buf.append(default_menu, strlen(default_menu));
            doc = Fl_XmlParser::create_dom(buf.data(), buf.bytes(), &locator, false);
        } catch(Fl_Exception &e) {
            Fl::fatal("Cannot open menu! [%s]", e.text().c_str());
        }
    }

    if(doc) {
        begin();

        Fl_XmlNode *node = doc->root_node();
        if(node) {
            for(uint n=0; n<node->children(); n++) {
                Fl_XmlNode *np = node->child(n);
                if((np->is_element() || np->is_leaf()) && np->name()=="Item")
                    build_menu_item(np);
            }
        }

        end();
    }
}

int MainMenu::popup(int X, int Y, int W, int H)
{
    if(Fl::event_button()==1) {
        m_open = true;
        init_entries();
        int newy=Y-calculate_height()-h()-1;
        // mainmenu is inside a group:
        if (parent()->parent()->y()+newy<1) newy=Y;
        int ret = Fl_Menu_::popup(X, newy, W, H);
        clear();
        m_open = false;
        return ret;
    }
    return 0;
}
