/*
 * $Id$
 *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <fltk/run.h>
#include <fltk/filename.h>
#include <fltk/Window.h>
#include <fltk/ScrollGroup.h>
#include <fltk/Button.h>
#include <fltk/ask.h>
#include <fltk/events.h>
#include <fltk/MenuBar.h>
#include <fltk/ItemGroup.h>
#include <fltk/Item.h>
#include <fltk/Divider.h>
#include <fltk/file_chooser.h>
//#include <fltk/FileBrowser.h>
#include <fltk/TiledGroup.h>

#include "../edelib2/about_dialog.h"
#include "../edelib2/Icon.h"
#include "../edelib2/MimeType.h"
#include "../edelib2/NLS.h"
#include "../edelib2/Run.h"
#include "../edelib2/Util.h"

#include "EDE_FileBrowser.h"
#include "EDE_DirTree.h"


#define DEFAULT_ICON "misc-vedran"


using namespace fltk;
using namespace edelib;



Window *win;
TiledGroup* tile;
ScrollGroup* sgroup;
FileBrowser* fbrowser;
DirTree* dirtree;
char current_dir[PATH_MAX];
static bool semaphore=false;
bool showhidden = false;
bool showtree = false;

char **cut_copy_buffer = 0;
bool operation_is_copy = false;



// This is for the icon view, which should be redesigned to be like FileBrowser

void loaddir(const char* path);

void button_press(Widget* w, void*) {
	if (event_clicks() || event_key() == ReturnKey) {
//		run_program((char*)w->user_data(),true,false,true); // use elauncher
//		char tmp[256];
//		snprintf(tmp,255,"konsole --noclose -e %s",(char*)w->user_data());
//		run_program(tmp,true,false,false); 
		if (!w->user_data() || (strlen((char*)w->user_data())==0)) 
			alert(_("Unknown file type"));
		else if (strncmp((char*)w->user_data(),"efiler ",7)==0) {
			// don't launch new efiler instance
			char tmp[PATH_MAX];
			strncpy(tmp,(char*)w->user_data()+7,PATH_MAX);

			// remove quotes
			if (tmp[0] == '\'' || tmp[0] == '"')
				memmove(tmp,tmp+1,strlen(tmp)-1);
			int i=strlen(tmp)-2;
			if (tmp[i] == '\'' || tmp[i] == '"')
				tmp[i] = '\0';

			loaddir(tmp);
		} else {
			fprintf(stderr, "Running: %s\n", (char*)w->user_data());
			run_program((char*)w->user_data(),false,false,true);
		}
	}
	if (event_is_click()) w->take_focus();
	fprintf (stderr, "Event: %s (%d)\n",event_name(event()), event());
}


void loaddir(const char *path) {
	// If user clicks too fast, it can cause problems
	if (semaphore) { 
		return;
	}
	semaphore=true;

	// Set current_dir
	if (filename_isdir(path)) {
		if (path[0] == '~') // Expand tilde
			snprintf(current_dir,PATH_MAX,"%s/%s",getenv("HOME"),path+1);
		else 
			strcpy(current_dir,path);
	} else
		strcpy(current_dir,getenv("HOME"));
	// Trailing slash should always be there
	if (current_dir[strlen(current_dir)-1] != '/') strcat(current_dir,"/");
fprintf (stderr, "loaddir(%s) = (%s)\n",path,current_dir);

	// Update directory tree
	dirtree->set_current(current_dir);

	// set window label
	win->label(tasprintf(_("%s - File manager"), current_dir));

	// Update file browser...
	if (fbrowser->visible()) { fbrowser->load(current_dir); semaphore=false; return; }

	// some constants - TODO: move to configuration
	int startx=0, starty=0;
	int sizex=90, sizey=58;
	int spacex=5, spacey=5;

	// variables used later
	Button **icon_array;
	int icon_num=0;
	dirent **files;


	sgroup->remove_all();

	sgroup->begin();
	// list files
	icon_num = fltk::filename_list(current_dir, &files, alphasort); // no sort needed because icons have coordinates
	icon_array = (Button**) malloc (sizeof(Button*) * icon_num + 1);
	// fill array with zeros, for easier detection if button exists
	for (int i=0; i<icon_num; i++) icon_array[i]=0;

	int myx=startx, myy=starty;
	for (int i=0; i<icon_num; i++) {
		char *n = files[i]->d_name; //shortcut

		// don't show ./ (current directory)
		if (strcmp(n,"./")==0) continue;

		// hide files with dot except ../ (up directory)
		if (!showhidden && (n[0] == '.') && (strcmp(n,"../")!=0)) continue;

		// hide files ending with tilde (backup) - NOTE
		if (!showhidden && (n[strlen(n)-1] == '~')) continue;

		Button* o = new Button(myx, myy, sizex, sizey);
		o->box(NO_BOX);
		//o->labeltype(SHADOW_LABEL);
		o->labelcolor(BLACK);
		o->callback((Callback*)button_press);
		o->align(ALIGN_INSIDE|ALIGN_CENTER|ALIGN_WRAP);
		//o->when(WHEN_CHANGED|WHEN_ENTER_KEY);

		o->label(n);
		o->image(Icon::get(DEFAULT_ICON,Icon::SMALL));

		myx=myx+sizex+spacex;
		// 4 - edges
		if (myx+sizex > sgroup->w()) { myx=startx; myy=myy+sizey+spacey; }

		icon_array[i] = o;
	}
	sgroup->end();
	// Give first icon the focus
	sgroup->child(0)->take_focus();

	sgroup->redraw();

	// Detect icon mimetypes etc.
	for (int i=0; i<icon_num; i++) {
		// ignored files
		if (!icon_array[i]) continue;
		fltk::check(); // update interface

		// get mime data
		char fullpath[PATH_MAX];
		snprintf (fullpath,PATH_MAX-1,"%s%s",current_dir,files[i]->d_name);
		MimeType *m = new MimeType(fullpath);

fprintf(stderr,"Adding: %s (%s), cmd: '%s'\n", fullpath, m->id(), m->command());

		// tooltip
		char *tooltip;
		if (strcmp(m->id(),"directory")==0) 
			asprintf(&tooltip, "%s - %s", files[i]->d_name, m->type_string());
		else 
			asprintf(&tooltip, "%s (%s) - %s", files[i]->d_name, nice_size(filename_size(fullpath)), m->type_string());
		icon_array[i]->tooltip(tooltip);

		// icon
		icon_array[i]->image(m->icon(Icon::SMALL));

		// get command to execute
		if (strcmp(files[i]->d_name,"../")==0) {
			// up directory - we don't want ../ poluting filename
			char exec[PATH_MAX];
			int slashes=0, j;
			for (j=strlen(current_dir); j>0; j--) {
				if (current_dir[j]=='/') slashes++;
				if (slashes==2) break;
			}
			if (slashes<2)
				sprintf(exec,"efiler /");
			else {
				sprintf(exec,"efiler ");
				strncat(exec,current_dir,j);
			}
			icon_array[i]->user_data(strdup(exec));

		} else if (m->command() && (strlen(m->command())>0))
			icon_array[i]->user_data(strdup(m->command()));
		else
			icon_array[i]->user_data(0);

		// make sure label isn't too large
		// TODO: move this to draw() method
		int lx,ly; 
		icon_array[i]->measure_label(lx,ly);
		int pos=strlen(icon_array[i]->label());
		if (pos>252) pos=252;
		char fixlabel[256];
		while (lx>sizex) {
			strncpy(fixlabel,icon_array[i]->label(),pos);
			fixlabel[pos]='\0';
			strcat(fixlabel,"...");
			icon_array[i]->label(strdup(fixlabel));
			icon_array[i]->measure_label(lx,ly);
			pos--;
		}
		icon_array[i]->redraw();
		delete m;
	}

	sgroup->redraw();
	semaphore=false;
}


// Directory tree callback
void dirtree_cb(Widget* w, void*) {
	if (!event_clicks() && event_key() != ReturnKey) return;
	char *d = (char*) dirtree->item()->user_data();
	if (d && strlen(d)>0) loaddir(d);
}


// List view
// Currently using fltk::FileBrowser which is quite ugly (so only callback is here)

void fbrowser_cb(Widget* w, void*) {
	// Take only proper callbacks
	if (!event_clicks() && event_key() != ReturnKey) return;

	// Construct filename
	const char *c = fbrowser->system_path(fbrowser->value());
	char filename[PATH_MAX];
	if (strncmp(c+strlen(c)-3,"../",3)==0) {
		strcpy(filename,current_dir); // both are [PATH_MAX]
		filename[strlen(filename)-1] = '\0'; // remove trailing slash in a directory
		char *c2 = strrchr(filename,'/'); // find previous slash
		if (c2) *(c2+1) = '\0'; // cut everything after this slash
		else strcpy(filename,"/"); // if nothing is found, filename becomes "/"
	} else {
		strncpy(filename,c,PATH_MAX);
	}

	// Change directory
	if (filename_isdir(filename)) 
		loaddir(filename);

	// Let elauncher handle this file...
	else 
		run_program(tsprintf("file:%s",filename),false,false,true);
}


// Menu callbacks

// File menu
void open_cb(Widget*, void*) {
	Widget* w;
	if (sgroup->visible())
		w = sgroup->child(sgroup->focus_index());
	else
		w = fbrowser;
	event_clicks(2);
	w->do_callback();
}
void location_cb(Widget*, void*) {
	const char *dir = dir_chooser(_("Choose location"),current_dir);
	if (dir) loaddir(dir);
}
void quit_cb(Widget*, void*) {exit(0);}

// Edit menu

// Execute cut or copy operation when List View is active
void do_cut_copy_fbrowser(bool m_copy) {
	// Count selected icons, for malloc
	int num = fbrowser->children();
	int nselected = 0;
	for (int i=0; i<num; i++)
		if (fbrowser->selected(i)) nselected++;

	// Clear cut/copy buffer and optionally ungray the previously cutted icons
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=0; i<num; i++)
				fbrowser->child(i)->textcolor(BLACK); // FIXME: use color from style
		}
	}

	// Allocate buffer
	cut_copy_buffer = (char**)malloc(sizeof(char*) * (nselected+2));

	// Add selected files to buffer and optionally grey icons (for cut effect)
	int buf=0;
	for (int i=0; i<=num; i++) {
		if (fbrowser->selected(i)) {
			cut_copy_buffer[buf] = strdup(fbrowser->system_path(i));
			if (!m_copy) fbrowser->child(i)->textcolor(GRAY50);
			buf++;
		}
	}
	cut_copy_buffer[buf] = 0;
	operation_is_copy = m_copy;

	// Deselect all
	fbrowser->deselect();
}

// Execute cut or copy operation when Icon View is active
void do_cut_copy_sgroup(bool m_copy) {
	// Group doesn't support type(MULTI) so only one item can be selected
	// Will be changed

	int num = fbrowser->children();

	// Clear cut/copy buffer and optionally ungray the previously cutted icon
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=0; i<num; i++)
				sgroup->child(i)->textcolor(BLACK); // FIXME: use color from style
		}
	}

	// Allocate buffer
	cut_copy_buffer = (char**)malloc(sizeof(char*) * 3);

	// Add selected files to buffer and optionally grey icons (for cut effect)
	// FIXME: label doesn't contain filename!!
	asprintf(&cut_copy_buffer[0], "%s%s", current_dir, sgroup->child(sgroup->focus_index())->label());
	if (!m_copy) sgroup->child(sgroup->focus_index())->textcolor(GRAY50);
	cut_copy_buffer[1]=0;
	operation_is_copy=m_copy;
}

void cut_cb(Widget*, void*) {
	if (sgroup->visible()) 
		do_cut_copy_sgroup(false);
	else
		do_cut_copy_fbrowser(false);
}

void copy_cb(Widget*, void*) {
	if (sgroup->visible()) 
		do_cut_copy_sgroup(true);
	else
		do_cut_copy_fbrowser(true);
}

void paste_cb(Widget*, void*) {
	
}

// View menu
void iconsview_cb(Widget*,void*) {
	if (sgroup->visible() && event_key() == F8Key) {
		sgroup->hide(); 
		fbrowser->show(); 
		fbrowser->take_focus();
	} else {
		sgroup->show(); 
		fbrowser->hide(); 
		sgroup->take_focus();
	}
	// We update the inactive view only when it's shown i.e. now
	loaddir(current_dir);
}
void listview_cb(Widget*,void*) { sgroup->hide(); fbrowser->show(); }
void showhidden_cb(Widget* w, void*) {
	Item *i = (Item*)w;
	if (showhidden) {
		showhidden=false;
		i->clear();
	} else {
		showhidden=true;
		i->set();
	}
	fbrowser->show_hidden(showhidden);
	//fbrowser->redraw();
	loaddir(current_dir);
}
void showtree_cb(Widget*,void*) {
	if (!showtree) {
		tile->position(1,0,150,0);
		showtree=true;
	} else {
		tile->position(150,0,0,0);
		showtree=false;
	}
}
void refresh_cb(Widget*,void*) {
	loaddir(current_dir);
}
void case_cb(Widget*,void*) {
	fprintf(stderr,"Not implemented yet...\n");
}

// Help menu
void about_cb(Widget*, void*) { about_dialog("EFiler", "0.1", _("EDE File Manager"));}




// GUI design

int main (int argc, char **argv) {
	win = new fltk::Window(600, 400);
	win->color(WHITE);
	win->begin();

	// Main menu
	{MenuBar *m = new MenuBar(0, 0, 600, 25);
		m->begin();
		{ItemGroup *o = new ItemGroup(_("&File"));
			o->begin();
			{Item *o = new Item(_("&Open"));
				o->callback(open_cb);
				o->shortcut(CTRL+'o');
			}
			{Item *o = new Item(_("Open &with..."));
				//o->callback(open_cb);
				//o->shortcut(CTRL+'o');
			}
			new Divider();
			{Item *o = new Item(_("Open &location"));
				o->callback(location_cb);
				//o->shortcut(CTRL+'o');
			}
			new Divider();
			{Item *o = new Item(_("&Quit"));
				o->callback(quit_cb);
				o->shortcut(CTRL+'q');
			}
			o->end();
		}
		{ItemGroup *o = new ItemGroup(_("&Edit"));
			o->begin();
			{Item *o = new Item(_("&Cut"));
				o->callback(cut_cb);
				o->shortcut(CTRL+'x');
			}
			{Item *o = new Item(_("C&opy"));
				//o->callback(open_cb);
				o->shortcut(CTRL+'c');
			}
			{Item *o = new Item(_("&Paste"));
				//o->callback(open_cb);
				o->shortcut(CTRL+'v');
			}
			{Item *o = new Item(_("&Rename"));
				//o->callback(open_cb);
				o->shortcut(F2Key);
			}
			{Item *o = new Item(_("&Delete"));
				//o->callback(open_cb);
				o->shortcut(DeleteKey);
			}
			new Divider();
			{Item *o = new Item(_("&Preferences..."));
				o->shortcut(CTRL+'p');
			}
			o->end();
		}
		{ItemGroup *o = new ItemGroup(_("&View"));
			o->begin();
			{Item *o = new Item(_("&Icons"));
				o->type(Item::RADIO);
				o->shortcut(F8Key);
				o->set();
				o->callback(iconsview_cb);
			}
			{Item *o = new Item(_("&Detailed List"));
				o->type(Item::RADIO);
				o->callback(listview_cb);
			}
			new Divider();
			{Item *o = new Item(_("&Show hidden"));
				o->type(Item::TOGGLE);
				o->callback(showhidden_cb);
			}
			{Item *o = new Item(_("Directory &Tree"));
				o->type(Item::TOGGLE);
				o->shortcut(F9Key);
				o->callback(showtree_cb);
			}
			new Divider();
			{Item *o = new Item(_("&Refresh"));
				//o->type();
				o->callback(refresh_cb);
				o->shortcut(F5Key);
			}
			{ItemGroup *o = new ItemGroup(_("S&ort"));
				o->begin();
				{Item *o = new Item(_("&Case sensitive"));
					//o->type();
					o->callback(case_cb);
				}
				o->end();
			}
			o->end();
		}
		{ItemGroup *o = new ItemGroup(_("&Help"));
			o->begin();
			{Item *o = new Item(_("&About File Manager"));
				o->shortcut();
				o->callback(about_cb);
			}
			o->end();
		}
		m->end();
	}

	// Main tiled group
	{tile = new TiledGroup(0, 25, 600, 375);
		tile->color(WHITE);
		tile->begin();

		// Directory tree
		{dirtree = new DirTree(0, 0, 150, 375);
			dirtree->box(DOWN_BOX);
			dirtree->color(WHITE);
			dirtree->load();
			dirtree->callback(dirtree_cb);
			dirtree->when(WHEN_CHANGED|WHEN_ENTER_KEY);
		}

		// Icon view
		{sgroup = new ScrollGroup(150, 0, 450, 375);
			sgroup->box(DOWN_BOX);
			sgroup->color(WHITE);
	//		sgroup->highlight_color(WHITE);
	//		sgroup->selection_color(WHITE);
			sgroup->align(ALIGN_LEFT|ALIGN_TOP);
	//		g->label("There are no files in current directory");
		}
	
		// List view
		{fbrowser = new FileBrowser(150, 0, 450, 375);
			fbrowser->box(DOWN_BOX);
			fbrowser->color(WHITE);
			fbrowser->callback(fbrowser_cb);
	//		sgroup->align(ALIGN_LEFT|ALIGN_TOP);
	//		g->label("There are no files in current directory");
			fbrowser->when(WHEN_ENTER_KEY);
			//fbrowser->labelsize(12);
			fbrowser->type(Browser::MULTI);
			fbrowser->hide();
		}
		tile->end();
	}

	// We hide dirtree and browser expands to fill space
	tile->position(150,0,1,0);

	win->end();
	win->resizable(tile);
	win->icon(Icon::get("folder",Icon::TINY));
	win->show();

	if (argc==1) { // No params
		loaddir ("");
	} else {
		loaddir (argv[1]);
	}

	return fltk::run();
}
