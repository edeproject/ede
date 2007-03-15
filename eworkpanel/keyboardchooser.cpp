// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "keyboardchooser.h"

#define MAX_KEYBOARDS 5

KeyboardChooser *kbcapplet;

static fltk::Image keyboard_pix((const char **)keyboard_xpm);

void setKeyboard(const char* pKbdLayout)
{
	if(!pKbdLayout.empty()) {
		char* proc = (char*)malloc(strlen("setxkbmap ")+strlen(pKbdLayout));
		proc = strcpy(proc, "setxkbmap ");
		pKbdLayout = strcat(proc,pKbdLayout);
		fltk::start_child_process(proc, false);
		
		char pShortKbd[2];
		strncpy(pShortKbd, pKbdLayout, 2);
		kbcapplet->tooltip(pKbdLayout);
		kbcapplet->label(pShortKbd);
		kbcapplet->redraw();
	}
}

// count occurences of character within string
int strcount(const char *string, int character)
{
	char *p = string;
	int counter;
	while (p != 0)
		if (p++ == character) counter++;
	return counter;
}

void CB_setKeyboard(fltk::Item *item, void *pData)
{
	EDE_Config pGlobalConfig(find_config_file("ede.conf", true));
	if (!pGlobalConfig.error() && item) {
		char* kbdname = item->field_name();
	
		pGlobalConfig.set("Keyboard", "Layout", kbdname);
		pGlobalConfig.flush();
		setKeyboard(kbdname);

		// update history
		char* recentlist;
		pGlobalConfig.get("Keyboard", "RecentKeyboards", recentlist, "");
		
		if (strstr(recentlist, kbdname) != NULL) return;
		if (strcount(recentlist, '|') > MAX_KEYBOARDS) 
			recentlist = strchr(recentlist, '|');
		
		recentlist = realloc(recentlist, strlen(recentlist)+strlen(kbdname)+1);
		strcat(recentlist, "|");
		strcat(recentlist, kbdname);
		
		pGlobalConfig.set("Keyboard", "RecentKeyboards", recentlist);
		pGlobalConfig.flush();
		
		// refresh menu list
		for (int i=0; i<kbcapplet->children(); i++) {
			if (strcmp(kbcapplet->child(i)->field_name(),kbdname) != 0) return;
		}
		fltk::Item *mKbdItem = new fltk::Item(item->label());
		mKbdItem->field_name(kbdname);
		mKbdItem->callback((fltk::Callback*)CB_setKeyboard);
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
	fltk::Item *mKbdItem = new fltk::Item("English (US)");
	mKbdItem->field_name("us");
	mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
	mKbdItem->image(keyboard_pix);
	new fltk::Divider(10, 5);
	fltk::Item_Group *more = new fltk::Item_Group(_("More..."));
	mPanelMenu->end();

	more->begin();
	for (int i=0; i<49; i++)
	{
		fltk::Item *mKbdItem = new fltk::Item(countries[i]);
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
	char* xdir, xfilename;
	FILE *fp;
	char kbdnames[300][15];
	char kbddescriptions[300][50];
	EDE_Config pGlobalConfig(fl_find_config_file("ede.conf", true));
	
	// First look for directory
	for(int ii=0; ii<2; ii++)
		if( fltk::is_dir(X11DirList[ii]) ) {
			xdir = strdup(X11DirList[ii]);
			goto step2;
		}
	addKeyboardsOld(mPanelMenu); return;

    // Look for filename
step2:
	for(int ii=0; ii<2; ii++) {
		xfilename = (char*) malloc(strlen(xdir)+strlen(rulesFileList[ii]));
		strcpy(xfilename,xdir)
		strcat(xfilename,rulesFileList[ii]);
		if( fltk::file_exists(xfilename) )
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
	char* recentlist;
	pGlobalConfig.get("Keyboard", "RecentKeyboards", recentlist, "");
	for (int i = 0; i < kbdno; i++) {
		if (strchr(recentlist,kbdnames[i]) != NULL) {
			fltk::Item *mKbdItem = new fltk::Item(kbddescriptions[i]);
			mKbdItem->field_name(kbdnames[i]);
			mKbdItem->callback((Fl_Callback*)CB_setKeyboard);
			mKbdItem->image(keyboard_pix);
		}
	}
	new fltk::Divider(10, 5);
	fltk::Item_Group *more = new fltk::Item_Group(_("More..."));
	mPanelMenu->end();

        more->begin();
	for (int i=0;i<kbdno;i++) {
		fltk::Item *mKbdItem = new fltk::Item(kbddescriptions[i]);
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
	char* pKbdLayout;
	EDE_Config pGlobalConfig(find_config_file("ede.conf", true));
	pGlobalConfig.get("Keyboard", "Layout", pKbdLayout, "us");
	setKeyboard(pKbdLayout);
}



// ----------------------------
// KeyboardChooser class
// ----------------------------

KeyboardChooser::KeyboardChooser(int x, int y, int w, int h, fltk::Boxtype up_c, fltk::Boxtype down_c, const char *label)
    : fltk::Menu_Button(x, y, w, h, label)
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
	fltk::Boxtype box = up;
	fltk::Flags flags;
	fltk::Color color;
	
	if (!active_r()) {
		flags = fltk::INACTIVE;
		color = this->color();
	} else if (belowmouse()) {
		flags = fltk::HIGHLIGHT;
		color = highlight_color();
		if (!color) color = this->color();
		box = down;
	} else {
		flags = 0;
		color = this->color();
	}
	
	if(!box->fills_rectangle()) {
		fltk::push_clip(0, 0, this->w(), this->h());
		parent()->draw_group_box();
		fltk::pop_clip();
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
    fltk::Style *s = fltk::Style::find("Menu");
    Height = s->box->dh();
    for(int n=0; n<children(); n++)
    {
        fltk::Widget *i = child(n);
        if(!i) break;
        if(!i->visible()) continue;
        fltk::font(i->label_font(), i->label_size());
        Height += i->height()+s->leading;
    }
}

int KeyboardChooser::popup()
{
    m_open = true;
    calculate_height();
    int retval = fltk::Menu_::popup(0, 0-Height);//, w(), h());
    m_open = false;
    return retval;
}
