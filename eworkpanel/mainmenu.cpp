#include "mainmenu.h"
#include "menu.h"

#include <edeconf.h>
#include <unistd.h>
#include <pwd.h>
#include <locale.h>

#include <icons/ede-small.xpm>
#include <icons/find.xpm>
#include <icons/run.xpm>
#include <icons/programs.xpm>

fltk::Pixmap ede_pix((const char **)ede_small2_xpm);
fltk::Pixmap programs_pix((const char **)programs_xpm);
fltk::Pixmap find_pix((const char **)find_xpm);
fltk::Pixmap run_pix((const char **)run_xpm);

// strdupcat() - it's cool to strcat with implied realloc
//  -- NOTE: due to use of realloc *always* use strdupcat return value:
//            dest = strdupcat(dest,src);
// and *never* use it like:
//            strdupcat(dest,src);
char *strdupcat(char *dest, const char *src)
{
	if (!dest) {
		dest=(char*)malloc(strlen(src));
	} else {
		dest=(char*)realloc (dest, strlen(dest)+strlen(src)+1);
	}
	strcat(dest,src);
	return dest;
}


///////////////////////////
extern EDE_Config pGlobalConfig;

MainMenu::MainMenu()
    : fltk::Menu_Button(0,0,0,0, "Start")
{
    layout_align(fltk::ALIGN_LEFT);

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
    fltk::Style *s = fltk::Style::find("Menu");
    int menuheight = s->box->dh();
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *i = child(n);
        if(!i) break;
        if(!i->visible()) continue;

        fltk::font(i->label_font(), i->label_size());
        menuheight += i->height()+s->leading;
    }
    return menuheight;
}

void MainMenu::draw()
{
    fltk::Boxtype box = this->box();
    fltk::Flags flags;
    fltk::Color color = this->color();
    fltk::Color lcolor = label_color();
    if (!active_r())
    {
        flags = fltk::INACTIVE;
    }
    else if (belowmouse()) {
        flags = fltk::HIGHLIGHT;
        color = fltk::lighter(color);
        lcolor = fltk::lighter(label_color());
        if(!color) color = this->color();
        if(!lcolor) color = this->label_color();        
    }
    else if (m_open) {
        flags = fltk::HIGHLIGHT|fltk::VALUE;
        color = fltk::lighter(color);
        lcolor = fltk::lighter(label_color());
        if(!color) color = this->color();
        if(!lcolor) color = this->label_color();
    } else {
        flags = 0;
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

    fltk::font(label_font(), label_size());
    label_type()->draw(label(), X, Y, W-X, H, lcolor, flags|fltk::ALIGN_LEFT);
}

void MainMenu::layout()
{
    fltk::font(label_font(), label_size());
    int W = int(fltk::width(label())) + 12;
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
    fltk::Menu_Button::layout();
}

void MainMenu::clear_favourites()
{
    static char* favourites;
    if(!favourites || strcmp(favourites,"")==0) {
        favourites = strdupcat(favourites,fltk::homedir());
        favourites = strdupcat(favourites,"/.ede/favourites/");

        if(!fltk::file_exists(favourites)) {
            mkdir( favourites, 0777 );
        }
    }

    dirent **files;
    int pNumFiles = fltk::filename_list(favourites, &files);

    if (pNumFiles > 10)
    {
        for (int i=0; i<(pNumFiles-10); i++) {
            if (strcmp(files[i]->d_name, ".") != 0 && strcmp(files[i]->d_name, "..") != 0 ) {
                char* filename = strdup(favourites);
                filename = strdupcat(filename,files[i]->d_name);
                unlink(filename);
		free(filename);
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
void MainMenu::resolve_program(char* cmd)
{
    char pRun[FL_PATH_MAX];

    snprintf(pRun, sizeof(pRun)-1, "elauncher \"%s\"", cmd);
    fltk::start_child_process(pRun, false);
}

fltk::Image *MainMenu::find_image(const char* icon) const
{
    char* iconpath = strdup(fltk::file_expand(icon));

    fltk::Image *im=0;

    if(fltk::file_exists(iconpath))
        im = fltk::Image::read(iconpath);

    if(!im) {
        free(iconpath);
        iconpath = strdup(PREFIX"/share/ede/icons/16x16/");
	iconpath = strdupcat(icon);
        im = fltk::Image::read(iconpath);
    }
    free(iconpath);

    if(im && (im->width()!=16 || im->height()!=16)) {
        fltk::Image *scaled = im->scale(16, 16);
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

int str_to_type(char* str)
{
    if(strcmp(str,"Exec")==0) return ITEM_EXEC;
    else if(strcmp(str,"AppDir")==0) return ITEM_APPDIR;
    else if(strcmp(str,"SubDir")==0) return ITEM_SUBDIR;
    else if(strcmp(str,"FileBrowser")==0) return ITEM_FILEBROWSER;
    else if(strcmp(str,"Divider")==0) return ITEM_DIVIDER;
    return ITEM_NONE;
}

bool is_group_item(int t) { return (t==ITEM_APPDIR || t==ITEM_SUBDIR || t==ITEM_FILEBROWSER); }

char* MainMenu::get_item_name(Fl_XmlNode *node)
{
    char* name = strdup("");
    for(int n=0; n<node->children(); n++) {
        fltk::XmlNode *np = node->child(n);
        if(np->is_element() && np->name()=="Name") {
            char* lang = strdup(np->get_attribute("Lang"));
            if(strcmp(lang,"")==0 && (strlen(name)==0)) {
                free(name);
                name=strdup("");
                np->text(name);
            } else if(strcmp(lang,locale())==0) {
                free(name);
		name=strdup("");
                np->text(name);
                break;
            }
        }
    }
    return name;
}

char* MainMenu::get_item_dir(Fl_XmlNode *node)
{
    char* dir = strdup(node->get_attribute("Dir"));

    if(strcmp(dir,"$DEFAULT_PROGRAMS_DIR")==0) {
        free(dir);
        dir = strdup(PREFIX"/share/ede/programs");
    }

    return fltk::file_expand(dir);
}

// THIS MUST BE CHANGED ASAP!
#include "logoutdialog.h"
#include "aboutdialog.h"
void MainMenu::set_exec(EItem *i, const char* exec)
{
    if(strcmp(exec,"$LOGOUT")==0)
        i->callback((fltk::Callback*)LogoutDialog);
    else if(strcmp(exec,"$ABOUT")==0)
        i->callback((fltk::Callback *)AboutDialog);
    else
        i->exec(exec);
}

void MainMenu::build_menu_item(fltk::XmlNode *node)
{
    if(!node) return;

    int type = str_to_type(node->get_attribute("Type"));
    if(type==ITEM_NONE) return;

    fltk::Widget *w=0;
    EItemGroup *g=0;
    EItem *i=0;

    switch(type) {
    case ITEM_EXEC:
        i = new EItem(this);
        i->callback((fltk::Callback*)cb_exec_item);
        set_exec(i, node->get_attribute("Exec"));
        i->image(run_pix);
        w = (fltk::Widget *)i;
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
        w = (fltk::Widget *)new fltk::Menu_Divider();
        break;
    }

    if(g) {
        g->begin();
        w = (fltk::Widget*)g;
    }

    fltk::Image *im=0;
    if(node->has_attribute("Icon")) {
        im = find_image(node->get_attribute("Icon"));
    } else {
        char* im_path = strdup(node->get_attribute("Exec"));
        im_path = strdupcat(im_path,".png");
        im = find_image(im_path);
    }
    if(im) w->image(im);

    char* label = strdup(get_item_name(node));
    w->label(label);

    for(int n=0; n<node->children(); n++) {
        fltk::XmlNode *np = node->child(n);
        if((np->is_element() || np->is_leaf()) && np->name()=="Item")
            build_menu_item(np);
    }

    if(w->is_group())
        ((fltk::Group*)w)->end();
}

void MainMenu::init_entries()
{
    // Update locale
    m_locale = setlocale(LC_ALL, NULL);
    int pos = m_locale.rpos('_');
    if(pos>0) m_locale.sub_delete(pos, m_locale.length()-pos);
    if(m_locale=="C" || m_locale=="POSIX") m_locale.clear();

    const char *file = find_config_file("ede_mainmenu.xml", true);

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
        fltk::warning("Menu not found, creating default..");
        try {
            fltk::Buffer buf;
            buf.append(default_menu, strlen(default_menu));
            buf.save_file(file);
        } catch(fltk::Exception &e) {
            fltk::warning(e.text());
        }
        fp = fopen(file, "r");
        if(!fp) fltk::fatal("Cannot write default menu.");
    }
    fltk::XmlLocator locator;

    fltk::XmlDoc *doc=0;
    if(fp) {
        try {
            doc = fltk::XmlParser::create_dom(fp, &locator, false);
        } catch(fltk::XmlException &exp) {
            char* error = strdup(exp.text());
            error = strdupcat(error,"\n\n");
            error = strdupcat(error,fltk::XmlLocator::error_line(file, *exp.locator()));
            error = strdupcat(error,"\n");
            fltk::warning(error);
        }
    }

    if(!doc) {
        // One more try!
        try {
            fltk::Buffer buf;
            buf.append(default_menu, strlen(default_menu));
            doc = fltk::XmlParser::create_dom(buf.data(), buf.bytes(), &locator, false);
        } catch(fltk::Exception &e) {
            fltk::fatal("Cannot open menu! [%s]", e.text().c_str());
        }
    }

    if(doc) {
        begin();

        fltk::XmlNode *node = doc->root_node();
        if(node) {
            for(int n=0; n<node->children(); n++) {
                fltk::XmlNode *np = node->child(n);
                if((np->is_element() || np->is_leaf()) && np->name()=="Item")
                    build_menu_item(np);
            }
        }

        end();
    }
}

int MainMenu::popup(int X, int Y, int W, int H)
{
    if(fltk::event_button()==1) {
        m_open = true;
        init_entries();
        int ret = fltk::Menu_::popup(X, Y-calculate_height()-h()-1, W, H);
        clear();
        m_open = false;
        return ret;
    }
    return 0;
}
