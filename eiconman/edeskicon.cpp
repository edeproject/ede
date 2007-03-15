/*
 * $Id$
 *
 * Desktop icons manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "edeskicon.h"
#include "propdialog.h"
#include "eiconman.h"
#include "../edeconf.h"

//#include <efltk/fl_draw.h>

//#include <efltk/Fl_Image.h>
//#include <efltk/Fl_Image_Filter.h>

#define ICONSIZE 48

//Fl_Menu_Button *popup;
Icon *menu_item=0;

extern Desktop *desktop;

void menu_cb(fltk::Widget *w, long a)
{
    if(menu_item) {
        switch(a) {
        case 1:
            Icon::cb_execute((fltk::Item*)w, menu_item);
            break;
        case 3:
            delete_icon(w, menu_item);
            break;
        case 4:
            property_dialog(w, menu_item, false);
            break;
        }
    }
}

char* get_localized_string()
{
    const char* locale = setlocale(LC_MESSAGES, NULL);
//    int pos = locale.rpos('_');
//    if(pos>0) locale.sub_delete(pos, locale.length()-pos);
    if(strlen(locale)==0 || strcmp(locale,"C")==0 || strcmp(locale,"POSIX")==0) 
        return strdup("Name");
    else {
        char localName[128];
        sprintf(localName,"Name[%s]", locale);
        return strdup(localName);
    }
}

char* get_localized_name(EDE_Config &iconConfig)
{
    char icon_name[128]; //wchar ?
    char *tmp = get_localized_string();
    if(iconConfig.get("Desktop Entry", tmp, icon_name)) {
        iconConfig.get("Desktop Entry", "Name", icon_name, "None");
    }
    free(tmp);
    return strdup(icon_name);
}

Icon::Icon(char *icon_config) : fltk::Widget(0, 0, ICONSIZE, ICONSIZE)
{
    if(!popup) {
        popup = new fltk::PopupMenu(0, 0, 0, 0);
        if(popup->parent())
            popup->parent()->remove(popup);
        popup->parent(0);
        popup->type(fltk::PopupMenu::POPUP3);
        popup->begin();

        fltk::Item *open_item = new fltk::Item(_("&Open"));
        open_item->callback(menu_cb, 1);
        //open_item->x_offset(12);

        fltk::Item *delete_item = new fltk::Item(_("&Delete"));
        delete_item->callback(menu_cb, 3);
        //delete_item->x_offset(12);

	   new fltk::Divider();

        fltk::Item *property_item = new fltk::Item(_("&Properties"));
        property_item->callback(menu_cb, 4);
        //property_item->x_offset(12);

        popup->end();
    }

    cfg = new EDE_Config(icon_config);

    icon_im = 0;
    micon = 0;

    cfg->set_section("Desktop Entry");
    cfg->read("X", x_position, 100);
    cfg->read("Y", y_position, 100);
    position(x_position, y_position);

//    label_font(FL_HELVETICA);
    label(icon_name);
    align(fltk::ALIGN_BOTTOM|fltk::ALIGN_WRAP);
    tooltip(icon_name);
    box(fltk::NO_BOX);

    update_all();
    desktop->begin();
}

Icon::~Icon()
{
    if (icon_im) delete icon_im;
    if (cfg) delete cfg;
}

void Icon::cb_execute_i()
{
    EDE_Config &iconfig = *cfg;
    iconfig.set_section("Desktop Entry");

    char *cmd=0;
    if(!iconfig.read("Exec", cmd, 0) && cmd)
    {
        char pRun[256];
        char browser[256];
        EDE_Config pGlobalConfig(EDE_Config::find_config_file("ede.conf", 0));
        pGlobalConfig.get("Web", "Browser", browser, 0, sizeof(browser));
        if(pGlobalConfig.error() && !browser) {
            strncpy(browser, "netscape", sizeof(browser));
        }

        char *location = cmd;
        char *prefix = strstr(location, ":");
        if(prefix) // it is internet resource
        {
            *prefix = '\0';
            if (!strcasecmp(location, "http") || !strcasecmp(location, "ftp") || !strcasecmp(location, "file"))
            {	snprintf(pRun, sizeof(pRun)-1, "%s %s &", browser, cmd);
            }
	    else if (!strcasecmp(location, "gg"))
	    {	snprintf(pRun, sizeof(pRun)-1, "%s http://www.google.com/search?q=\"%s\" &", browser, ++prefix);
	    }
	    else if (!strcasecmp(location, "leo"))
	    {	snprintf(pRun, sizeof(pRun)-1, "%s http://dict.leo.org/?search=\"%s\" &", browser, ++prefix);
	    }
	    else if (!strcasecmp(location, "av"))
	    {	snprintf(pRun, sizeof(pRun)-1, "%s http://www.altavista.com/sites/search/web?q=\"%s\" &", browser, ++prefix);
	    }
	    else  {
                snprintf(pRun, sizeof(pRun)-1, "%s %s &", browser, cmd);
            }
        }
	else // local executable
        {   snprintf(pRun, sizeof(pRun)-1, "%s &", cmd);
        }
        run_program(pRun);

        free((char*)cmd);
    }
}

int Icon::handle(int e)
{
    static int bx, by;
    static int x_icon, y_icon;
    static int X, Y;
    static bool button1 = false;
    int dx, dy;
    

    if (e==fltk::PUSH) {
        button1 = (fltk::event_button()==1);
    }

    // Left mouse button
    if(button1) {
        switch(e) {
        case fltk::DRAG:

            if(!micon) {
                micon = new MovableIcon(this);
                micon->show();
            }

            dx = ((fltk::event_x_root()-bx)/label_gridspacing) * label_gridspacing;
            dy = ((fltk::event_y_root()-by)/label_gridspacing) * label_gridspacing;
            X=x_icon+dx;
            Y=y_icon+dy;

            if(X<desktop->x()) X=desktop->x();
            if(Y<desktop->y()) Y=desktop->y();
            if(X+w()>desktop->x()+desktop->w()) X=desktop->x()+desktop->w()-w();
            if(Y+h()>desktop->y()+desktop->h()) Y=desktop->y()+desktop->h()-h();

            micon->position(X, Y);

            return 1;

        case fltk::RELEASE:

            // This happens only when there was no drag
            if(fltk::event_is_click()) {
                if (one_click_exec)
                    cb_execute_i();
                return 1;
            }

            // We will update config only on FL_RELEASE, when 
            // dragging is over
            if(micon) {
                delete micon;
                micon = 0;
            }

            position(X-desktop->x(), Y-desktop->y());
            desktop->redraw();

            cfg->set_section("Desktop Entry");
            cfg->write("X", x());
            cfg->write("Y", y());
            cfg->flush();

            return 1;
	    
        case fltk::PUSH:

            take_focus();

            bx = (fltk::event_x_root()/label_gridspacing)*label_gridspacing;
            by = (fltk::event_y_root()/label_gridspacing)*label_gridspacing;
            x_icon = ((desktop->x()+x())/label_gridspacing)*label_gridspacing;
            y_icon = ((desktop->y()+y())/label_gridspacing)*label_gridspacing;

            // Double click
            if ((!one_click_exec) && (Fl::event_clicks() > 0)) {
                fltk::event_clicks(0);
                cb_execute_i();
            }

            desktop->redraw();

            return 1;
        }
    }

    switch (e) {
    case fltk::SHORTCUT:
    case fltk::KEY:
        if(fltk::event_key()==fltk::Enter||fltk::event_key()==fltk::Space) {
            cb_execute_i();
        }
        break;

    case fltk::FOCUS:
    case fltk::ENTER:
        return 1;

    case fltk::PUSH:
        take_focus();
        desktop->redraw();
        if(fltk::event_button()==3) {
            menu_item = this;
            popup->popup();
            menu_item = 0;
            return 1;
        }
        break;

    default:
        break;
    }

    return fltk::Widget::handle(e);
}

void Icon::draw()
{
    fltk::Flags f=0;
    fltk::Image *im = icon_im;
    if(focused()) {
        f=fltk::SELECTED;
    }

    if(im)
        im->draw(0, 0, w(), h(),f);
    else {
        fltk::color(fltk::RED);
        fltk::rect(0,0,w(),h());
        fltk::color(fltk::BLACK);
        fltk::rectf(1,1,w()-2,h()-2);
        fltk::color(fltk::WHITE);
        fltk::font(label_font()->bold(), 10);
        fltk::draw("NO ICON FOUND!", 1, 1, w()-2, h()-2, fltk::ALIGN_TOP|fltk::ALIGN_LEFT|fltk::ALIGN_WRAP);
    }

    int X = w()-(w()/2)-(lwidth/2);
    int Y = h()+2;

    if(!label_trans) {
        fltk::color(label_background);
        fltk::rectf(X,Y,lwidth,lheight);
    }

    if(focused()) {
        focus_box()->draw(X, Y, lwidth, lheight, color(), 0);
    }

    fltk::font(label_font(), label_size());

    // A little shadow, from Dejan's request :)
    // SUCKS!
    /*fl_color(fl_darker(label_color()));
    fl_draw(label(), X-1, Y+1, lwidth, lheight, flags());
    fl_draw(label(), X, Y+1, lwidth, lheight, flags());
    */

    fltk::color(label_color());
    fltk::draw(label(), X, Y, lwidth, lheight, flags());
}

void Icon::update_icon()
{
    if(icon_im) delete icon_im;

    char path[fltk::PATH_MAX];
    snprintf(path,fltk::PATH_MAX,PREFIX"/share/ede/icons/48x48/%s",icon_file);

    if(!fltk::file_exists(path)) strncpy(path, icon_file, fltk::PATH_MAX);

    if(fltk::file_exists(path))
    {
        icon_im = fltk::Image::read(path, 0);
    } else {
        icon_im = 0;
    }

    if(!icon_im) {
        icon_im = fltk::Image::read(PREFIX"/share/ede/icons/48x48/folder.png", 0);
    }

    if(icon_im) {
        if(icon_im->width()!=48 || icon_im->height()!=48) {
            fltk::Image *old = icon_im;
            icon_im = old->scale(48,48);
            delete old;
        }
        icon_im->mask_type(MASK_ALPHA);
        icon_im->threshold(128);
    }
}

void Icon::layout()
{
    if(layout_damage()&fltk::LAYOUT_XYWH && icon_im)
    {
#if 0
        // Alpha blends image to bg!
        // This sucks, cause if icon overlaps another, it will
        // draw bg top of overlapped icon...
        if(icon_im->format()->Amask)
        {
            if(desktop->bg_image) {
                int pitch = icon_im->pitch();

                uint8 *data = new uint8[h()*pitch];

                int X=x(),Y=y(),W=w(),H=h();
                if(X<0) X=0;
                if(Y<0) Y=0;
                if(X+W>desktop->w()) X=desktop->w()-W;
                if(Y+H>desktop->h()) Y=desktop->h()-H;

                Fl_Rect r(X,Y,W,H);
                Fl_Rect r2(0,0,W,H);
                Fl_Renderer::blit(desktop->bg_image->data(), &r, desktop->bg_image->format(), desktop->bg_image->pitch(),
                                  data, &r2, icon_im->format(), pitch, 0);

                if(im) delete im;
                im = new Fl_Image(W, H, icon_im->format(), data);

                // Blit image data to our bg_image
                Fl_Renderer::alpha_blit(icon_im->data(), &r2, icon_im->format(), icon_im->pitch(),
                                        im->data(), &r2, im->format(), im->pitch(),
                                        0);
            } else {
                //blend to color
                im = icon_im->back_blend(desktop->bg_color);
            }
        }
        else
#endif
        {
            if(icon_im) {
                icon_im->mask_type(MASK_ALPHA);
                icon_im->threshold(128);
            }
        }
    }

    fltk::Widget::layout();
}

void Icon::update_all()
{
    EDE_Config &iconConfig = *cfg;
    iconConfig.read_file(false);
    iconConfig.set_section("Desktop Entry");

    // Icon Label:
    icon_name = get_localized_name(iconConfig);
    tooltip(icon_name);
    label(icon_name);

    label_color(label_foreground);
    label_size(label_fontsize);

    lwidth = label_maxwidth; // This is a bit strange, but otherwise we get mysterious crashes...
    lheight= 0;
    fltk::font(label_font(), label_size());
    fltk::measure(icon_name, lwidth, lheight, fltk::ALIGN_WRAP);
    lwidth += 4; //  height+= 4;

    // Icon file:
    iconConfig.read("Icon", icon_file, "folder.png");

    update_icon();

    redraw();
    //desktop->redraw();
}

void save_icon(Icon *i_window)
{
    if(i_name->size()==0) {
        fltk::alert(_("Name of the icon must be filled."));
    }
    else
    {
        const char *icon_file = i_filename->value();
        const char *icons_path = PREFIX"/share/ede/icons/48x48";
        if(!strncmp(icons_path, i_filename->value(), strlen(icons_path))) {
            // Only relative path, if icon in default location
            icon_file = fltk::filename_name(i_filename->value());
        }

        EDE_Config i_config(i_link->value());
        i_config.set_section("Desktop Entry");

        i_config.write(get_localized_string(), i_name->value());
        i_config.write("Name", i_name->value());	// fallback
        i_config.write("Exec", i_location->value());
        i_config.write("Icon", icon_file);
        i_config.flush();
        i_window->update_all();
    }
}

void delete_icon(Fl_Widget*, Icon *icon)
{
    if (fltk::ask(_("Delete this icon?"))) 
    {
        icon->hide();
        char fname[fltk::PATH_MAX];
        strncpy(fname, icon->get_cfg()->filename(), fltk::PATH_MAX);
        delete icon;
        if(remove(fname) < 0)
            fltk::alert(_("Remove of the icon %s failed. You probably do not have write access to this file."), fname);
    }
}

int create_new_icon()
{
    int ix=fltk::event_x_root();
    int iy=fltk::event_y_root();
    Icon *icon=0;
    const char *i = fltk::input(_("Enter the name of the new icon:"), "The Gimp");
    if (i)
    {
        char config[fltk::PATH_MAX];
        snprintf(config, sizeof(config)-1, "%s/.ede/desktop/%s.desktop", getenv("HOME"), i);

        if(!fltk::file_exists(config))
        {
            EDE_Config cfg(config);
            cfg.set_section("Desktop Entry");
            cfg.write("Icon", "no icon");
            cfg.write("X", ix);
            cfg.write("Y", iy);
            cfg.write(get_localized_string(), i);
            cfg.write("Exec", "Executable Here");
            //const char *u = fl_input(_("Enter the program name or the location to open:"), "gimp");
            cfg.flush();

            desktop->begin();
            icon = new Icon(config);
            desktop->end();
        }
        else {
            fltk::alert(_("The icon with the same name already exists."));
        }
    }
    if(icon) {
        property_dialog(0, icon, true);
        icon->position(ix,iy);
        icon->show();

        desktop->redraw();
        desktop->relayout();
    }
    return 0;
}

void update_iconeditdialog(Icon *i)
{ 
    i_link->value(i->get_cfg()->filename());

    EDE_Config &i_config = *i->get_cfg();

    char* val;
    i_config.set_section("Desktop Entry");

    val = get_localized_name(i_config);
    if(strlen(val)>0) {
        i_name->value(val);
    }

    if(!i_config.read("Exec", val, 0)) {
        i_location->value(val);
    }

    if(!i_config.read("Icon", val, 0)) {
        i_filename->value(val);
    }
}

void update_property_dialog(Icon *i)
{
    char* val;

    EDE_Config i_config(i->get_cfg()->filename());
    i_config.set_section("Desktop Entry");

    val = get_localized_name(i_config);
    if(!val.empty()) {
        pr_name->label(val);
    }

    if(!i_config.read("Exec", val, 0)) {
        pr_exec->label(val);
    }

/* TODO
    Fl_FileAttr *attr = fl_file_attr(i->get_cfg()->filename());
    if(attr)
    {
        char size[32];
        snprintf(size, 32, _("%d bytes, %s"), (int) attr->size, attr->time);
        pr_size->label(size);
        delete attr;
    }*/

    pr_icon->image(i->icon_im);
}

MovableIcon::MovableIcon(Icon *i)
: fltk::Window(desktop->x()+i->x(), desktop->y()+i->y(), i->w(), i->h())
{
    icon = i;
    set_override();
    create();

    fltk::Image *im = i->icon_im;
    if(im)
    {
        Pixmap mask = im->create_mask(im->width(), im->height());
        XShapeCombineMask(fltk::xdisplay, fltk::xid(this), ShapeBounding, 0, 0, mask, ShapeSet);

        align(fltk::ALIGN_INSIDE);
        image(im);
    }
}

MovableIcon::~MovableIcon()
{
}
