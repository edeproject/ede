/*
 * $Id$
 *
 * Colors and fonts settings
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <unistd.h>
#include <stdlib.h>

//#include <efltk/fl_draw.h>
#include <fltk/x.h> //#include <efltk/x.h>
#include "../edelib2/Run.h" //#include <efltk/Fl_Util.h>
//#include <efltk/Fl_String.h>

#include "ecolorutils.h"
#include "ecolorconf.h"
#include "EDE_FontChooser.h"



using namespace fltk;
using namespace edelib;

////////////////////////
// Useful functions from efltk
////////////////////////

char *my_get_homedir() {
   char *path = new char[PATH_MAX];
	const char *str1;

    str1=getenv("HOME");
    if (str1) {
         memcpy(path, str1, strlen(str1)+1);
         return path;
    }

    return 0;
}


////////////////////////



////////////////////////
// Useful functions that should be in fltk
////////////////////////

char *filename_noext(char *buf)
{
	char *p=buf, *q=0;
	while (*p++) 
	{
		if (*p == '/') q = 0;
#if defined(_WIN32) || defined(__EMX__)
		else if (*p == '\\') q = 0;
#endif
		else if (*p == '.') q = p;
	}
	if (q) *q='\0';
	return buf;
}


////////////////////////



EDEFont labelfont, textfont;

static void sendClientMessage(Window w, Atom a, long x)
{
// no worky
/*  XEvent ev;
  long mask;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = CurrentTime;
  mask = 0L;
  if (w == RootWindow(fl_display, fl_screen))
    mask = SubstructureRedirectMask;	   
  XSendEvent(fl_display, w, False, mask, &ev);*/
}

void sendUpdateInfo(Atom what)
{
// no worky
/*    unsigned int i, nrootwins;
    Window dw1, dw2, *rootwins = 0;
    int screen_count = ScreenCount(fl_display);
    for (int s = 0; s < screen_count; s++) {
        Window root = RootWindow(fl_display, s);
        XQueryTree(fl_display, root, &dw1, &dw2, &rootwins, &nrootwins);
        for (i = 0; i < nrootwins; i++) {
            if (rootwins[i]!=RootWindow(fl_display, fl_screen)) {
                sendClientMessage(rootwins[i], what, 0);
            }
        }
    }*/
}

// Comment from before
/*
void updateSizes(Fl_Input_Browser *font_sizes)
{
    int *sizes;
    int cnt = fl_font()->sizes(sizes);

    font_sizes->clear();
    font_sizes->begin();

    char tmp[8];
    for(int n=0; n<cnt; n++)
    {
        snprintf(tmp, sizeof(tmp)-1, "%d", sizes[n]);
        Fl_Item *i = new Fl_Item();
        i->copy_label(tmp);
    }
    font_sizes->end();
}

void updateEncodings(Fl_Input_Browser *font_combo)
{
    int encs;
    const char **array;
    encs = fl_font()->encodings(array);

    fontEncoding->clear();
    fontEncoding->begin();
    for(int n=0; n<encs; n++)
    {
        new Fl_Item(array[n]);
    }
    fontEncoding->end();
}

void updateFontChange(Fl_Input_Browser *font_combo, Fl_Input_Browser *font_sizes)
{
    //Fl_Font f = fl_find_font(font_combo->value());
    Fl_Font f = fl_create_font(font_combo->value());    
    int s = (int)atoi(font_sizes->value());

    if(!f) return;

    fl_font(f,s);

    updateSizes(font_sizes);
    updateEncodings(font_combo);
}

void updateFontAll()
{
    updateFontChange(labelFontInput, labelSize);
    updateFontChange(textFontInput, textSize);
}*/


void apply_colors_apps(Color fg, Color bg, Color text, const char* font)
{
    uchar r, g, b, r1, g1, b1, r2, g2, b2;
    split_color(bg, r, g, b);
    split_color(fg, r1, g1, b1);
    split_color(text, r2, g2, b2);

    char filePath[PATH_MAX];
    snprintf(filePath,PATH_MAX,"%s/.Xdefaults",my_get_homedir());
    
    char *backgroundTypes[34] = 
    {
    "*XmList.background" ,    "*XmLGrid.background",
    "Netscape*XmList.background" ,   "Netscape*XmLGrid.background",
    "*text*background",   "*list*background",
    "*Text*background",   "*List*background", 
    "*textBackground",   "*XmTextField.background", 
    "*XmText.background",     "Netscape*XmTextField.background", 
    "Netscape*XmText.background",     "*background", 
    "*Background",  "nscal*Background",
    "*Menu*background",     "OpenWindows*WindowColor",
    "Window.Color.Background",   "netscape*background",
    "Netscape*background",   ".netscape*background",
    "Ddd*background",   "Emacs*Background",
    "Emacs*backgroundToolBarColor",//25 
    "*XmList.selectBackground" ,   "*XmLGrid.selectBackground",
    "Netscape*XmList.selectBackground" ,  "Netscape*XmLGrid.selectBackground",
    "*XmTextField.selectBackground",  "*XmText.selectBackground", 
    "Netscape*XmTextField.selectBackground",  "Netscape*XmText.selectBackground", 
    "*selectBackground" //34
		   
    };	

    FILE *colorFile = fopen(filePath, "w");
    for (int i = 0 ; i < 34; i++)
    {
        fprintf(colorFile, "%s:  #%02X%02X%02X\n", backgroundTypes[i],(short int) r, (short int) g, (short int) b);
    }	
    fprintf(colorFile, "foreground:  #%02X%02X%02X\n", r1, g1, b1);
    fprintf(colorFile, "xterm*background:  #FFFFFF\n");	//especialy for Xterm
    fclose(colorFile);

    char runString[PATH_MAX];
    snprintf(runString,PATH_MAX,"xrdb -merge -all %s/.Xdefaults",my_get_homedir());
    
//    if (fl_start_child_process(runString)==-1)
    if (run_program(runString)>255)
	alert("Error executing xrdb program.");
}


void apply_colors_gtk(Color fg, 
		      Color bg, 
		      Color selection, 
		      Color selection_text, 
		      Color tooltip, 
		      Color tooltip_text, 
		      
		      Color text, 		      
		      const char* font)
{
    uchar r, g, b;
    uchar text_r, text_g, text_b;
    //, b1, r2, g2, b2;
    
    uchar selection_r, selection_g, selection_b;
    uchar selection_text_r, selection_text_g, selection_text_b;
    uchar tooltip_r, tooltip_g, tooltip_b;
    uchar tooltip_text_r, tooltip_text_g, tooltip_text_b;
    
    split_color(bg, r, g, b);
    split_color(fg, text_r, text_g, text_b);

    split_color(selection, selection_r, selection_g, selection_b);
    split_color(selection_text, selection_text_r, selection_text_g, selection_text_b);
    split_color(tooltip, tooltip_r, tooltip_g, tooltip_b);
    split_color(tooltip_text, tooltip_text_r, tooltip_text_g, tooltip_text_b);
    
//    fl_get_color(text, r2, g2, b2);

    char filePath[PATH_MAX];
    snprintf(filePath,PATH_MAX,"%s/.gtkrc",my_get_homedir());

    FILE *gtkFile = fopen(filePath, "w");
    
    fprintf(gtkFile, "style \"default\" \n");
    fprintf(gtkFile, "{\n");
    fprintf(gtkFile, "fontset = \"%s\" \n", font);
    fprintf(gtkFile, "bg[NORMAL] = \"#%02X%02X%02X\"\n", r, g, b);
    fprintf(gtkFile, "fg[NORMAL] = \"#%02X%02X%02X\"\n", text_r, text_g, text_b);
    fprintf(gtkFile, "bg[PRELIGHT] = \"#%02X%02X%02X\"\n", r, g, b);
    fprintf(gtkFile, "fg[PRELIGHT] = \"#%02X%02X%02X\"\n", text_r, text_g, text_b);
    fprintf(gtkFile, "bg[ACTIVE] = \"#%02X%02X%02X\"\n", r, g, b);
    fprintf(gtkFile, "fg[ACTIVE] = \"#%02X%02X%02X\"\n", text_r, text_g, text_b);
    fprintf(gtkFile, "bg[SELECTED] = \"#%02X%02X%02X\"\n", selection_r, selection_g, selection_b);
    fprintf(gtkFile, "fg[SELECTED] = \"#%02X%02X%02X\"\n", selection_text_r, selection_text_g, selection_text_b);
    fprintf(gtkFile, "}\n");
    
    fprintf(gtkFile, "style \"menu\" \n");
    fprintf(gtkFile, "{\n");
    fprintf(gtkFile, "bg[PRELIGHT] = \"#%02X%02X%02X\"\n", selection_r, selection_g, selection_b);
    fprintf(gtkFile, "fg[PRELIGHT] = \"#%02X%02X%02X\"\n", selection_text_r, selection_text_g, selection_text_b);
    fprintf(gtkFile, "}\n");

    fprintf(gtkFile, "style \"tooltip\" \n");
    fprintf(gtkFile, "{\n");
    fprintf(gtkFile, "bg[NORMAL] = \"#%02X%02X%02X\"\n", tooltip_r, tooltip_g, tooltip_b);
    fprintf(gtkFile, "fg[NORMAL] = \"#%02X%02X%02X\"\n", tooltip_text_r, tooltip_text_g, tooltip_text_b);
    fprintf(gtkFile, "}\n");
    
    fprintf(gtkFile, "class \"*\" style \"default\"\n");
    fprintf(gtkFile, "widget_class \"*Menu*\" style \"menu\"  \n");
    fprintf(gtkFile, "widget \"gtk-tooltips\" style \"tooltip\"  \n");
    
    
    fclose(gtkFile);
}


void apply_colors_qt(Color fg, Color bg, Color text, const char* font)
{
    uchar r, g, b, r1, g1, b1, r2, g2, b2;
    split_color(bg, r, g, b);
    split_color(fg, r1, g1, b1);
    split_color(text, r2, g2, b2);

    char filePath[PATH_MAX];
    snprintf(filePath,PATH_MAX,"%s/.qt/qtrc",my_get_homedir());

    FILE *qtfile = fopen(filePath, "w");
    
    fprintf(qtfile, "[General]\n");
    fprintf(qtfile, "GUIEffects=none^e\n");
    fprintf(qtfile, "style=Windows\n\n");
    fprintf(qtfile, "[Palette]\n");
    fprintf(qtfile, "active=#000000^e#%02x%02x%02x^e#ffffff^e#%02x%02x%02x^e#000000^e"
	    "#%02x%02x%02x^e#000000^e#ffffff^e#000000^e#ffffff^e#%02x%02x%02x^e#000000^e"
	    "#7783bd^e#ffffff^e#0000ff^e#ff00ff^e\n",
	    r,g,b, r,g,b, r,g,b, r,g,b);
    fprintf(qtfile, "disabled=#808080^e#%02x%02x%02x^e#ffffff^e#f2f2f2^e#%02x%02x%02x^e"
	    "#b7b7b7^e#b7b7b7^e#ffffff^e#000000^e#ffffff^e#dcdcdc^e#000000^e"
	    "#000080^e#ffffff^e#0000ff^e#ff00ff^e\n", 
	    r,g,b, r,g,b);
    fprintf(qtfile, "inactive=#000000^e#%02x%02x%02x^e#ffffff^e#f2f2f2^e#%02x%02x%02x^e"
		    "#b7b7b7^e#000000^e#ffffff^e#000000^e#ffffff^e#dcdcdc^e"
		    "#000000^e#7783bd^e#ffffff^e#0000ff^e#ff00ff^e\n",
		    r,g,b, r,g,b);

    fclose(qtfile);
}


void apply_colors_kde(Color fg, Color bg, Color text, const char* font)
{
    uchar r, g, b, r1, g1, b1, r2, g2, b2;
    split_color(bg, r, g, b);
    split_color(fg, r1, g1, b1);
    split_color(text, r2, g2, b2);

    char filePath[PATH_MAX];
    snprintf (filePath,PATH_MAX,"%s/.kderc",my_get_homedir());

    FILE *kdefile = fopen(filePath, "w");
    
    fprintf(kdefile, "[General]\n");
    fprintf(kdefile, "background=%d,%d,%d\n", r, g, b);
    fprintf(kdefile, "foreground=%d,%d,%d\n", r1, g1, b1);
    
    fclose(kdefile);
}

void saveScheme(char *scheme)
{
    char *keys[] = 
    {
        "color", "label color", "selection color",
        "selection text color", "highlight color", "text color",
        "highlight label color",
    };
    Button *colorBoxes[7] = 
    {
        colorBox, labelColorBox, selectionColorBox, selectionTextColorBox,
        highlightColorBox, textColorBox, highlightLabelColorBox
    };

// We can save new scheme even if there is no existing
//    if (schemeListBox->size() > 1) 
//    {
        if (colorBox->color() == labelColorBox->color()) 
	{	alert(_("Color and label color are the same. Edit colors first."));
	}
        else 
	{
            Config colorConfig(scheme); //save to "active".scheme

            colorConfig.set_section("widgets/default");
            for (int boxIndex=0; boxIndex<7; boxIndex++) {
                colorConfig.write(keys[boxIndex], (int)colorBoxes[boxIndex]->color());
            }

            colorConfig.write("text background", (int)textBackgroundBox->color());

	    // we don't want to lose leading space...
	    char tr[128];
	    strncpy (tr, labelfont.font->system_name(), 128);
	    if (tr[0] == ' ') tr[0] = '_';
            colorConfig.write("label font", tr);
	    strncpy (tr, textfont.font->system_name(), 128);
	    if (tr[0] == ' ') tr[0] = '_';
	    colorConfig.write("text font", tr);

            colorConfig.write("label size", labelfont.size);
            colorConfig.write("text size",  textfont.size);
	    colorConfig.write("font encoding",  textfont.encoding);

            colorConfig.set_section("widgets/tooltip");
	    colorConfig.write("color", (int)tooltipBox->color());
            colorConfig.write("label color", (int)tooltipTextColorButton->color());

            colorConfig.set_section("global colors");
            colorConfig.write("background", (int)backgroundBox->color());
        }
//    }
}

void saveActiveScheme()
{
    char pathActive[PATH_MAX];
    snprintf(pathActive,PATH_MAX,"%s/.ede/schemes/Active.scheme",my_get_homedir());

    saveScheme(pathActive);
}

void saveSchemeAs()
{
    const char *schemeName = input(_("Save scheme as:"), _("New scheme"));
    if (schemeName) 
    {
	char pathScheme[PATH_MAX]; 
	//pathScheme.printf("%s/.ede/schemes/%s.scheme", fl_homedir().c_str(), schemeName);
	snprintf(pathScheme, PATH_MAX, "%s/.ede/schemes/%s.scheme", my_get_homedir(), schemeName);
	saveScheme(pathScheme);
	schemeListBox->add(filename_noext(filename_name(pathScheme)));
    }	
}

void applyColors() 
{
//    sendUpdateInfo(FLTKChangeScheme);

    if (allApplyRadioButton->value()==1)
    {
	apply_colors_apps(labelColorBox->color(), backgroundBox->color(), 
		    textBackgroundBox->color(), labelFontInput->label());
	apply_colors_gtk(labelColorBox->color(), backgroundBox->color(), 
			 selectionColorBox->color(), selectionTextColorBox->color(),
			 tooltipBox->color(), tooltipTextColorButton->color(),
			 textBackgroundBox->color(), labelFontInput->label()
	    );
	apply_colors_qt(labelColorBox->color(), backgroundBox->color(), 
	    textBackgroundBox->color(), labelFontInput->label());
	apply_colors_kde(labelColorBox->color(), backgroundBox->color(), 
	    textBackgroundBox->color(), labelFontInput->label());
    }
}

void fillItems() 
{
    char *file;

    char path[PATH_MAX];
    snprintf(path,PATH_MAX,"%s/.ede/schemes",my_get_homedir());

    if (access(path,0)) { mkdir( path, 0777 ); }

    dirent **files;
    int count = filename_list(path, &files);

// We should always have an "active" scheme, even if directory is empty
//    if (count > 0)
//    {
        new Item("Active");
	schemeListBox->text("Active");


        for(int n=0; n<count; n++)
        {
            file = files[n]->d_name;
            if( strcmp(file, ".")!=0 && strcmp(file, "..")!=0)
            {
		char filename[PATH_MAX];
		snprintf(filename,PATH_MAX,"%s/%s", path, file);
                if (!filename_isdir(filename) &&
                    filename_match(file, "*.scheme") && strcmp(file, "Active.scheme")!=0) 
		{
                    new Item(strdup(filename_noext(filename_name(filename))));
                }
            }
            free(files[n]);
        }
        free(files);
	getSchemeColors(); //we apply first scheme - active.scheme
//    }

}

void getSchemeColors()
{
// Hardcoded defaults are below, inside read() calls

    char tr[128];
    int ir = 0;
    char *keys[] = 
    {  
        "color", "label color", "selection color",
        "selection text color", "highlight color", "text color",
        "highlight label color",
    };
    long keys_defaults[] = 
    {
        7, 32, 796173568, 7, 49, 32, 32
    };
    Button *colorBoxes[7] = 
    {
        colorBox, labelColorBox, selectionColorBox, selectionTextColorBox,
        highlightColorBox, textColorBox, highlightLabelColorBox
    };
// We always have at least "Active" on the list
//    if (schemeListBox->size() > 1)
//    {
        Config *colorConfig;

        const char *ai = schemeListBox->text();
        if (strcmp(ai, "Active")==0)
        {
            char pathActive[PATH_MAX];
            snprintf(pathActive, sizeof(pathActive)-1, "%s/.ede/schemes/Active.scheme", my_get_homedir());
            colorConfig = new Config(pathActive);
        } else {
            char pathScheme[PATH_MAX];
            snprintf(pathScheme, sizeof(pathScheme)-1, "%s/.ede/schemes/%s.scheme", my_get_homedir(), ai);
            // However, sometimes a bogus entry is selected:
            if (!filename_exist(pathScheme)) return;
            colorConfig = new Config(pathScheme);
        }

        for(int boxIndex = 0; boxIndex < 7; boxIndex++)
	{
            colorConfig->set_section("widgets/default");
            colorConfig->read(keys[boxIndex], ir, keys_defaults[boxIndex]);
            colorBoxes[boxIndex]->color((Color)ir);
            colorBoxes[boxIndex]->highlight_color((Color)ir);
        }

        colorConfig->set_section("widgets/tooltip");
        colorConfig->read("color", ir, -16784896);
        tooltipBox->color((Color)ir);
        tooltipBox->highlight_color((Color)ir);
        
        colorConfig->read("label color",ir, 32);
        tooltipTextColorButton->color((Color)ir);
        tooltipTextColorButton->highlight_color((Color)ir);
        
        colorConfig->set_section("widgets/default");
        colorConfig->read("text background", ir, 7);
        textBackgroundBox->color((Color)ir);
        textBackgroundBox->highlight_color((Color)ir);
        
        char tmpencoding[PATH_MAX];
        colorConfig->read("font encoding", tr, "iso8859-2", sizeof(tr)); strncpy(tmpencoding,tr,PATH_MAX);

        colorConfig->read("label font", tr, fltk::HELVETICA->name(), sizeof(tr));
	{
		if (tr[0] == '_') tr[0] = ' ';	// converted leading space
		fltk::Font* thefont = font(tr); //Style.h
		labelfont.font = thefont;
		if (labelfont.encoding) free(labelfont.encoding);
		labelfont.encoding = strdup(tmpencoding);
		labelfont.defined = true;

		colorConfig->read("label size", ir, 12);
		labelfont.size = ir;
	}

	colorConfig->read("text font", tr, fltk::HELVETICA->name(), sizeof(tr));
	{
		if (tr[0] == '_') tr[0] = ' ';
		fltk::Font* thefont = font(tr);
		textfont.font = thefont;
		if (textfont.encoding) free(textfont.encoding);
		textfont.encoding = strdup(tmpencoding);
		textfont.defined = true;

		colorConfig->read("text size", ir, 12);
		textfont.size = ir;
	}

	labelFontInput->label(font_nice_name(labelfont));
	textFontInput->label(font_nice_name(textfont));

        colorConfig->set_section("global colors");
        colorConfig->read("background", ir, -673724416);
        backgroundBox->color((Color)ir);

        colorBox->parent()->parent()->redraw();

        delete colorConfig;
//    }
}

void loadEfltkConfig()
{
    char *file = 0;
    file = Config::find_file("efltk.conf", false, Config::USER);
    if(!file) file = Config::find_file("efltk.conf", false, Config::SYSTEM);

    Config cfg(file, true, false);
    if(!cfg.error()) 
    {
        bool b_val;
        float f_val;
        int i_val;

        // Read Fl_Image defaults:
        cfg.get("Images", "State Effects", b_val, true);
        imagesStateEffect->value(b_val);

        // Read Fl_Menu_Window defaults:
        cfg.get("Menus", "Effects", b_val, true);
        menusEnableEffects->value(b_val);
        cfg.get("Menus", "Subwindow Effect", b_val, true);
        menusEnableSubwindowEffects->value(b_val);
        cfg.get("Menus", "Effect Type", i_val, 1);
        menusEffectType->value(i_val);
        cfg.get("Menus", "Speed", f_val, 1.5f);
        menusSpeed->value(f_val);
        cfg.get("Menus", "Delay", f_val, 0.2f);
        menusDelay->value(f_val);


        // Read Fl_Tooltip defaults:
        cfg.get("Tooltips", "Effects", b_val, true);
        tooltipsEnableEffects->value(b_val);
        cfg.get("Tooltips", "Effect Type", i_val, 2);
        tooltipsEffectType->value(i_val);
        cfg.get("Tooltips", "Enabled", b_val, true);
        tooltipsEnable->value(b_val);
        cfg.get("Tooltips", "Delay", f_val, 1.0f);
        tooltipsDelay->value(f_val);

        // Read Fl_MDI_Window defaults:
        cfg.get("MDI", "Animate", b_val, true);
        mdiAnimation->value(b_val);
        cfg.get("MDI", "Opaque", b_val, false);
        mdiOpaqueAnimation->value(b_val);
    } 
}    

void saveEfltkConfig()
{
    char *file = 0;
    file = Config::find_file("efltk.conf", false, Config::USER);
    if(!file) file = Config::find_file("efltk.conf", false, Config::SYSTEM);

    Config cfg(file, true, true);
    if(!cfg.error()) 
    {
        cfg.set("Images", "State Effects", imagesStateEffect->value());
	
        cfg.set("Menus", "Effects", menusEnableEffects->value());
        cfg.set("Menus", "Subwindow Effect", menusEnableSubwindowEffects->value());
        cfg.set("Menus", "Effect Type", menusEffectType->value());
        cfg.set("Menus", "Speed", (float)menusSpeed->value());
        cfg.set("Menus", "Delay", (float)menusDelay->value());

        cfg.set("Tooltips", "Effects", tooltipsEnableEffects->value());
        cfg.set("Tooltips", "Effect Type", tooltipsEffectType->value());
        cfg.set("Tooltips", "Enabled", tooltipsEnable->value());
        cfg.set("Tooltips", "Delay", (float)tooltipsDelay->value());

        cfg.set("MDI", "Animate", mdiAnimation->value());
        cfg.set("MDI", "Opaque", mdiOpaqueAnimation->value());

       // sendUpdateInfo(FLTKChangeSettings);
    }
}


// FONT STUFF:
// returns nice name for a font
const char* font_nice_name(EDEFont font) {
	if (!font.defined)
		return "Unknown";

	char nicename[PATH_MAX];
	snprintf (nicename, PATH_MAX, "%s (%d)", font.font->name(), font.size);

	// capitalize bold, italic
//	nicename.sub_replace("bold","Bold");
//	nicename.sub_replace("italic","Italic");

//	nicename = nicename + " (";
//	nicename = nicename + Fl_String(font.size);
//	nicename = nicename + ")";
	
	const char* n = strdup(nicename);
	return n;
}


// callback for button to set label font
void labelfont_cb() {
	EDEFont ret = font_chooser(labelfont);

	if (ret.defined) {
		labelfont = ret;
		labelFontInput->label(font_nice_name(labelfont));
		labelFontInput->redraw();
	}
}


// callback for button to set label font
void textfont_cb() {
	EDEFont ret = font_chooser(textfont);
	
	if (ret.defined) {
		textfont = ret;
		textFontInput->label(font_nice_name(textfont));
		textFontInput->redraw();
	}
}
