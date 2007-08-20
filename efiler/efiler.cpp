/*
 * $Id$
 *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h> // timer in cb_open
#include <sys/resource.h> // for core dumps

// for application icon:
#include <X11/xpm.h>
#include <FL/x.H> 
#include "efiler.xpm"


#include <Fl/Fl.H>
#include <Fl/Fl_Double_Window.H>
#include <Fl/Fl_Menu_Bar.H>
#include <Fl/Fl_File_Chooser.H> // for fl_dir_chooser, used in "Open location"
#include <Fl/filename.H>
#include <Fl/Fl_File_Input.H> // location bar

#include <edelib/Nls.h>
#include <edelib/MimeType.h>
#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/Run.h>

#include "EDE_FileView.h" // our file view widget
#include "EDE_DirTree.h" // directory tree
#include "Util.h" // ex-edelib
#include "ede_ask.h" // replacement for fl_ask
//#include "Ask.h"

#include "fileops.h" // file operations
#include "filesystem.h" // filesystem support


Fl_Window* win;
FileDetailsView* view;
Fl_Menu_Bar* main_menu;
Fl_Box* statusbar;
DirTree* dirtree;
Fl_Tile* tile;
Fl_Group* location_bar;
Fl_File_Input* location_input;
Fl_Menu_Button* context_menu;

char current_dir[FL_PATH_MAX];
bool showhidden;
bool semaphore;
bool dirsfirst;
bool ignorecase;
bool showtree;
bool showlocation;
int tree_width;


// constants

const int default_window_width = 600;
const int default_window_height = 400;
const int menubar_height = 30;
const int location_bar_height = 40;
const int statusbar_height = 24;
const int statusbar_width = 400;

int default_tree_width=150;

/*-----------------------------------------------------------------
	Some improvements to Fl_File_Input, that should be added 
	upstream:
	* enable to set shortcut in label (STR 1770 for Fl_Input_)
	* navigate using Ctrl+arrow (here we jump to dir. sep.)
-------------------------------------------------------------------*/

class My_File_Input : public Fl_File_Input {
	int shortcut_;
public:
	My_File_Input(int X,int Y,int W,int H,const char *label=0) : Fl_File_Input(X,Y,W,H,label) {
		set_flag(SHORTCUT_LABEL);
		shortcut_ = 0;
	}

	int shortcut() const {return shortcut_;}
	void shortcut(int s) {shortcut_ = s;}

	int handle(int e) {
		if (e==FL_SHORTCUT) {
			if (!(shortcut() ? Fl::test_shortcut(shortcut()) : test_shortcut())) return 0;
			if (Fl::visible_focus() && handle(FL_FOCUS)) Fl::focus(this);
			return 1;
		}
		if (e==FL_KEYDOWN) {
			if (Fl::event_state(FL_CTRL)) {
				const char* v = value();
				if (Fl::event_key()==FL_Left) {
					for (uint i=position()-2; i>=0; i--)
						if (v[i]=='/') {
							if (Fl::event_state(FL_SHIFT))
								position(i+1,mark()); 
							else
								position(i+1,i+1); 
							break; 
						}
					return 1; // don't go out
				}
				if (Fl::event_key()==FL_Right) {
					for (uint i=position()+1; i<strlen(v); i++)
						if (v[i]=='/') { 
							if (Fl::event_state(FL_SHIFT))
								position(i+1,mark()); 
							else
								position(i+1,i+1); 
							break; 
						}
					return 1; // don't go out
				}
			}
		}
		return Fl_File_Input::handle(e);
	}
};




/*-----------------------------------------------------------------
	"Simple opener" - keep a list of openers in text file
	This is just a temporary fix so that efiler works atm.
-------------------------------------------------------------------*/

// These variables are global to save time on construction/destruction
edelib::MimeType mime_resolver;
struct stat stat_buffer;

struct sopeners {
	char* type;
	char* opener;
	sopeners* next;
} *openers=0;

char *simpleopener(const char* mimetype) {
	sopeners* p;
	if (!openers) {
		FILE* fp = fopen("openers.txt","r");
		if (!fp) { ede_alert(_("File openers.txt not found")); return 0; }
		char buf[FL_PATH_MAX*2];
		while (!feof(fp)) {
			fgets((char*)&buf, FL_PATH_MAX*2, fp);
			if (buf[0]=='\0' || buf[1]=='\0' || buf[0]=='#') continue;
			buf[strlen(buf)-1]='\0';
			char *tmp = strstr(buf, "||");
			if (!tmp) continue; // malformatted opener
			*tmp = '\0';
			sopeners* q = new sopeners; 
			q->type=strdup(buf);
			q->opener=strdup(tmp+2);
//fprintf (stderr, "Found type: '%s' opener: '%s'\n",q->type,q->opener);
			q->next=0;
			if (!openers) openers=q;
			else p->next = q;
			p=q;
		}
		fclose(fp);
	}
	p=openers;
	char *result=0; int topscore=0;
	while (p != 0) {
		int score=strlen(p->type);
		if (strncmp(p->type,mimetype,score)==0 && score>topscore) {
			topscore=score;
			result=p->opener;
		}
		p=p->next;
	}
	return result;
}





/*-----------------------------------------------------------------
	LoadDir()
-------------------------------------------------------------------*/

// modification of versionsort which ignores case
int myversionsort(const void *a, const void *b) {
	struct dirent** ka = (struct dirent**)a;
	struct dirent** kb = (struct dirent**)b;
	char* ma = strdup((*ka)->d_name);
	char* mb = strdup((*kb)->d_name);
	edelib::str_tolower((unsigned char*)ma); edelib::str_tolower((unsigned char*)mb);
	int k = strverscmp(ma,mb);
	free(ma); free(mb);
	return k;
}

// Return a string describing permissions relative to the current user
const char* permission_text(mode_t mode, uid_t uid, gid_t gid) {
	static uid_t uuid = getuid();
	static gid_t ugid = getgid();
	// use geteuid() and getegid()? what's the difference?
	if (S_ISDIR(mode)) {
		// Directory
		if (uid == uuid) { 
			if (!(mode&S_IXUSR))
				return _("Can't enter directory");
			if (!(mode&S_IRUSR))
				return _("Can't list directory");
			if (!(mode&S_IWUSR))
				return _("Can't add, delete, rename files"); // shorter description?
			return _("Full permissions");
		} else if (gid == ugid) {
			if (!(mode&S_IXGRP))
				return _("Can't enter directory");
			if (!(mode&S_IRGRP))
				return _("Can't list directory");
			if (!(mode&S_IWGRP))
				return _("Can't add, delete, rename files");
			return _("Full permissions");
		} else {
			if (!(mode&S_IXOTH))
				return _("Can't enter directory");
			if (!(mode&S_IROTH))
				return _("Can't list directory");
			if (!(mode&S_IWOTH))
				return _("Can't add, delete, rename files");
			return _("Full permissions");
		}
	} else {
		// Regular file (anything else should have similar permissions)
		if (uid == uuid) {
			if ((mode&S_IXUSR) && (mode&S_IRUSR) && (mode&S_IWUSR))
				return _("Read, write, execute");
			if ((mode&S_IRUSR) && (mode&S_IWUSR))
				return _("Read, write");
			if ((mode&S_IXUSR) && (mode&S_IRUSR))
				return _("Read only, execute");
			if ((mode&S_IRUSR))
				return _("Read only");
			// ignore weird permissions such as "write only"
			return _("Not readable");
		} else if (gid == ugid) {
			if ((mode&S_IXGRP) && (mode&S_IRGRP) && (mode&S_IWGRP))
				return _("Read, write, execute");
			if ((mode&S_IRGRP) && (mode&S_IWGRP))
				return _("Read, write");
			if ((mode&S_IXGRP) && (mode&S_IRGRP))
				return _("Read only, execute");
			if ((mode&S_IRGRP))
				return _("Read only");
			return _("Not readable");
		} else {
			if ((mode&S_IXOTH) && (mode&S_IROTH) && (mode&S_IWOTH))
				return _("Read, write, execute");
			if ((mode&S_IROTH) && (mode&S_IWOTH))
				return _("Read, write");
			if ((mode&S_IXOTH) && (mode&S_IROTH))
				return _("Read only, execute");
			if ((mode&S_IROTH))
				return _("Read only");
			return _("Not readable");
		}
	}
}

void loaddir(const char *path) {
	if (semaphore) return; // Prevent loaddir to interrupt previous loaddir - that can result in crash
	semaphore=true;


	char old_dir[FL_PATH_MAX];
	strncpy(old_dir,current_dir,strlen(current_dir)); // Restore olddir in case of error

	// Sometimes path is just a pointer to current_dir
	char* tmpath = strdup(path);

	// Set current_dir
	// fl_filename_isdir() thinks that / isn't a dir :(
	if (strcmp(path,"/")==0 || fl_filename_isdir(path)) {
		if (path[0] == '~') {// Expand tilde
			snprintf(current_dir,FL_PATH_MAX,"%s/%s",getenv("HOME"),tmpath+1);
		} else if (path[0] != '/') { // Relative path
			snprintf(current_dir,FL_PATH_MAX,"%s/%s",getenv("PWD"),tmpath);
		} else {
			if (path!=current_dir) strncpy(current_dir,tmpath,strlen(tmpath)+1);
		}
	} else {
		ede_alert(tsprintf(_("Directory not found: %s"),path));
		free(tmpath);
		semaphore=false;
		return;
	}
	
	free(tmpath);

	// Trailing slash should always be there
	if (current_dir[strlen(current_dir)-1] != '/') strcat(current_dir,"/");

	// Compact dotdot (..)
	while (char *tmp = strstr(current_dir,"/../")) {
		char *tmp2 = tmp+4;
		tmp--;
		while (tmp != current_dir && *tmp != '/') tmp--;
		tmp++;
		while (*tmp2 != '\0') *tmp++ = *tmp2++;
		*tmp='\0';
	}

	// List all files in directory
	int size=0;
	dirent **files;
	if (ignorecase) 
		size = scandir(current_dir, &files, 0, myversionsort);
	else
		size = scandir(current_dir, &files, 0, versionsort);

	if (size<1) { // there should always be . and ..
		ede_alert(_("Permission denied!"));
//		edelib::fl_alert(_("Permission denied!"));
		strncpy(current_dir,old_dir,strlen(current_dir));
		semaphore=false;
		return;
	}

	// Ok, now we know everything is fine...

	// Update directory tree
	dirtree->set_current(current_dir);
	location_input->value(current_dir);

	// set window label
	// copy_label() is a method that calls strdup() and later free()
	win->copy_label(tsprintf(_("%s - File manager"), current_dir));
	statusbar->copy_label(tsprintf(_("Scanning directory %s..."), current_dir)); 

	view->clear();

fprintf(stderr, "Size: %d\n", size);
	FileItem **item_list = new FileItem*[size];
	int fsize=0;

	for (int i=0; i<size; i++) {
		char *n = files[i]->d_name; //shortcut
		if (i>0) free(files[i-1]); // see scandir(3)

		// don't show . (current directory)
		if (strcmp(n,".")==0) continue;

		// hide files with dot except .. (up directory)
		if (!showhidden && (n[0] == '.') && (strcmp(n,"..")!=0)) continue;

		// hide files ending with tilde (backup) - NOTE
		if (!showhidden && (n[strlen(n)-1] == '~')) continue;

		char fullpath[FL_PATH_MAX];
		snprintf (fullpath,FL_PATH_MAX-1,"%s%s",current_dir,n);

		if (stat(fullpath,&stat_buffer)) continue; // happens when user has traverse but not read privilege

		FileItem *item = new FileItem;
		item->name = n;
		item->realpath = fullpath;
		item->date = nice_time(stat_buffer.st_mtime);
		item->permissions = permission_text(stat_buffer.st_mode,stat_buffer.st_uid,stat_buffer.st_gid);
		if (strcmp(n,"..")==0) {
			item->icon = "undo";
			item->description = "Go up";
			item->size = "";
		} else if (S_ISDIR(stat_buffer.st_mode)) { // directory
			item->icon = "folder";
			item->description = "Directory";
			// item->name += "/";
			item->size = "";
			item->realpath += "/";
		} else {
			item->icon = "unknown";
			item->description = "Unknown";
			item->size = nice_size(stat_buffer.st_size);
		}

		item_list[fsize++] = item;
	}
	free(files[size-1]); free(files); // see scandir(3)


	// Populate view
	for (int i=0; i<fsize; i++)
		if (item_list[i]->description == "Go up")
			view->add(item_list[i]);
	if (dirsfirst) {
		for (int i=0; i<fsize; i++)
			if (item_list[i]->description == "Directory")
				view->add(item_list[i]);
		for (int i=0; i<fsize; i++) 
			if (item_list[i]->description != "Directory" && item_list[i]->description != "Go up")
				view->add(item_list[i]);
	} else {
		for (int i=0; i<fsize; i++) 
			if (item_list[i]->description != "Go up")
				view->add(item_list[i]);
	}
	view->redraw();
	Fl::check();

	// Update mime types - can be slow...
	for (int i=0; i<fsize; i++) {
		if (item_list[i]->description != "Directory" && item_list[i]->description != "Go up") {
			mime_resolver.set(item_list[i]->realpath.c_str());
			edelib::String desc,icon;
			desc = mime_resolver.comment();
			// First letter of desc should be upper case:
			if (desc.length()>0 && desc[0]>='a' && desc[0]<='z') desc[0] = desc[0]-'a'+'A';
			icon = mime_resolver.icon_name();
			if (desc!="" || icon!="") {
				if (desc != "") item_list[i]->description = desc;
				if (icon != "") item_list[i]->icon = icon;
fprintf (stderr, "ICON: %s !!!!!\n", icon.c_str());
				view->update(item_list[i]);
			}
			Fl::check();
		}
	}

	// Cleanup
	for (int i=0; i<fsize; i++) delete item_list[i];
	delete[] item_list;
	semaphore=false;

	// Get partition size and free space
	double totalsize, freesize;
	if (fs_usage(current_dir,totalsize,freesize)) {
		double percent = (totalsize-freesize)/totalsize*100;
		char *tmp = strdup(nice_size(totalsize)); // nice_size() operates on a static char buffer, we can't use two calls in the same command
		statusbar->copy_label(tsprintf(_("Filesystem %s, Size %s, Free %s (%4.1f%% used)"), find_fs_for(current_dir), tmp, nice_size(freesize), percent));
		free(tmp);
	} else
		statusbar->label(_("Error reading filesystem information!"));
}



/*-----------------------------------------------------------------
	Main menu and other callbacks
-------------------------------------------------------------------*/

// This callback is called by doubleclicking on file list, by main menu and context menu
void open_cb(Fl_Widget*w, void*data) {
fprintf (stderr,"cb\n");

	if (Fl::event_clicks() || Fl::event_key() == FL_Enter || w==main_menu || w==context_menu) {

fprintf (stderr,"enter\n");
//if (Fl::event_clicks()) fprintf(stderr, "clicks\n");
//if (Fl::event_key()==FL_Enter) fprintf(stderr, "ekey\n");
		static timeval tm = {0,0};
		timeval newtm;
		gettimeofday(&newtm,0);
		if (newtm.tv_sec - tm.tv_sec < 1 || (newtm.tv_sec-tm.tv_sec==1 && newtm.tv_usec<tm.tv_usec)) return; // no calling within 1 second
		tm=newtm;
		if (view->get_focus()==0) return; // This can happen while efiler is loading

		char* path = (char*)view->path(view->get_focus());
		fprintf(stderr, "Path: %s (ev %d)\n",path,Fl::event());

		if (stat(path,&stat_buffer)) return; // error
		if (S_ISDIR(stat_buffer.st_mode)) {  // directory
			loaddir(path);
			return;
		}

		// Find opener
		mime_resolver.set(path);
		char* opener = simpleopener(mime_resolver.type().c_str());

	// dump core
	struct rlimit *rlim = (struct rlimit*)malloc(sizeof(struct rlimit));
	getrlimit (RLIMIT_CORE, rlim);
	rlim_t old_rlimit = rlim->rlim_cur; // keep previous rlimit
	rlim->rlim_cur = RLIM_INFINITY;
	setrlimit (RLIMIT_CORE, rlim);

		const char *o2 = tsprintf(opener,path); // opener should contain %s
		fprintf (stderr, "run_program: %s\n", o2);

		if (opener) { 
			int k=edelib::run_program(o2,false); fprintf(stderr, "retval: %d\n", k); 
		} else
			statusbar->copy_label(tsprintf(_("No program to open %s!"), fl_filename_name(path)));

	rlim->rlim_cur = old_rlimit;
	setrlimit (RLIMIT_CORE, rlim);
	free(rlim);

	}
} // open_cb



// Main menu callbacks
void new_cb(Fl_Widget*, void*) {
	// FIXME: use program path 
	edelib::run_program(tsprintf("efiler %s",current_dir),false); 
}

void quit_cb(Fl_Widget*, void*) { win->hide(); }

void cut_cb(Fl_Widget*, void*) { do_cut_copy(false); }
void copy_cb(Fl_Widget*, void*) { do_cut_copy(true); }
void paste_cb(Fl_Widget*, void*) { Fl::paste(*view,1); } // view->handle() will call do_paste()
void delete_cb(Fl_Widget*, void*) { do_delete(); }


void showtree_cb(Fl_Widget*, void*) {
	showtree = !showtree;
	if (!showtree) {
		tree_width = dirtree->w();
		tile->position(default_tree_width, 1, 1, 1); // NOTE this doesn't always work!
fprintf(stderr, "Hide tree: %d\n", tree_width);
	} else {
		int currentw = dirtree->w();
		tile->position(currentw, 1, tree_width, 1);
fprintf(stderr, "Show tree: %d %d\n", currentw, tree_width);
	}
}

void refresh_cb(Fl_Widget*, void*) { 
	loaddir(current_dir); 
}

void locationbar_cb(Fl_Widget*, void*) {
	showlocation = !showlocation;
	if (showlocation) {
		location_bar->show();
		location_bar->resize(0, menubar_height, win->w(), location_bar_height);
		tile->resize(0, menubar_height+location_bar_height, win->w(), win->h()-menubar_height-location_bar_height-statusbar_height);
//		win->resizable(tile); // win keeps old tile size...
		win->redraw();
	} else {
		location_bar->hide();
//		location_bar->resize(0, menubar_height, win->w(), 0);
		tile->resize(0, menubar_height, win->w(), win->h()-menubar_height-statusbar_height);
		win->redraw();
		win->resizable(tile);
		win->redraw();
	}
}

void showhidden_cb(Fl_Widget*, void*) { 
	showhidden=!showhidden; 
	dirtree->show_hidden(showhidden);
	dirtree->reload(); 
	loaddir(current_dir); 
}

void case_cb(Fl_Widget*, void*) { 
	ignorecase=!ignorecase;
	dirtree->ignore_case(ignorecase);
	dirtree->reload(); 
	loaddir(current_dir); 
}

void dirsfirst_cb(Fl_Widget*, void*) { 
	dirsfirst=!dirsfirst; 
	loaddir(current_dir); 
}


// dummy callbacks - TODO
void ow_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); } // make a list of openers
void pref_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void iconsview_cb(Fl_Widget*, void*) { 
//fprintf(stderr, "callback\n");
view->type(FILE_ICON_VIEW);
loaddir(current_dir);
}
void listview_cb(Fl_Widget*, void*) { 
//fprintf(stderr, "callback\n"); 
view->type(FILE_DETAILS_VIEW); 
loaddir(current_dir);
}
void about_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void aboutede_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }



// Directory tree callback
void tree_cb(Fl_Widget*, void*) {
	if (Fl::event_clicks() || Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter) {
		int selected = dirtree->get_focus();
		loaddir((char*)dirtree->data(selected));
	}
}


// This callback handles location input: changing the directory, autocomplete 
// and callbacks for shortcut buttons
// location_input is set to FL_WHEN_CHANGED 
void location_input_cb(Fl_Widget*, void*) {
	if (Fl::event_key() == FL_Enter || Fl::event()==FL_RELEASE)
		// second event is click on button
		loaddir(location_input->value()); 

	if (Fl::event()==FL_KEYDOWN && Fl::event_key()!=FL_BackSpace && Fl::event_key()!=FL_Delete) {
		// Pressing a key in Fl_Input will automatically replace selection with that char
		// So there are really two possibilities:
		//   1. Cursor is at the end, we add autocomplete stuff at cursor pos
		//   2. Cursor is in the middle, we do nothing

		const char* loc = location_input->value(); // shortcut
		if (strlen(loc)<1 || loc[strlen(loc)-1]=='/') return;
		uint pos = location_input->position();
		if (pos!=strlen(loc)) return; // cursor in the middle
		int mark = location_input->mark();

		// To avoid scandir, we will use view contents
		if ((strlen(loc)>strlen(current_dir)) && (!strchr(loc+strlen(current_dir),'/'))) {
			int i;
			for (i=1; i<=view->size(); i++) {
				const char* p = view->path(i);
				if ((p[strlen(p)-1] == '/') && (strncmp(loc, p, strlen(loc))==0))
					break;
			}

			if (i<=view->size()) {
				location_input->replace(pos, mark, view->path(i)+pos);
				location_input->position(pos);
				location_input->mark(strlen(view->path(i)));
			}
			// else beep();  ??

		} else {
			// sigh, we need to scandir....
			char* k;
			if (!(k=strrchr(loc,'/'))) return;
			char* updir = strdup(loc);
			updir[k-loc+1] = '\0';

			dirent **files;
			int size = scandir(updir, &files, 0, versionsort);
			if (size<1) { free(updir); return; }

			int i;
			char p[FL_PATH_MAX];
			for (i=0; i<size; i++) {
				snprintf (p,FL_PATH_MAX-1,"%s%s",updir,files[i]->d_name);

				struct stat buf;
				if (stat(p,&buf)) continue; // happens when user has traverse but not read privilege
				if (S_ISDIR(buf.st_mode)) {
					strcat(p,"/");
					if (strncmp(loc, p, strlen(loc))==0)
						break;
				}
				free(files[i]);
			}
			free(files); free(updir);
			if (i<size) {
				location_input->replace(pos, mark, p+pos);
				location_input->position(pos);
				location_input->mark(strlen(p));
			}
			// else beep();  ??
		}
	}
}



// TODO: move this to view
// every item should have own context menu defined

Fl_Menu_Item context_menu_definition[] = {
	{_("&Open"),		0,	open_cb},
	{_("Open &with..."),	0,	ow_cb,		0,FL_MENU_DIVIDER},
	{_("&Cut"),		0,	cut_cb},
	{_("Co&py"),		0,	copy_cb},
	{_("Pa&ste"),		0,	paste_cb},
	{_("&Delete"),		0,	delete_cb,	0,FL_MENU_DIVIDER},
	{_("P&references..."),	0,	pref_cb},
	{0}
};

// Right click - show context menu
void context_cb(Fl_Widget*, void*) {
	context_menu->popup();
	context_menu->value(-1);
}


/*-----------------------------------------------------------------
	GUI design
-------------------------------------------------------------------*/

// Main window menu - definition
Fl_Menu_Item main_menu_definition[] = {
	{_("&File"),	0, 0, 0, FL_SUBMENU},
		{_("&Open"),		FL_CTRL+'o',	open_cb},
		{_("Open &with..."),	0,		ow_cb,		0,FL_MENU_DIVIDER},
//		{_("Open &location"),	0,		location_cb,	0,FL_MENU_DIVIDER},
		{_("&New window"),	FL_CTRL+'n',	new_cb,		0,FL_MENU_DIVIDER},
		{_("&Quit"),		FL_CTRL+'q',	quit_cb},
		{0},

	{_("&Edit"),	0, 0, 0, FL_SUBMENU},
		{_("&Cut"),		FL_CTRL+'x',	cut_cb},
		{_("C&opy"),		FL_CTRL+'c',	copy_cb},
		{_("&Paste"),		FL_CTRL+'v',	paste_cb},
		{_("&Rename"),		FL_F+2,		0}, // no callback - view handles this
		{_("&Delete"),		FL_Delete,	delete_cb,	0,	FL_MENU_DIVIDER},
		{_("Pre&ferences"),	FL_CTRL+'p',	pref_cb},
		{0},

	{_("&View"),	0, 0, 0, FL_SUBMENU},
		{_("&Icons"),		FL_F+8,		iconsview_cb,	0,	FL_MENU_RADIO}, // coming soon
		{_("&Detailed list"),	0,		listview_cb,	0,	FL_MENU_RADIO|FL_MENU_VALUE|FL_MENU_DIVIDER}, 
		{_("Directory &tree"),	FL_F+9,		showtree_cb,	0,	FL_MENU_TOGGLE|FL_MENU_VALUE},
		{_("&Location bar"),	FL_F+10,	locationbar_cb,	0,	FL_MENU_TOGGLE|FL_MENU_VALUE},
		{_("&Hidden files"),	0,		showhidden_cb,	0,	FL_MENU_TOGGLE|FL_MENU_DIVIDER},
		{_("&Refresh"),		FL_F+5,		refresh_cb},
		{_("&Sort"),	0, 0, 0, FL_SUBMENU},
			{_("&Case sensitive"),	0,	case_cb,	0,	FL_MENU_TOGGLE},
			{_("D&irectories first"), 0,	dirsfirst_cb,	0,	FL_MENU_TOGGLE|FL_MENU_VALUE},
			{0},
		{0},

	{_("&Help"),	0, 0, 0, FL_SUBMENU},
		{_("&About File Manager"), 0,		about_cb}, // coming soon
		{_("&About EDE"), 	0,		aboutede_cb}, // coming soon
		{0},
	{0}
};



// Main program
extern int FL_NORMAL_SIZE;
int main (int argc, char **argv) {

	// Parse command line - this must come first
	int unknown=0;
	Fl::args(argc,argv,unknown);
	if (unknown==argc)
		snprintf(current_dir, FL_PATH_MAX, getenv("HOME"));
	else {
		if (strcmp(argv[unknown],"--help")==0) {
			printf(_("EFiler - EDE File Manager\nPart of Equinox Desktop Environment (EDE).\nCopyright (c) 2000-2007 EDE Authors.\n\nThis program is licenced under terms of the\nGNU General Public Licence version 2 or newer.\nSee COPYING for details.\n\n"));
			printf(_("Usage:\n\tefiler [OPTIONS] [PATH]\n\n"));
			printf("%s\n",Fl::help);
			return 1;
		}
		strncpy(current_dir, argv[unknown], strlen(argv[unknown])+1);
	}


fl_register_images();
edelib::IconTheme::init("crystalsvg");

FL_NORMAL_SIZE=12;
fl_message_font(FL_HELVETICA, 12);


	// Main GUI design

	win = new Fl_Double_Window(default_window_width, default_window_height);
//	win->color(FL_WHITE);
	win->begin();
		main_menu = new Fl_Menu_Bar(0,0,default_window_width,menubar_height);
		main_menu->menu(main_menu_definition);

		location_bar = new Fl_Group(0, menubar_height, default_window_width, location_bar_height);
		location_bar->begin();
			location_input = new My_File_Input(70, menubar_height+2, default_window_width-200, location_bar_height-5, _("L&ocation:"));
			location_input->align(FL_ALIGN_LEFT);
			location_input->callback(location_input_cb);
			location_input->when(FL_WHEN_ENTER_KEY_CHANGED);
		location_bar->end();
		location_bar->box(FL_UP_BOX);
		location_bar->resizable(location_input);

		tile = new Fl_Tile(0, menubar_height+location_bar_height, default_window_width, default_window_height-menubar_height-location_bar_height-statusbar_height);
		tile->begin();
			dirtree = new DirTree(0, menubar_height+location_bar_height, default_tree_width, default_window_height-menubar_height-location_bar_height-statusbar_height);
			dirtree->when(FL_WHEN_ENTER_KEY_ALWAYS|FL_WHEN_RELEASE_ALWAYS);
			dirtree->callback(tree_cb);

			view = new FileDetailsView(150, menubar_height+location_bar_height, default_window_width-default_tree_width, default_window_height-menubar_height-location_bar_height-statusbar_height);
			view->callback(open_cb);
			// callbacks for file ops
			view->rename_callback(do_rename);
			view->paste_callback(do_paste);
			view->context_callback(context_cb);
		tile->end();

		Fl_Group *sbgroup = new Fl_Group(0, default_window_height-statusbar_height, default_window_width, statusbar_height);
			statusbar = new Fl_Box(2, default_window_height-statusbar_height+2, statusbar_width, statusbar_height-4);
			statusbar->box(FL_DOWN_BOX);
			statusbar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);
			statusbar->label(_("EFiler is starting..."));

			Fl_Box* filler = new Fl_Box(statusbar_width+2, default_window_height-statusbar_height+2, default_window_width-statusbar_width, statusbar_height-4);
		sbgroup->end();
		sbgroup->resizable(filler);

		context_menu = new Fl_Menu_Button (0,0,0,0);
		context_menu->type(Fl_Menu_Button::POPUP3);
		context_menu->menu(context_menu_definition);
		context_menu->box(FL_NO_BOX);

	win->end();
	win->resizable(tile);
//	win->resizable(view);

	// Set application (window manager) icon
	// FIXME: due to fltk bug this icon doesn't have transparency
	fl_open_display();
	Pixmap p, mask;
	XpmCreatePixmapFromData(fl_display, DefaultRootWindow(fl_display), efiler_xpm, &p, &mask, NULL);
	win->icon((char*)p);


	// TODO remember previous configuration
	showhidden=false; dirsfirst=true; ignorecase=true; semaphore=false; showtree=true; showlocation=true;
	tree_width = default_tree_width;

	win->show(argc,argv);
	view->take_focus();
	dirtree->init();
	dirtree->show_hidden(showhidden);
	dirtree->ignore_case(ignorecase);
	loaddir(current_dir);

	return Fl::run();
}
