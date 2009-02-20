// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "keyboardchooser.h"
#include "icons/keyboard.xpm"

#define MAX_KEYBOARDS 5

KeyboardChooser *kbcapplet;

static Fl_Image keyboard_pix((const char **)keyboard_xpm);

void setKeyboard(const Fl_String &pKbdLayout)
{
	if(!pKbdLayout.empty()) {
		Fl_String ApplyString("setxkbmap " + pKbdLayout);
		fl_start_child_process(ApplyString, false);
		
		Fl_String pShortKbd = pKbdLayout.sub_str(0, 2);
		kbcapplet->tooltip(pKbdLayout);
		kbcapplet->label(pShortKbd);
		kbcapplet->redraw();
	}
}

void CB_setKeyboard(Fl_Item *item, void *pData)
{
	Fl_Config pGlobalConfig(fl_find_config_file("ede.conf", true));
	if (!pGlobalConfig.error() && item) {
		Fl_String kbdname = item->field_name();
	
		pGlobalConfig.set("Keyboard", "Layout", kbdname);
		pGlobalConfig.flush();
		setKeyboard(kbdname);

		// update history
		Fl_String recentlist;
		pGlobalConfig.get("Keyboard", "RecentKeyboards", recentlist, "");
		Fl_String_List recentkbd(recentlist,"|");
		if (recentkbd.index_of(kbdname) > -1) return;
		
		Fl_String_List copylist;
		if (recentkbd.count() < MAX_KEYBOARDS)
			copylist.append(recentkbd.item(0));
		for (unsigned int i=1; i<recentkbd.count(); i++)
			copylist.append(recentkbd.item(i));
		copylist.append(kbdname);

		recentlist = copylist.to_string("|");
		pGlobalConfig.set("Keyboard", "RecentKeyboards", recentlist);
		pGlobalConfig.flush();
		
		// refresh menu list
		for (int i=0; i<kbcapplet->children(); i++) {
			if (kbcapplet->child(i)->field_name() == kbdname) return;
		}
		Fl_Item *mKbdItem = new Fl_Item(item->label());
		mKbdItem->field_name(kbdname);
		mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
		mKbdItem->image(keyboard_pix);
		kbcapplet->insert(*mKbdItem,0);
	}
}

// in case something fails, this function will produce a
// static list of keymaps
void addKeyboardsOld(KeyboardChooser *mPanelMenu)
{
	char *countries[49] = {
	"us",   "en_US",  "us_intl",  "am", "az", "by",  "be",  "br",
	"bg",  "ca",    "cz",  "cz_qwerty",  "dk",   "dvorak",  "ee",
	"fi",  "fr",  "fr_CH",  "de",  "de_CH",  "el",  "hr",  "hu",
	"is",  "il",  "it",   "jp",   "lt",   "lt_std",  "lt_p",  "lv",
	"mk",  "no",  "pl",   "pt",   "ro",   "ru",   "sr",   "si",
	"sk",  "sk_qwerty",  "es",  "se",  "th",  "ua",  "gb",  "vn",
	"nec/jp",  "tr"
	};

	mPanelMenu->begin();
	Fl_Item *mKbdItem = new Fl_Item("English (US)");
	mKbdItem->field_name("us");
	mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
	mKbdItem->image(keyboard_pix);
	new Fl_Divider(10, 5);
	Fl_Item_Group *more = new Fl_Item_Group(_("More..."));
	mPanelMenu->end();

	more->begin();
	for (int i=0; i<49; i++)
	{
		Fl_Item *mKbdItem = new Fl_Item(countries[i]);
		mKbdItem->field_name(countries[i]);
		mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
		mKbdItem->image(keyboard_pix);
	}
	more->end();
}

void addKeyboards(KeyboardChooser *mPanelMenu)
{
	const char* X11DirList[2] = {"/usr/X11R6/lib/X11/", "/usr/local/X11R6/lib/X11/"};
	const char* rulesFileList[2] = {"xkb/rules/xorg.lst", "xkb/rules/xfree86.lst"};
	Fl_String xdir, xfilename;
	FILE *fp;
	char kbdnames[300][15];
	char kbddescriptions[300][50];
	Fl_Config pGlobalConfig(fl_find_config_file("ede.conf", true));
	
	// First look for directory
	for(int ii=0; ii<2; ii++)
		if( fl_is_dir(X11DirList[ii]) ) {
			xdir = X11DirList[ii];
			goto step2;
		}
	addKeyboardsOld(mPanelMenu); return;

    // Look for filename
step2:
	for(int ii=0; ii<2; ii++) {
		xfilename = xdir + rulesFileList[ii];
		if( fl_file_exists(xfilename) )
			goto step3;
	}
	addKeyboardsOld(mPanelMenu); return;
    
    // now load layouts into widget...
step3:
	fp = fopen(xfilename, "r");
	if(!fp) {
		addKeyboardsOld(mPanelMenu); return;
	}

    
    // warning: ugly code ahead (parser)
	char line[256]; line[0]='\0';
	while ((!feof(fp)) && (!strstr(line,"! layout"))) { 
		fgets(line,255,fp);
	}
	int kbdno = 0;
	while ((!feof(fp) && (strcmp(line,"\n")))) { 
		fgets(line,255,fp);
		int i=0, j=0;
		char name[10];
		char description[200];
		while((line[i] != 13) && (line[i] != 10)) {
			while ((line[i] == 32) || (line[i] == 9))
				i++;
			while ((line[i] != 32) && (line[i] != 9))
				name[j++] = line[i++];
			name[j] = 0; j=0;
			while ((line[i] == 32) || (line[i] == 9))
				i++;
			while ((line[i] != 13) && (line[i] != 10))
				description[j++] = line[i++];
			description[j] = 0;
		}
		strcpy (kbdnames[kbdno],name);
		strcpy (kbddescriptions[kbdno++],description);
	}
	fclose(fp);

    
	// now populate the menu
	// main menu with "More..."
	mPanelMenu->begin();
	Fl_String recentlist;
	pGlobalConfig.get("Keyboard", "RecentKeyboards", recentlist, "");
	Fl_String_List recentkbd(recentlist,"|");
	for (int i = 0; i < kbdno; i++) {
		if (recentkbd.index_of(kbdnames[i]) > -1) {
			Fl_Item *mKbdItem = new Fl_Item(kbddescriptions[i]);
			mKbdItem->field_name(kbdnames[i]);
			mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
			mKbdItem->image(keyboard_pix);
		}
	}
	new Fl_Divider(10, 5);
	Fl_Item_Group *more = new Fl_Item_Group(_("More..."));
	mPanelMenu->end();

        more->begin();
	for (int i=0;i<kbdno;i++) {
		Fl_Item *mKbdItem = new Fl_Item(kbddescriptions[i]);
		mKbdItem->field_name(kbdnames[i]);
		mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
		mKbdItem->image(keyboard_pix);
	}
	more->end();
        
      
/*    for (int i=0; i<rules->layouts.num_desc; i++)
    {
        mPanelMenu->begin();
        Fl_Item *mKbdItem = new Fl_Item(rules->layouts.desc[i].name);
        mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
        mKbdItem->image(keyboard_pix);
        mPanelMenu->end();
    }*/
	return;
}


void getKeyboard(KeyboardChooser *mButton)
{
	Fl_String pKbdLayout;
	Fl_Config pGlobalConfig(fl_find_config_file("ede.conf", true));
	pGlobalConfig.get("Keyboard", "Layout", pKbdLayout, "us");
	setKeyboard(pKbdLayout);
}



// ----------------------------
// KeyboardChooser class
// ----------------------------

KeyboardChooser::KeyboardChooser(int x, int y, int w, int h, Fl_Boxtype up_c, Fl_Boxtype down_c, const char *label)
    : Fl_Menu_Button(x, y, w, h, label)
{
	kbcapplet = this;
	
	m_open = false;
	Height = 0;
	up = up_c;
	down = down_c;
	
	anim_speed(2);
	anim_flags(BOTTOM_TO_TOP);

	addKeyboards(this);
	getKeyboard(this);

}

void KeyboardChooser::draw()
{
	Fl_Boxtype box = up;
	Fl_Flags flags;
	Fl_Color color;
	
	if (!active_r()) {
		flags = FL_INACTIVE;
		color = this->color();
	} else if (belowmouse() || m_open) {
		flags = FL_HIGHLIGHT;
		color = highlight_color();
		if (!color) color = this->color();
		box = down;
	} else {
		flags = 0;
		color = this->color();
	}
	
	if(!box->fills_rectangle()) {
		fl_push_clip(0, 0, this->w(), this->h());
		parent()->draw_group_box();
		fl_pop_clip();
	}
	
	box->draw(0, 0, this->w(), this->h(), color, flags);
	
	int x,y,w,h;
	x = y = 0;
	w = this->w(); h = this->h();
	box->inset(x,y,w,h);
	draw_inside_label(x,y,w,h,flags);
}

void KeyboardChooser::calculate_height()
{
    Fl_Style *s = Fl_Style::find("Menu");
    Height = s->box->dh();
    for(int n=0; n<children(); n++)
    {
        Fl_Widget *i = child(n);
        if(!i) break;
        if(!i->visible()) continue;
        fl_font(i->label_font(), i->label_size());
        Height += i->height()+s->leading;
    }
}

int KeyboardChooser::popup()
{
    m_open = true;
    calculate_height();
    int newy=0-Height;
    Fl_Widget* panel = parent()->parent()->parent(); // ugh
    if (panel->y()+newy<1) newy=parent()->h();
    int retval = Fl_Menu_::popup(0, newy);//, w(), h());
    m_open = false;
    return retval;
}
