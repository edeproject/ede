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
#include <unistd.h> // for getuid & getgid

// for application icon:
#include <X11/xpm.h>
#include <FL/x.H> 
#include "efiler.xpm"


#include <Fl/Fl.H>
#include <Fl/Fl_Menu_Bar.H>
#include <Fl/Fl_Menu_Button.H>
#include <Fl/filename.H>
#include <Fl/Fl_File_Input.H> // location bar

#include <edelib/Directory.h>
#include <edelib/DirWatch.h>
#include <edelib/IconTheme.h> // for setting the icon theme
#include <edelib/MessageBox.h>
#include <edelib/MimeType.h>
#include <edelib/Nls.h>
#include <edelib/Resource.h>
#include <edelib/Run.h>
#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/Window.h>

#include "EDE_FileView.h" // our file view widget
#include "EDE_DirTree.h" // directory tree
#include "Util.h" // ex-edelib

#include "fileops.h" // file operations
#include "filesystem.h" // filesystem support
#include "ede_strverscmp.h" // local copy of strverscmp
#include "mailcap.h" // handling mailcap file



edelib::Window* win;
FileView* view;
Fl_Menu_Bar* main_menu;
Fl_Box* statusbar;
DirTree* dirtree;
Fl_Tile* tile;
Fl_Group* location_bar;
Fl_File_Input* location_input;
Fl_Menu_Button* context_menu;

edelib::Resource app_config;
edelib::MimeType mime_resolver;

struct stat64 stat_buffer;

// global values

char current_dir[FL_PATH_MAX];
bool showhidden;
bool semaphore;
bool dirsfirst;
bool ignorecase;
bool showtree;
bool showlocation;
int tree_width;
bool notify_available;


// constants

const int default_window_width = 600;
const int default_window_height = 400;
const int menubar_height = 30;
const int location_bar_height = 40;
const int statusbar_height = 24;
const int statusbar_width = 400;
const bool dumpcore_on_exec = true; // if external handler crashes

int default_tree_width=150;



/*-----------------------------------------------------------------
	Subclassed Window for handling F8 key to switch between
	icon view and list view
-------------------------------------------------------------------*/

void iconsview_cb(Fl_Widget*, void*);
void listview_cb(Fl_Widget*, void*); 

class EFiler_Window : public edelib::Window {
public:
	EFiler_Window (int W, int H, const char* title=0) : edelib::Window(W,H,title) {}
	EFiler_Window (int X, int Y, int W, int H, const char* title=0) : 
		edelib::Window(X,Y,W,H,title) {}

	int handle(int e) {
		// Have F8 function as switch between active views
		if (e == FL_KEYBOARD && Fl::event_key()==FL_F+8) {
			void*p;
			if (view->type() == FILE_DETAILS_VIEW) {
				iconsview_cb(main_menu,p);
			} else {
				listview_cb(main_menu,p);
			}
			return 1;
		}
		return edelib::Window::handle(e);
	}
};



/*-----------------------------------------------------------------
	Some improvements to Fl_File_Input, that should be added 
	upstream:
	* enable to set shortcut in label (STR 1770 for Fl_Input_)
	* navigate using Ctrl+arrow (jump to dir separator)
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
	loaddir() and friends
-------------------------------------------------------------------*/


// Use local copy of strverscmp
int ede_versionsort(const void *a, const void *b) {
	struct dirent** ka = (struct dirent**)a;
	struct dirent** kb = (struct dirent**)b;
	return ede_strverscmp((*ka)->d_name,(*kb)->d_name);
}

// Modification of versionsort which ignores case
int ede_versioncasesort(const void *a, const void *b) {
	struct dirent** ka = (struct dirent**)a;
	struct dirent** kb = (struct dirent**)b;
	char* ma = strdup((*ka)->d_name);
	char* mb = strdup((*kb)->d_name);
	edelib::str_tolower((unsigned char*)ma); edelib::str_tolower((unsigned char*)mb);
	int k = ede_strverscmp(ma,mb);
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

// This function loads list of files in path into main view
void loaddir(const char *path) {
	if (semaphore) return; // Prevent loaddir to interrupt previous loaddir - that can result in crash
	semaphore=true;


	char old_dir[FL_PATH_MAX];
	strncpy(old_dir,current_dir,strlen(current_dir)); // Restore olddir in case of error

	// Sometimes path is just a pointer to current_dir
	char* tmpath = strdup(path);

	// Set current_dir  ( fl_filename_isdir() thinks that / isn't a dir :(
	if (strcmp(path,"/")==0 || fl_filename_isdir(path)) {
		if (path[0] == '~' && path[1] == '/') {// Expand tilde
			snprintf(current_dir,FL_PATH_MAX,"%s/%s",edelib::dir_home().c_str(),tmpath+1);
		} else if (path[0] != '/') { // Relative path
			char *t = tmpath;
			if (path[0] == '.' && path[1] == '/') t+=2; // remove '.'
			else if (path[0] == '.' && path[1] != '.') t+=1;
			snprintf(current_dir,FL_PATH_MAX,"%s/%s",edelib::dir_current().c_str(),t);
		} else {
			if (path!=current_dir) strncpy(current_dir,tmpath,strlen(tmpath)+1);
		}
	} else {
		edelib::alert(_("Directory not found: %s"),path);
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
		size = scandir(current_dir, &files, 0, ede_versioncasesort);
	else
		size = scandir(current_dir, &files, 0, ede_versionsort);

	if (size<1) { // there should always be at least '.' and '..'
		edelib::alert(_("Permission denied!"));
		strncpy(current_dir,old_dir,strlen(current_dir));
		semaphore=false;
		return;
	}

	// Ok, now we know everything is fine...

	// Remove old watch and add new one
	edelib::DirWatch::remove(old_dir);
	edelib::DirWatch::add(current_dir, edelib::DW_CREATE | edelib::DW_DELETE | edelib::DW_ATTRIB | edelib::DW_RENAME | edelib::DW_MODIFY);

	// Update directory tree
	if (showtree) dirtree->set_current(current_dir);
	location_input->value(current_dir);

	// set window label
	// copy_label() is a method that calls strdup() and later free()
	win->copy_label(tsprintf(_("%s - File manager"), my_filename_name(current_dir)));
	statusbar->copy_label(tsprintf(_("Scanning directory %s..."), current_dir)); 

	view->clear();

	FileItem **item_list = new FileItem*[size];
	int fsize=0;

	for (int i=0; i<size; i++) {
		char *n = files[i]->d_name; //shortcut
		if (i>0) free(files[i-1]); // see scandir(3)

		// don't show . (current directory)
		if (strcmp(n,".")==0) continue;

		// hide files with dot except .. (up directory)
		if (!showhidden && (n[0] == '.') && (strcmp(n,"..")!=0)) continue;

		// hide files ending with tilde (backup)
		if (!showhidden && (n[strlen(n)-1] == '~')) continue;

		char fullpath[FL_PATH_MAX];
		snprintf (fullpath,FL_PATH_MAX-1,"%s%s",current_dir,n);

		if (stat64(fullpath,&stat_buffer)) continue; // happens when user has traverse but not read privilege

		FileItem *item = new FileItem;
		item->name = n;
		item->realpath = fullpath;
		item->date = nice_time(stat_buffer.st_mtime);
		item->permissions = permission_text(stat_buffer.st_mode,stat_buffer.st_uid,stat_buffer.st_gid);
		if (strcmp(n,"..")==0) {
			// item->icon = "go-up"; // undo is prettier?!
			// in edeneu theme, I prefer go-jump rotated 180 degrees... in crystalsvg, undo is best
			item->icon = "edit-undo";
			item->description = "Go up";
			item->size = "";
		} else if (S_ISDIR(stat_buffer.st_mode)) { // directory
			item->icon = "folder";
			item->description = "Directory";
			// item->name += "/";
			item->size = "";
			item->realpath += "/";
		} else {
			item->icon = "empty"; //in crystalsvg "misc" is better...
			// NOTE: "empty" is *not* listed in icon-naming-spec [1], but is present in 
			// both KDE and GNOME themes.
			// [1] http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-0.8.html
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
				view->update(item_list[i]);
			}
			Fl::check(); // keep interface responsive while updating mimetypes
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
	file_open()
-------------------------------------------------------------------*/

// This function will open a file using a handler according to its MIME type
// and the action parameter
void file_open(const char* path, MailcapAction action) {

	if (stat64(path,&stat_buffer)) return; // stat error
	if (S_ISDIR(stat_buffer.st_mode)) {  // directories are handled internally
		loaddir(path);
		return;
	}

	// Find opener
	mime_resolver.set(path);
//	char* opener = simpleopener(mime_resolver.type().c_str());
	const char* opener = mailcap_opener(mime_resolver.type().c_str(), action);

	struct rlimit *rlim; rlim_t old_rlimit;
	if (dumpcore_on_exec) {
		rlim = (struct rlimit*)malloc(sizeof(struct rlimit));
		getrlimit (RLIMIT_CORE, rlim);
		old_rlimit = rlim->rlim_cur; // keep previous rlimit
		rlim->rlim_cur = RLIM_INFINITY;
		setrlimit (RLIMIT_CORE, rlim);
	}

	const char *o2 = tsprintf(opener,path); // opener should contain %s
	fprintf (stderr, "run_program: %s\n", o2);

	if (action == MAILCAP_EXEC) {
		int k=edelib::run_program(path,false); fprintf(stderr, "retval: %d\n", k); 
	} else if (opener) {
		int k=edelib::run_program(o2,false); fprintf(stderr, "retval: %d\n", k); 
	} else {
		statusbar->copy_label(tsprintf(_("No program to open %s! (%s)"), fl_filename_name(path),mime_resolver.type().c_str()));
		// TODO: open with...
	}

	if (dumpcore_on_exec) {
		rlim->rlim_cur = old_rlimit;
		setrlimit (RLIMIT_CORE, rlim);
		free(rlim);
	}
}



/*-----------------------------------------------------------------
	Main menu and other callbacks
-------------------------------------------------------------------*/


// ------------ Main menu callbacks


// File menu

// File > Open
// This callback is also called by doubleclicking on file list and from context menu
void open_cb(Fl_Widget*w, void*data) {
	// Ignore any other caller / event
	if (Fl::event_clicks() || Fl::event_key() == FL_Enter || w==main_menu || w==context_menu) {
		// Prevent calling two times rapidly
		// (e.g. sloppy double/triple/quadruple click)
		const int min_secs = 1; // min. number of secs between two calls
		static timeval tm = {0,0};
		timeval newtm;
		gettimeofday(&newtm,0);
		if (newtm.tv_sec - tm.tv_sec < min_secs || (newtm.tv_sec-tm.tv_sec==min_secs && newtm.tv_usec<tm.tv_usec)) return; 
		tm=newtm;

		// Nothing is selected!?
		// This can happen while efiler is loading
		if (view->get_focus()==0) return; 

		file_open(view->path(view->get_focus()), MAILCAP_VIEW);
	}
}

// File > Open with...
// TODO: make a list of openers etc.
void openwith_cb(Fl_Widget*, void*) {
	const char* app = edelib::input(_("Enter the name of application to open this file with:"));
	const char* file = view->path(view->get_focus());
	edelib::run_program(tsprintf("%s '%s'", app, file), /*wait=*/false);
} 

// File > New (efiler window)
void new_cb(Fl_Widget*, void*) {
	// FIXME: use program path, in case it's not in PATH
	edelib::run_program(tsprintf("efiler %s",current_dir),false); 
}

// File > Quit
void quit_cb(Fl_Widget*, void*) { win->hide(); }

// Options in Edit menu
void cut_cb(Fl_Widget*, void*) { do_cut_copy(false); }
void copy_cb(Fl_Widget*, void*) { do_cut_copy(true); }
void paste_cb(Fl_Widget*, void*) { Fl::paste(*view,1); } // view->handle() will call do_paste()
void delete_cb(Fl_Widget*, void*) { do_delete(); }
void viewrename_cb(Fl_Widget*, void*) { view->start_rename(); } // Just call view's rename functionality

// Options in View menu

// View > Directory tree
void showtree_cb(Fl_Widget*, void*) {
	showtree = !showtree;
	if (!showtree) {
		tree_width = dirtree->w();
		tile->position(default_tree_width, 1, 1, 1); // NOTE this doesn't always work!
	} else {
		int currentw = dirtree->w();
		tile->position(currentw, 1, tree_width, 1);
		dirtree->set_current(current_dir);
	}
}

// View > Refresh (and also F5 key)
void refresh_cb(Fl_Widget*, void*) { 
	loaddir(current_dir); 
}

// View > Location bar
void locationbar_cb(Fl_Widget*, void*) {
	showlocation = !showlocation;
	if (showlocation) {
		location_bar->show();
		location_bar->resize(0, menubar_height, win->w(), location_bar_height);
		tile->resize(0, menubar_height+location_bar_height, win->w(), win->h()-menubar_height-location_bar_height-statusbar_height);
		win->init_sizes();
		win->redraw();
	} else {
		location_bar->resize(0, menubar_height, win->w(), 0);
		tile->resize(0, menubar_height, win->w(), win->h()-menubar_height-statusbar_height);
		location_bar->hide();
		win->init_sizes();
		win->redraw();
	}
}

// View > Hidden files
void showhidden_cb(Fl_Widget*, void*) { 
	showhidden=!showhidden; 
	dirtree->show_hidden(showhidden);
	dirtree->reload(); 
	loaddir(current_dir); 
}

// View > Sort > Ignore case
void case_cb(Fl_Widget*, void*) { 
	ignorecase=!ignorecase;
	dirtree->ignore_case(ignorecase);
	dirtree->reload(); 
	loaddir(current_dir); 
}

// View > Sort > Directories first
void dirsfirst_cb(Fl_Widget*, void*) { 
	dirsfirst=!dirsfirst; 
	loaddir(current_dir); 
}


// This fn changes menu and so must be implemented after menu definition
void update_menu_item(const char* label, bool value);

// View > Icons
void iconsview_cb(Fl_Widget*, void*) {
	if (view->type() != FILE_ICON_VIEW) {  
		view->type(FILE_ICON_VIEW);
		loaddir(current_dir);
		update_menu_item(_("&Icons"),true);
		update_menu_item(_("&Detailed list"),false);
	}
}

// View > Detailed list
void listview_cb(Fl_Widget*, void*) { 
	if (view->type() != FILE_DETAILS_VIEW) {
		view->type(FILE_DETAILS_VIEW); 
		loaddir(current_dir);
		update_menu_item(_("&Icons"),false);
		update_menu_item(_("&Detailed list"),true);
	}
}

// Help > About efiler
void about_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }

// Help > About EDE
void aboutede_cb(Fl_Widget*, void*) { 
	// Sanel created an external app for About EDE called eabout
	edelib::run_program("eabout", /*wait=*/false);
}




// ------------ Other callbacks


// Context menu callbacks (items are displayed depending on type)
void fileedit_cb(Fl_Widget*w, void*) {
	file_open(view->path(view->get_focus()), MAILCAP_EDIT);
}
void fileprint_cb(Fl_Widget*w, void*) {
	file_open(view->path(view->get_focus()), MAILCAP_PRINT);
}
void fileexec_cb(Fl_Widget*w, void*) {
	file_open(view->path(view->get_focus()), MAILCAP_EXEC);
}
void pref_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
// Other options use callbacks from Edit menu


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


// Callback called by edelib::DirWatch
void directory_change_cb(const char* dir, const char* what_changed, int flags, void* d) {
	if (!what_changed || flags==edelib::DW_REPORT_NONE) return; // edelib whas unable to figure out what exactly changed
	if (strcmp(current_dir,dir)) return; // for some reason we're being notified about non-current dir

	// Find item in view
	int found=-1;
	for (int i=1; i<=view->size(); i++) {
		const char *item = view->path(i);
		if (strcmp(item+strlen(dir),what_changed)==0) { found=i; break; }
	}

	// If item was changed, it's simplest to remove it from the list then add it again
	if (found>-1) view->remove(found);
	if (flags==edelib::DW_REPORT_DELETE) return; // delete ends here

	// Adding new item - code mostly copied from loaddir()
	// TODO: create a separate function for this
	char fullpath[FL_PATH_MAX];
	snprintf (fullpath,FL_PATH_MAX-1,"%s%s",dir,what_changed);

	if (stat64(fullpath,&stat_buffer)) return; // happens when user has traverse but not read privilege

	FileItem *item = new FileItem;
	item->name = what_changed;
	item->realpath = fullpath;
	item->date = nice_time(stat_buffer.st_mtime);
	item->permissions = permission_text(stat_buffer.st_mode,stat_buffer.st_uid,stat_buffer.st_gid);
	if (strcmp(what_changed,"..")==0) {
		item->icon = "edit_undo";
		item->description = "Go up";
		item->size = "";
	} else if (S_ISDIR(stat_buffer.st_mode)) { // directory
		item->icon = "folder";
		item->description = "Directory";
		// item->name += "/";
		item->size = "";
		item->realpath += "/";
	} else {
		item->icon = "empty";
		item->description = "Unknown";
		item->size = nice_size(stat_buffer.st_size);
	}

	view->add(item);
	view->redraw();
	Fl::check(); // update interface to give time for mimetype resolver

	mime_resolver.set(fullpath);
	edelib::String desc,icon;
	desc = mime_resolver.comment();
	// First letter of desc should be upper case:
	if (desc.length()>0 && desc[0]>='a' && desc[0]<='z') desc[0] = desc[0]-'a'+'A';
	icon = mime_resolver.icon_name();
	if (desc!="" || icon!="") {
		if (desc != "") item->description = desc;
		if (icon != "") item->icon = icon;
		view->update(item);
	}
	view->redraw();
}


// Function returns true if file is executable
bool file_executable(const char* path) {
	if (stat64(path,&stat_buffer)) return false; // error

	static uid_t uid = stat_buffer.st_uid, uuid = getuid();
	static gid_t gid = stat_buffer.st_gid, ugid = getgid();
	mode_t mode = stat_buffer.st_mode;

	if (S_ISDIR(mode)) return false;
	
	if (uid == uuid && mode&S_IXUSR) return true;
	if (gid == ugid && mode&S_IXGRP) return true;
	if (mode&S_IXOTH) return true;
	return false;
}



/*-----------------------------------------------------------------
	Load / store user preferences
-------------------------------------------------------------------*/

void load_preferences() {
	bool icons_view=false;
	int winw, winh;

	app_config.load("ede/efiler");
	app_config.get("gui","show_hidden",showhidden,false); // Show hidden files
	app_config.get("gui","dirs_first",dirsfirst,true); // Directories before ordinary files
	app_config.get("gui","ignore_case",ignorecase,true); // Ignore case when sorting
	app_config.get("gui","show_location",showlocation,true); // Show location bar
	app_config.get("gui","show_tree",showtree,false); // Show directory tree
	app_config.get("gui","icons_view",icons_view,false); // Show icons (if false, show details)

	app_config.get("gui","window_width",winw,600); // Window dimensions
	app_config.get("gui","window_height",winh,400);

	// Apply settings
	if (winw!=default_window_width || winh!=default_window_height)
		win->resize(win->x(),win->y(),winw,winh);

	// Location and tree are currently shown - we just need to hide them if neccessary

	if (!showlocation) {
		showlocation = !showlocation; //locationbar_cb will revert
		locationbar_cb(win,0); 
		update_menu_item(_("&Location bar"),showlocation);
	}
	if (!showtree) {
		showtree = !showtree; // showtree_cb will revert
		showtree_cb(win,0);
		update_menu_item(_("Directory &tree"),showtree);
	}
	if (icons_view) iconsview_cb(win,0); else listview_cb(win,0);
	update_menu_item(_("&Hidden files"),showhidden);
	update_menu_item(_("D&irectories first"),dirsfirst);
}


void save_preferences() {
	app_config.set("gui","show_hidden",showhidden); // Show hidden files
	app_config.set("gui","dirs_first",dirsfirst); // Directories before ordinary files
	app_config.set("gui","ignore_case",ignorecase); // Ignore case when sorting
	app_config.set("gui","show_location",showlocation); // Show location bar
	app_config.set("gui","show_tree",showtree); // Show directory tree
	app_config.set("gui","icons_view",(view->type()==FILE_ICON_VIEW)); // Show icons (if false, show details)

	app_config.set("gui","window_width",win->w()); // Window dimensions
	app_config.set("gui","window_height",win->h());

	app_config.save("ede/efiler");
}



/*-----------------------------------------------------------------
	Menu definitions
-------------------------------------------------------------------*/


// Main menu
Fl_Menu_Item main_menu_definition[] = {
	{_("&File"),	0, 0, 0, FL_SUBMENU},
		{_("&Open"),		FL_CTRL+'o',	open_cb},
		{_("Open &with..."),	0,		openwith_cb,	0,FL_MENU_DIVIDER},
//		{_("Open &location"),	0,		location_cb,	0,FL_MENU_DIVIDER},
		{_("&New window"),	FL_CTRL+'n',	new_cb,		0,FL_MENU_DIVIDER},
		{_("&Quit"),		FL_CTRL+'q',	quit_cb},
		{0},

	{_("&Edit"),	0, 0, 0, FL_SUBMENU},
		{_("&Cut"),		FL_CTRL+'x',	cut_cb},
		{_("C&opy"),		FL_CTRL+'c',	copy_cb},
		{_("&Paste"),		FL_CTRL+'v',	paste_cb},
		{_("&Rename"),		FL_F+2,		viewrename_cb},
		{_("&Delete"),		FL_Delete,	delete_cb,	0,	FL_MENU_DIVIDER},
		{_("Pre&ferences"),	FL_CTRL+'p',	pref_cb},
		{0},

	{_("&View"),	0, 0, 0, FL_SUBMENU},
		{_("&Icons"),		0,		iconsview_cb,	0,	FL_MENU_TOGGLE},
		{_("&Detailed list"),	0,		listview_cb,	0,	FL_MENU_TOGGLE|FL_MENU_VALUE|FL_MENU_DIVIDER}, 
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


// Update checkbox in main menu
// (needs to come after the menu definition)
void update_menu_item(const char* label, bool value) {
	Fl_Menu_Item *k = main_menu_definition;
	while(k!=0 && k+1!=0) {
		if (k->label()!=0 && strcmp(k->label(),label)==0) break;
		k++;
	}
	if (k!=0)
		if (value) k->set(); else k->clear();
}


// Context menu - shown on right click in view (see context_cb())
Fl_Menu_Item context_menu_definition[] = {
	{_("&Open"),		0,	open_cb,},
	{_("&View"),		0,	open_cb,	0, FL_MENU_INVISIBLE},
	{_("&Edit"),		0,	fileedit_cb,	0, FL_MENU_INVISIBLE},
	{_("Pri&nt"),		0,	fileprint_cb,	0, FL_MENU_INVISIBLE},
	{_("&Execute"),		0,	fileexec_cb,	0,FL_MENU_INVISIBLE},
	{_("Open &with..."),	0,	openwith_cb,	0,FL_MENU_DIVIDER},
	{_("&Cut"),		0,	cut_cb},
	{_("Co&py"),		0,	copy_cb},
	{_("Pa&ste"),		0,	paste_cb},
	{_("&Delete"),		0,	delete_cb,	0,FL_MENU_DIVIDER},
	{_("P&references..."),	0,	pref_cb},
	{0}
};


// Right click - show context menu
void context_cb(Fl_Widget*, void*) {
	char* path = (char*)view->path(view->get_focus());

	mime_resolver.set(path);
	int actions = mailcap_actions(mime_resolver.type().c_str());

	// Update context menu to show only options relevant to this file type
	Fl_Menu_Item *k = context_menu_definition;
	while(k->label()!=0) {
		if (k->label()!=0 && strcmp(k->label(),_("&Open"))==0)
			if (actions&MAILCAP_EDIT) k->hide(); else k->show();
		if (k->label()!=0 && (strcmp(k->label(),_("&View"))==0 || strcmp(k->label(),_("&Edit"))==0))
			if (actions&MAILCAP_EDIT) k->show(); else k->hide();
		if (k->label()!=0 && strcmp(k->label(),_("Pri&nt"))==0)
			if (actions&MAILCAP_PRINT) k->show(); else k->hide();
		if (k->label()!=0 && strcmp(k->label(),_("&Execute"))==0)
			if (file_executable(path)) k->show(); else k->hide();
		k++;
	}

	context_menu->popup();
	context_menu->value(-1);
}


/*-----------------------------------------------------------------
	main()
-------------------------------------------------------------------*/

// Parser for Fl::args()
int parsecmd(int argc, char**argv, int &index) {
	if ((strcmp(argv[index],"-ic")==0) || (strcmp(argv[index],"-icons")==0)) {
		edelib::IconTheme::init(argv[++index]);
		++index;
		return 1;
	}
	return 0;
}

// Main program
extern int FL_NORMAL_SIZE;
int main (int argc, char **argv) {

	// Parse command line - this must come first
	int unknown=0;
	Fl::args(argc,argv,unknown,parsecmd);
	if (unknown==argc)
		snprintf(current_dir, FL_PATH_MAX, edelib::dir_home().c_str());
	else {
		if (strcmp(argv[unknown],"--help")==0) {
			printf(_("EFiler - EDE File Manager\nPart of Equinox Desktop Environment (EDE).\nCopyright (c) 2000-2007 EDE Authors.\n\nThis program is licenced under terms of the\nGNU General Public Licence version 2 or newer.\nSee COPYING for details.\n\n"));
			printf(_("Usage:\n\tefiler [OPTIONS] [PATH]\n\n"));
			printf(_("Options:\n -ic[ons] icon theme\n%s\n"),Fl::help);
			return 1;
		}
		strncpy(current_dir, argv[unknown], strlen(argv[unknown])+1);
	}

	edelib::DirWatch::init();
	edelib::DirWatch::callback(directory_change_cb);
	// Let other components know if we have notify
	if (edelib::DirWatch::notifier() == edelib::DW_NONE)
		notify_available=false;
	else
		notify_available=true;

FL_NORMAL_SIZE=12;
//fl_message_font(FL_HELVETICA, 12);

	// Main GUI design
	win = new EFiler_Window(default_window_width, default_window_height);

	win->init(); // new method required by edelib::Window

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

			view = new FileView(150, menubar_height+location_bar_height, default_window_width-default_tree_width, default_window_height-menubar_height-location_bar_height-statusbar_height);
			view->callback(open_cb);
			// callbacks for file ops
			view->rename_callback(do_rename);
			view->paste_callback(do_paste);
			view->context_callback(context_cb);
		tile->end();

		// Status bar group
		Fl_Group *sbgroup = new Fl_Group(0, default_window_height-statusbar_height, default_window_width, statusbar_height);
			statusbar = new Fl_Box(2, default_window_height-statusbar_height+2, //statusbar_width,
			default_window_width-4, statusbar_height-4);
			statusbar->box(FL_DOWN_BOX);
			statusbar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);
			statusbar->label(_("EFiler is starting..."));
		sbgroup->end();
		sbgroup->resizable(statusbar);

		context_menu = new Fl_Menu_Button (0,0,0,0);
		context_menu->type(Fl_Menu_Button::POPUP3);
		context_menu->menu(context_menu_definition);
		context_menu->box(FL_NO_BOX);

	win->end();
	win->resizable(tile);
//	win->resizable(view);

	// Set application (window manager) icon
	// TODO: use icon from theme (e.g. system-file-manager)
	win->window_icon(efiler_xpm); // new method in edelib::Window

	// We need to init dirtree before loading anything into it
	dirtree->init();
	dirtree->show_hidden(showhidden);
	dirtree->ignore_case(ignorecase);

	// Read user preferences
	load_preferences();

	//Fl::visual(FL_DOUBLE|FL_INDEX); // see Fl_Double_Window docs
	semaphore=false; // semaphore for loaddir

	win->show(argc,argv);
	view->take_focus();
	loaddir(current_dir);

	// Main event loop
	Fl::run();

	// Cleanup and shutdowns
	edelib::DirWatch::shutdown();
	save_preferences();

	return 0;
}
