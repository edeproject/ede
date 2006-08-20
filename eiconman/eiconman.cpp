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

#include "eiconman.h"
#include "edeskconf.h"
#include "edeskicon.h"

/*#include <efltk/Fl_Directory_DS.h>
#include <efltk/Fl_WM.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Renderer.h>*/
#include <fltk/run.h>

#include "../edeconf.h"

int label_background =  46848;
int label_foreground = fltk::WHITE;
int label_fontsize = 12;
int label_maxwidth = 75;
int label_gridspacing = 16;
bool label_trans = false;
bool one_click_exec = 0;
bool auto_arr = false;

Desktop *desktop = 0;

void scanIcons(char *path)
{
/*    Fl_Directory_DS dds;
    dds.directory(path);
    dds.open();

    while(!dds.eof())
    {
        Fl_String name(dds["Name"].get_string());
        Fl_String filename(dds.directory() + name);

        dds.next();

        if(access(filename, R_OK)!=0) continue;

        if(fl_file_match(name, "*.desktop")) {
            Icon *desktopIcon = new Icon(filename);
            desktopIcon->show();
        }
    }*/
// Rewrite using fltk::filename_list
	dirent **files;
	int num_files = fltk::filename_list(path, &files); // no sort needed because icons have coordinates

	if (num_files <= 0)
		return;

	for (int i = 0; i < num_files; i++) {
		char filename[256]; //fltk::PATH_MAX - expected unqualified-id before numeric constant (?!?!)
		snprintf(filename,256,"%s/%s",path,files[i]->d_name);
		if (access(filename, R_OK)!=0) continue;
		if (fltk::filename_match(files[i]->d_name, "*.desktop")) {
			Icon *desktopIcon = new Icon(filename);
			desktopIcon->show();
		}
	}
}

void readIconsConfiguration()
{
    EDE_Config globalConfig(EDE_Config::find_config_file("ede.conf", 0), true, false);

    globalConfig.set_section("IconManager");
    globalConfig.read("Label Background", label_background, 46848);
    globalConfig.read("Label Transparent", label_trans, false);
    globalConfig.read("Label Foreground", label_foreground, fltk::WHITE);
    globalConfig.read("Label Fontsize", label_fontsize, 12);
    globalConfig.read("Label Maxwidth", label_maxwidth, 75);
    globalConfig.read("Gridspacing", label_gridspacing, 16);
    globalConfig.read("OneClickExec", one_click_exec, 0);
    globalConfig.read("AutoArrange", auto_arr, false);
}

void cb_update_workarea(fltk::Widget *, void *arg)
{
    ((Desktop*)arg)->update_workarea();
}

void refresh(fltk::Widget *, void *arg)
{
    desktop->update_icons();
}

void icons_properties(fltk::Widget *, void *arg)
{
    run_program("eiconsconf",false);
}

void desktop_properties(fltk::Widget *, void *arg);

Desktop::Desktop()
  : fltk::DoubleBufferWindow(0, 0, w(), h(), "EIconMan")
{
    //Fl_WM::callback(cb_update_workarea, this, Fl_WM::DESKTOP_WORKAREA);

    wpaper = 0;
    bg_color = fltk::BLUE;
    bg_opacity = 255;
    bg_mode = 0;

    //window_type(Fl_WM::DESKTOP);

    popup = new fltk::PopupMenu(0, 0, 0, 0);
    popup->type(fltk::PopupMenu::POPUP3);
    popup->begin();

    fltk::Item *icon;

    icon = new fltk::Item(_("&New desktop item"));
    icon->callback((fltk::Callback*)create_new_icon, 0);
    //icon->x_offset(18);

    icon = new fltk::Item(_("&Refresh"));
    icon->callback((fltk::Callback*)refresh, 0);
    //icon->x_offset(18);

    new fltk::Divider();

    icon = new fltk::Item(_("&Icons Settings "));
    icon->callback((fltk::Callback*)icons_properties, (void*) this);
    //icon->x_offset(18);

    icon = new fltk::Item(_("&Background Settings"));
    icon->callback((fltk::Callback*)desktop_properties, (void*) this);
    //icon->x_offset(18);


    popup->end();

    end();
    show();
}

Desktop::~Desktop()
{
    if(wpaper) delete wpaper;
}

void Desktop::update_workarea()
{
    int X=0,Y=0,W=w(),H=h();
    // Get current workarea
    //Fl_WM::get_workarea(X,Y,W,H);

    resize(X,Y,W,H);
    layout();
    if(auto_arr) auto_arrange();
}

void Desktop::update_icons()
{
    for(int n=0; n<children(); n++) {
        fltk::Widget *w = child(n);
        if(w==popup) continue;
        Icon *i = (Icon *)w;
        i->update_all();
    }
    if(auto_arr)
        auto_arrange();

    fltk::flush();
}

void Desktop::auto_arrange()
{
    int X=label_maxwidth/5+10;
    int Y=10;
    int H=h();
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *w = child(n);
        if(w==popup) continue;

        bool again = false;
        Icon *i = (Icon *)w;
        do {
            again = false;
            i->position(X, Y);
            Y += i->h() + i->lheight + label_gridspacing + 10;
            if((Y-10)>=H) {
                Y=10;
                X+=label_maxwidth;
                again = true;
            }
        } while(again);
    }
    relayout();
    redraw();
}

int create_new_dnd_icon(int x, int y, char *filename) //create icon from dnd data
{
    if (filename) 
    {
        char config[256]; //fltk::PATH_MAX - expected unqualified-id before numeric constant (?!?!)
	const char *name = fltk::filename_name(filename);
	snprintf(config, sizeof(config)-1, "%s/.ede/desktop/%s.desktop", getenv("HOME"), name);
	
        if (!fltk::filename_exist((char*)config))
        {
	    char val[256]; //fltk::PATH_MAX - expected unqualified-id before numeric constant (?!?!)
	    EDE_Config checkconf(filename, true, false);
	
	    if(!checkconf.get("Desktop Entry", "Exec", val, 0, sizeof(val))) 
	    {	
    	        EDE_Config cfg(config);
		cfg.set_section("Desktop Entry");
		
		checkconf.get("Desktop Entry", "Icon", val, "no icon", sizeof(val)); 
        	cfg.write("Icon", val);
        	cfg.write("X", x);
        	cfg.write("Y", y);
        	cfg.write(get_localized_string(), get_localized_name(checkconf));

                checkconf.get("Desktop Entry", "Exec", val, filename, sizeof(val));
        	cfg.write("Exec", val);
        	cfg.flush();
	    }
	    else
	    {
	        EDE_Config cfg(config);
    	        cfg.set_section("Desktop Entry");
        	cfg.write("Icon", "no icon");
        	cfg.write("X", x);
        	cfg.write("Y", y);
        	cfg.write(get_localized_string(), name);
                cfg.write("Exec", filename);
        	cfg.flush();
	    }	
	    desktop->begin();
            Icon *icon = new Icon((char*)config);
    	    icon->show();
    	    desktop->end();
    	    desktop->redraw();
	}
	else 
	{
	    fltk::alert(_("The icon with the same name already exists."));
	}  
    }  
    return 0;
}

int Desktop::handle(int event) 
{
    if (event == fltk::KEY)
    {
        const int numchildren = children();
        int previous = focus_index()>0?focus_index():0;
        int i;

        if (fltk::focus()==this || fltk::focus() && !contains(fltk::focus())) return 0;
        int key = navigation_key();
        if(key)
        for (i = previous;;) {
            if (key == fltk::LeftKey || key == fltk::UpKey)
            {
                if (i) --i;
                else
                {
                    if (parent()) return false;
                    i = numchildren-1;
                }
            }
            else
            {
                ++i;
                if (i >= numchildren)
                {
                    if (parent()) return false;
                    i = 0;
                }
            }
            if (i == previous) break;
            if (key == fltk::DownKey || key == fltk::UpKey)
            {
                // for up/down, the widgets have to overlap horizontally:
                fltk::Widget* o = child(i);
                fltk::Widget* p = child(previous);
                if (o->x() >= p->x()+p->w() || o->x()+o->w() <= p->x()) continue;
            }
            if (child(i)->take_focus()) {
                redraw();
                return true;
            }
        }

        return 1;
    }

    int ret = fltk::DoubleBufferWindow::handle(event);

    switch(event)
    {
    case fltk::PUSH:
        if(fltk::event_button()==3) {
            popup->fltk::Menu::popup(fltk::Rectangle(fltk::event_x_root(), fltk::event_y_root()));
        }
        break;
    case fltk::FOCUS:
    case fltk::UNFOCUS:
        return 1;
	
    case fltk::DND_ENTER:
        return 1;
    case fltk::DND_DRAG:
	    //cursor((fltk::Cursor*)26, 70, 96);
            cursor((fltk::Cursor*)26); // FIXME - colors
            //cursor(fltk::CURSOR_MOVE, 70, 96);
    case fltk::DND_RELEASE:
//          cursor(fltk::CURSOR_DEFAULT, fltk::BLACK, fltk::WHITE);
            cursor(fltk::CURSOR_DEFAULT); // FIXME - colors
	    return 1;
    case fltk::PASTE:
	    create_new_dnd_icon(fltk::event_x_root(), fltk::event_y_root(), (char*)fltk::event_text());
	    return 1;		
	
    default:
        break;
    }

    return ret;
}

static int iconChangeHandler(int e)
{
/*    if(!e && fltk::xevent.type==ClientMessage && fltk::xevent.xclient.message_type==FLTKChangeSettings)
    {
        readIconsConfiguration();
        desktop->update_icons();
        return 1;
    }
    return 0;*/
}

void Desktop::draw()
{
    int numchildren = children();

    if(wpaper) {
        wpaper->draw(fltk::Rectangle(0,0,w(),h()));
    } else {
        fltk::color(bg_color);
        fltk::fillrect(0,0,w(),h());
    }

    int n;
    for(n = numchildren; n--;) {
        fltk::Widget &w = *child(n);
        if(!fltk::not_clipped(fltk::Rectangle(w.x(), w.y(), w.w(), w.h())))
            continue;
        fltk::push_matrix();
        fltk::translate(w.x(), w.y());
        w.set_damage(fltk::DAMAGE_ALL|fltk::DAMAGE_EXPOSE);
        w.draw();
        w.set_damage(0);
        fltk::pop_matrix();
    }
}

WPaper *make_image(fltk::Color bg_color, fltk::SharedImage *im, int w, int h, int mode, uchar opacity);

void Desktop::update_bg()
{
    //Fl_Renderer::system_init();

    if (wpaper) {
        delete wpaper;
        wpaper=0;
    }

    EDE_Config globalConfig(EDE_Config::find_config_file("ede.conf", 0), true, false);
    globalConfig.set_section("Desktop");
//    globalConfig.read("Color", bg_color, (fltk::Color)fltk::darker(fltk::BLUE));
    globalConfig.read("Color", bg_color, fltk::BLUE);
    globalConfig.read("Opacity", bg_opacity, 255);
    globalConfig.read("Mode", bg_mode, 0);
    globalConfig.read("Use", bg_use, 1);

    fltk::SharedImage *im=0;

    if(!globalConfig.read("Wallpaper", bg_filename, 0)) {
        if(bg_use) { 
            im = fltk::SharedImage::get(bg_filename);
//            if (im) im->system_convert();
        }
    } else {
        free(bg_filename);
        bg_filename=0;
    }

    if(im) {
        wpaper = make_image(bg_color, im, w(), h(), bg_mode, bg_opacity);
        delete im;
    }
    redraw();
}

#include <signal.h>

static bool running = true;

void exit_signal(int signum)
{
    printf("Exiting (got signal %d)\n", signum);
    running = false;
}

int main(int argc, char ** argv) 
{
    signal(SIGTERM, exit_signal);
    signal(SIGKILL, exit_signal);
    signal(SIGINT, exit_signal);

//    fl_init_locale_support("eiconman", PREFIX"/share/locale");
    //fl_init_images_lib();

    desktop = new Desktop();
    desktop->begin();

    char homedir[256]; //fltk::PATH_MAX - expected unqualified-id before numeric constant (?!?!)
    snprintf(homedir,256,"%s/.ede/desktop/",getenv("HOME"));
    if(fltk::filename_isdir(homedir)) {
        scanIcons(homedir);
    }

    desktop->end();
    desktop->show();

    readIconsConfiguration();

    desktop->update_workarea();
    desktop->update_bg();
    desktop->update_icons();

    //fltk::add_handler(iconChangeHandler); FIXME

    while(running) fltk::wait();

    delete desktop;

    return 0;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

//WPaper::WPaper(int W, int H, Fl_PixelFormat *fmt)
//: Fl_Image(W,H,fmt,0)
WPaper::WPaper(int W, int H)
: fltk::SharedImage()
{
	w(W);
	h(H);
// FIXME weird stuff...
//    no_screen(true);
//    state_effect(false);
}

WPaper::~WPaper()
{
//    if(id) {
//        fltk::delete_offscreen((Pixmap)id);
//        id=0;
//    }
//    clear();
}

void WPaper::_draw(int dx, int dy, int dw, int dh,
                  int sx, int sy, int sw, int sh,
                  fltk::Flags f)
{
//    if(!id) fltk::Image::_draw(dx,dy,dw,dh,sx,sy,sw,sh,f);
//    if(id && m_data) {
//        delete m_data;
//        m_data=0;
//    }
//    if(id) {
//        // convert to Xlib coordinates:
//        fltk::transform(dx,dy);
//        fltk::copy_offscreen(dx,dy,dw,dh,(Pixmap)id,0,0);
//    }
}

WPaper *make_image(fltk::Color bg_color,
                     fltk::SharedImage *im,
                     int w, int h,
                     int mode,
                     uchar opacity=255)
{
    // secret box render function from Fl_Image :)
    //extern uint8 *render_box(int w, int h, int bitspp, uint color, fltk::Colormap *pal, uint8 *buffer);

    //Fl_PixelFormat fmt = Fl_Renderer::system_format();
    //WPaper *bg_image = new WPaper(w, h, Fl_Renderer::system_format());
    WPaper *bg_image = new WPaper(w, h);

    int iw=im->width(), ih=im->height();
    int ix=0, iy=0;
    int xoff=0, yoff=0;
    fltk::SharedImage *newim = im;

    switch(mode) {
    //CENTER
    case 0: {
        ix=(w/2)-(iw/2);
        iy=(h/2)-(ih/2);
        if(ix<0) xoff=-ix; if(iy<0) yoff=-iy;
        if(ix<0) ix=0; if(iy<0) iy=0;
    }
    break;

    //STRECH
    case 1: {
        ix=0, iy=0, iw=w, ih=h;
        if(w!=im->width()||h!=im->height()) {
            newim = im->scale(w,h);
        }
    }
    break;

    //STRETCH ASPECT
    case 2: {
        int pbw = w, pbh = h;
        iw = pbw;
        ih = iw * im->height() / im->width();
        if(ih > pbh) {
            ih = pbh;
            iw = ih * im->width() / im->height();
        }
        ix=(w/2)-(iw/2), iy=(h/2)-(ih/2);
        if(ix<0) ix=0; if(iy<0) iy=0;
        if(iw>w) iw=w; if(ih>h) ih=h;
        if(iw!=im->width()||ih!=im->height()) {
            newim = im->scale(iw,ih);
        }
    }
    break;
    }

    // This could be an option, opacity
    newim->format()->alpha = opacity;

    if( (iw<w || ih<h) || newim->format()->alpha!=255) {
        // If image doesnt fill the whole screen, or opacity < 255
        // fill image first with bg color.
        //render_box(w, h, fmt->bitspp, bg_color, fmt->palette, bg_image->data());
    }

    if(iw>w) iw=w; if(ih>h) ih=h;
    fltk::Rectangle r(xoff, yoff, iw, ih);
    fltk::Rectangle r2(ix,iy, iw, ih);

    if(newim->format()->alpha>0) {
        // Blit image data to our bg_image
        bg_image->check_map(newim->format());
        //fltk::Renderer::alpha_blit(newim->data(), &r, newim->format(), newim->pitch(),
        //                        bg_image->data(), &r2, bg_image->format(), bg_image->pitch(),
        //                        0);
    }

    if(newim!=im)
        delete newim;

    return bg_image;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

static bool shown = false, changed=false;
static WPaper *mini_wpaper=0;
static fltk::SharedImage *mini_image=0;
static int mode=0;
static bool use=true;
static uchar opacity=0;
static fltk::Color color=0;
static char* filename;

void desktop_properties(fltk::Widget *, void *arg)
{
    if(!shown) {
        make_desktop_properties();
        shown = true;
    }

    mode = desktop->bg_mode;
    color = desktop->bg_color;
    opacity = desktop->bg_opacity;
    use = desktop->bg_use;
    filename = desktop->bg_filename;

    if(bg_prop_window->visible()) {
        bg_prop_window->show();
        return;
    }

    if(mini_image) { delete mini_image; mini_image=0;  }
    if(mini_wpaper){ delete mini_wpaper; mini_wpaper=0;}

    color_button->selection_color(color);
    mini_image_box->color(color);
    opacity_slider->value(opacity);
    mode_choice->value(mode);
    use_button->value(use);
    image_input->value(filename);

    if(!desktop->bg_filename.empty())
    {
        fltk::SharedImage *im = fltk::SharedImage::read(desktop->bg_filename);
        if(im) {
	    im->system_convert();
            float scalew = (float)mini_image_box->w()/(float)desktop->w();
            float scaleh = (float)mini_image_box->h()/(float)desktop->h();
            mini_image = im->scale(int(im->width()*scalew), int(im->height()*scaleh));
            delete im;
        }
    }

    if(mini_image) {
        mini_wpaper = make_image(color, mini_image,
                                 mini_image_box->w(), mini_image_box->h(),
                                 mode, opacity);
        mini_image_box->image(mini_wpaper);
    } else
        mini_image_box->image(0);

    if(use)
        bg_image_group->activate();
    else {
        bg_image_group->deactivate();
        mini_image_box->image(0);
    }

    bg_prop_window->show();
}

void bg_image_use(fltk::CheckButton *w, void *d)
{
    changed=true;
    use = (bool)w->value();
    if(w->value()) {
        bg_image_group->activate();
        mini_image_box->image(mini_wpaper);
    } else {
        bg_image_group->deactivate();
        mini_image_box->image(0);
    }

    bg_image_group->redraw();
    mini_image_box->redraw();
}

void bg_image_mode(fltk::Choice *w, void *d)
{
    changed=true;
    mode = w->value();
    if(mini_wpaper) delete mini_wpaper;
    if(mini_image) {
        mini_wpaper = make_image(color, mini_image,
                                 mini_image_box->w(), mini_image_box->h(),
                                 mode, opacity);
        mini_image_box->image(mini_wpaper);
    } else {
        mini_image_box->image(0);
    }
    mini_image_box->redraw();
}

void bg_image_input(fltk::Input *w, void *d)
{
    changed=true;

//    Fl_String val(w->value());
//    filename = fl_file_expand(val);
    filename = strdup(w->value());

    if(mini_wpaper) { delete mini_wpaper; mini_wpaper=0; }
    if(mini_image)  { delete mini_image; mini_image=0; }

    fltk::SharedImage *im = fltk::SharedImage::read(filename);
    if(im) {
        im->system_convert();
        float scalew = (float)mini_image_box->w()/(float)desktop->w();
        float scaleh = (float)mini_image_box->h()/(float)desktop->h();
        mini_image = im->scale(int(im->width()*scalew), int(im->height()*scaleh));
        delete im;
    }

    if(mini_image) {
        mini_wpaper = make_image(color, mini_image,
                                 mini_image_box->w(), mini_image_box->h(),
                                 mode, opacity);
        mini_image_box->image(mini_wpaper);
    } else {
        mini_image_box->image(0);
    }

    image_input->value(filename);
    image_input->redraw();
    mini_image_box->redraw();
}

void bg_image_opacity(fltk::ValueSlider *w, void *d)
{
    changed=true;
    opacity = (uchar)w->value();

    if(mini_wpaper) delete mini_wpaper;
    if(mini_image) {
        mini_wpaper = make_image(color, mini_image,
                                 mini_image_box->w(), mini_image_box->h(),
                                 mode, opacity);
        mini_image_box->image(mini_wpaper);
    } else {
        mini_image_box->image(0);
    }
    mini_image_box->redraw();
}

void bg_image_browse(fltk::Button *w, void *d)
{
    changed=true;
    fc_initial_preview = true;
    const char *f=fltk::file_chooser(image_input->value(),
                                 _("All Files, *,"
                                  "Png Images, *.png,"
                                   "Xpm Images, *.xpm,"
                                   "Jpeg Images, *.{jpg|jpeg},"
                                   "Gif Images, *.gif,"
                                   "Bmp Images, *.bmp"),
                                 _("Choose wallpaper:")
                                );

    if(f) {
        if(mini_wpaper) { delete mini_wpaper; mini_wpaper=0; }
        if(mini_image)  { delete mini_image; mini_image=0; }

        filename = f;

        fltk::SharedImage *im = fltk::SharedImage::read(f);
        if(im) {
            im->system_convert();
            float scalew = (float)mini_image_box->w()/(float)desktop->w();
            float scaleh = (float)mini_image_box->h()/(float)desktop->h();
            mini_image = im->scale(int(im->width()*scalew), int(im->height()*scaleh));
            delete im;
        }

        if(mini_image) {
            mini_wpaper = make_image(color, mini_image,
                                     mini_image_box->w(), mini_image_box->h(),
                                     mode, opacity);
            mini_image_box->image(mini_wpaper);
        } else
            mini_image_box->image(0);

        image_input->value(filename);
        image_input->redraw();
        mini_image_box->redraw();
        delete []f;
    }
}

void bg_image_color(fltk::LightButton *w, void *d)
{
    changed=true;
    fltk::Color nc = color;
    if(!fltk::color_chooser(_("Choose color"), nc))
        return;

    color = nc;

    mini_image_box->color(nc);
    color_button->selection_color(color);

    if(mini_wpaper) delete mini_wpaper;
    if(mini_image) {
        mini_wpaper = make_image(color, mini_image,
                                mini_image_box->w(), mini_image_box->h(),
                                mode, opacity);
        mini_image_box->image(mini_wpaper);
    } else {
        mini_image_box->image(0);
    }
    mini_image_box->redraw();
}

void bg_prop_cb(fltk::Window *w, void *d)
{
    if(mini_wpaper) { delete mini_wpaper; mini_wpaper=0; }
    if(mini_image)  { delete mini_image; mini_image=0; }
    w->hide();

    shown = false;
    delete w;
}

void bg_apply(fltk::Button *w, void *d)
{
    if(changed) {
        EDE_Config globalConfig(EDE_Config::find_config_file("ede.conf", 1), true, true);
        globalConfig.set_section("Desktop");
        globalConfig.write("Color", color);
        globalConfig.write("Opacity", int(opacity));
        globalConfig.write("Wallpaper", filename);
        globalConfig.write("Mode", mode);
        globalConfig.write("Use", use);
        globalConfig.flush();

        desktop->update_bg();
    }
    changed=false;
}

void bg_ok(fltk::Button *w, void *d)
{
    bg_apply(w,d);
    bg_prop_window->destroy();
}
