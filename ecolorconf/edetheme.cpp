//
// "$Id$"
//
// Startup, scheme and theme handling code for the Fast Light
// Tool Kit (FLTK).
//
// Copyright 1998-1999 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
//

// The "scheme" theme. This reads an earlier design for configuring fltk,
// a text-based "scheme" file, which described exactly what to put into
// the style structures for each widget class. We rejected this design
// because it was apparent that all interesting themes were completely
// defined by plugin code and thus the only part that was being used was
// the "themes" line from the file.

// The scheme argument (set by Fl_Style::scheme() or by the -scheme
// switch when Fl::arg() is used) is used to choose the scheme file to
// read, by adding ".scheme" to the end. If not specified or null,
// "default" is used.  There are some sample scheme files provided for
// your amusement, such as OldMotif.scheme.
//
// Modified for use with EDE by Martin Pekar 07/02/2002


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <fltk/Fl.h>
#include <fltk/fl_load_plugin.h>
#include <fltk/Fl_Color.h>
#include <fltk/Fl_Font.h>
#include <fltk/Fl_Labeltype.h>
#include <fltk/Fl_Style.h>
#include <fltk/Fl_Widget.h>
#include <fltk/fl_draw.h>
#include <fltk/x.h>

#include <ede/FLE_Config.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define access(a,b) _access(a,b)
#define R_OK        04
#endif

#ifndef PATH_MAX
#define PATH_MAX 128
#endif

static Fl_Color grok_color(FLE_Config* cf, const char *colstr)
{
    char *val=0;
    const char *p = colstr;
    val = cf->read_string("aliases", colstr);
    if(val) p = val;
    char* q;
    long l = strtoul(p, &q, 0);
    if(!*q) return (Fl_Color)l;
    //if(val) delete []val; //LEAK!!
    return fl_rgb(p);
}

static Fl_Font grok_font(FLE_Config *cf, const char* fontstr)
{
    char *val;
    const char *p = fontstr;
    val = cf->read_string("aliases", fontstr);
    if(val) p = val;
    char* q;
    long l = strtoul(p, &q, 0);
    if(!*q) return fl_fonts+l;
    //if(val) delete []val; //LEAK!!
    return fl_find_font(p);
}

////////////////////////////////////////////////////////////////

extern "C"
bool fltk_theme()
{
    char temp[PATH_MAX];
    /*  const char* scheme = Fl_Style::scheme();
     if (!scheme || !*scheme) scheme = "default";

     char temp[PATH_MAX];
     snprintf(temp, PATH_MAX, "%s.scheme", scheme);
     char sfile_buf[PATH_MAX];*/
    const char* sfile = fle_find_config_file("schemes/Active.scheme", 0);
    if (!sfile) {
        fprintf(stderr, "Cannot find default scheme \"%s\"\n", sfile);
        return false;
    }

    static bool recurse=false;
    if (recurse) {
        fprintf(stderr, "%s recusively loaded scheme.theme\n", sfile);
        return false;
    }

    //conf_clear_cache();
    FLE_Config conf(sfile);

    //if (!::getconf(sfile, "general/themes", temp, sizeof(temp)))
    char *themefile = conf.read_string("general", "themes");
    if(themefile && !conf.error())
    {
        recurse = true;
        Fl_Theme f = Fl_Style::load_theme(themefile);
        if(f) f();
        else fprintf(stderr,"Unable to load %s theme\n", themefile);
        recurse = false;
        delete []themefile;
    }

    char *valstr;
    Fl_Color col;

    //if(!::getconf(sfile, "global colors/background", valstr, sizeof(valstr))) {
    valstr = conf.read_string("global colors", "background");
    if(valstr && !conf.error()) {
        col = grok_color(&conf, valstr);
        fl_background(fl_get_color(col));
        delete []valstr;
    }

    static struct { const char* key; Fl_Color col; } colors[] = {
        { "DARK1", FL_DARK1 },
        { "DARK2", FL_DARK2 },
        { "DARK3", FL_DARK3 },
        { "LIGHT1", FL_LIGHT1 },
        { "LIGHT2", FL_LIGHT2 },
        { "LIGHT3", FL_LIGHT3 },
        { 0, 0 }
    };

    for (int i = 0; colors[i].key; i++) {
        snprintf(temp, sizeof(temp)-1, "%s", colors[i].key);
        //int res = ::getconf(sfile, temp, valstr, sizeof(valstr));
        valstr = conf.read_string("global colors", temp);
        int res = conf.error();
        if(!res && valstr) {
            col = grok_color(&conf, valstr);
            fl_set_color(colors[i].col, col);
            delete []valstr;
        }
    }

    //conf_list section_list = 0, key_list = 0;
    //conf_entry* cent;
    SectionList *section_list;
    Section *cent=0;

    Fl_Font font;
    Fl_Labeltype labeltype;
    Fl_Boxtype boxtype;

    //if(!getconf_sections(sfile, "widgets", &section_list))
    section_list = conf.section_list("widgets");
    if(section_list)
    {
        //for (cent = section_list; cent; cent = cent->next)
        for(cent = section_list->first(); cent; cent=section_list->next())
        {
            //Fl_Style* style = Fl_Style::find(cent->key);
            Fl_Style* style = Fl_Style::find(cent->name);
            if(!style) continue;

            conf.set_section(cent);

            // box around widget
            //if(!getconf_list(key_list, "box", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("box")) ) {
                if ( (boxtype = Fl_Boxtype_::find(valstr)) ) style->box = boxtype;
                delete []valstr;
            }

            // box around buttons within widget
            //if (!getconf_list(key_list, "button box", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("button box")) ) {
                if ( (boxtype = Fl_Boxtype_::find(valstr)) ) style->button_box = boxtype;
                delete []valstr;
            }

            // color of widget background
            //if (!getconf_list(key_list, "color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("color")) ) {
                style->color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of widget's label
            //if (!getconf_list(key_list, "label color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("label color")) ) {
                style->label_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of widget's background when widget is selected
            //if (!getconf_list(key_list, "selection color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("selection color" )) ) {
                style->selection_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of widget's text when text selected
            // color of widget's label when widget selected
            // color of widget's glyph when widget selected and no glyph box
            //if (!getconf_list(key_list, "selection text color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("selection text color")) ) {
                style->selection_text_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of widget's background when widget is highlighted
            //if (!getconf_list(key_list, "highlight color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("highlight color" ))) {
                style->highlight_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of widget's label when widget highlighted
            // color of widget's glyph/text when widget highlighted and no text/glyph box
            //if (!getconf_list(key_list, "highlight label color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("highlight label color" ))) {
                style->highlight_label_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // color of text/glyph within widget
            //if (!getconf_list(key_list, "text color", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("text color")) ) {
                style->text_color = grok_color(&conf, valstr);
                delete []valstr;
            }

            // font used for widget's label
            //if (!getconf_list(key_list, "label font", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("label font")) ) {
                if ( (font = grok_font(&conf, valstr)) ) style->label_font = font;
                delete []valstr;
            }

            // font used for text within widget
            //if (!getconf_list(key_list, "text font", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("text font" )) ) {
                if ( (font = grok_font(&conf, valstr)) ) style->text_font = font;
                delete []valstr;
            }

            // type of widget's label
            //if (!getconf_list(key_list, "label type", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("label type" )) ) {
                if ( (labeltype = Fl_Labeltype_::find(valstr)) ) style->label_type = labeltype;
                delete []valstr;
            }

            // font size of widget's label
            //if (!getconf_list(key_list, "label size", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("label size")) ) {
                style->label_size = (int)strtol(valstr,0,0);
                delete []valstr;
            }

            // font size of text within widget
            //if (!getconf_list(key_list, "text size", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("text size")) ) {
                style->text_size = (int)strtol(valstr,0,0);
                delete []valstr;
            }

            // leading
            //if (!getconf_list(key_list, "leading", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("leading")) ) {
                style->leading = (int)strtol(valstr,0,0);
                delete []valstr;
            }

            // font encoding
            //if (!getconf_list(key_list, "font encoding", valstr, sizeof(valstr)))
            if( (valstr=conf.read_string("font encoding")) ) {
                fl_encoding(valstr);
                //delete []valstr; //LEAK??
            }

            //conf_list_free(&key_list);
        }
        //conf_list_free(&section_list);
    }

    return true;
}

//
// End of "$Id$".
//
