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


#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h> // timer
#include <sys/resource.h> // for core
#include <sys/vfs.h> // used for statfs()


#include <Fl/Fl.H>
#include <Fl/Fl_Double_Window.H>
#include <Fl/Fl_Menu_Bar.H>
#include <Fl/Fl_File_Chooser.H> // for fl_dir_chooser, used in "Open location"
#include <Fl/filename.H>
#include <Fl/fl_ask.H>

#include <edelib/Nls.h>
#include <edelib/MimeType.h>
#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/Run.h>

#include "EDE_FileView.h" // our file view widget
#include "Util.h" // ex-edelib

#include "fileops.h" // file operations


#define DEFAULT_ICON "misc-vedran"

Fl_Window* win;
FileDetailsView* view;
Fl_Menu_Bar* main_menu;
Fl_Box* statusbar;

char current_dir[FL_PATH_MAX];
bool showhidden;
bool semaphore;
bool dirsfirst;
bool ignorecase;


// constants

const int default_window_width = 600;
const int default_window_height = 400;
const int menubar_height = 30;
const int statusbar_height = 24;
const int statusbar_width = 400;


/*-----------------------------------------------------------------
	Filesystem functions
-------------------------------------------------------------------*/

// Read filesystems - adapted from fltk file chooser
char filesystems[50][PATH_MAX]; // must be global
int get_filesystems() {
	FILE	*mtab;		// /etc/mtab or /etc/mnttab file

	// Results are cached in a static array
	static int fs_number=0;

	// On first access read filesystems
	if (fs_number == 0) {
		mtab = fopen("/etc/mnttab", "r");	// Fairly standard
		if (mtab == NULL)
			mtab = fopen("/etc/mtab", "r");	// More standard
		if (mtab == NULL)
			mtab = fopen("/etc/fstab", "r");	// Otherwise fallback to full list
		if (mtab == NULL)
			mtab = fopen("/etc/vfstab", "r");	// Alternate full list file

		char	line[PATH_MAX];	// Input line
		char	device[PATH_MAX], mountpoint[PATH_MAX], fs[PATH_MAX];
		while (mtab!= NULL && fgets(line, sizeof(line), mtab) != NULL) {
			if (line[0] == '#' || line[0] == '\n')
				continue;
			if (sscanf(line, "%s%s%s", device, mountpoint, fs) != 3)
				continue;
			strcpy(filesystems[fs_number],mountpoint);
			fs_number++;
		}	
		fclose (mtab);

		if (fs_number == 0) return 0; // error reading mtab/fstab
	}
	return fs_number;
}


// Get mount point of filesystem for given file
const char* find_fs_for(const char* file) {
	int fs_number = get_filesystems();
	if (fs_number==0) return 0; // error reading mtab/fstab

	// Find filesystem for file (largest mount point match)
	char *max;
	int maxlen = 0;
	for (int i=0; i<fs_number; i++) {
		int mylen = strlen(filesystems[i]);
		if ((strncmp(file,filesystems[i],mylen)==0) && (mylen>maxlen)) {
			maxlen=mylen;
			max = (char*)filesystems[i];
		}
	}
	if (maxlen == 0) return 0; // doesn't match any fs? there should always be root
	return max;
}


// Tests if two files are on the same filesystem
bool is_on_same_fs(const char* file1, const char* file2) {
	const char* fs1 = find_fs_for(file1);
	// See if file2 matches the same filesystem
	return (strncmp(file1,fs1,strlen(fs1))==0);
}




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
		if (!fp) { fl_alert(_("File openers.txt not found")); return 0; }
		char buf[FL_PATH_MAX*2];
		while (!feof(fp)) {
			fgets((char*)&buf, FL_PATH_MAX*2, fp);
			if (buf[0]=='\0' || buf[1]=='\0' || buf[0]=='#') continue;
			buf[strlen(buf)-1]='\0';
			char *tmp = strstr(buf, "||");
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

void loaddir(const char *path) {
	if (semaphore) return; // Prevent loaddir to interrupt previous loaddir - that can result in crash
	semaphore=true;


	char old_dir[FL_PATH_MAX];
	strncpy(old_dir,current_dir,strlen(current_dir)); // Restore olddir in case of error

	// Set current_dir
	if (fl_filename_isdir(path)) {
		if (path[0] == '~') // Expand tilde
			snprintf(current_dir,PATH_MAX,"%s/%s",getenv("HOME"),path+1);
		else 
			strcpy(current_dir,path);
	} else
		strcpy(current_dir,getenv("HOME"));

	// Trailing slash should always be there
	if (current_dir[strlen(current_dir)-1] != '/') strcat(current_dir,"/");

	// Compact dotdot (..)
	if (char *tmp = strstr(current_dir,"/../")) {
		char *tmp2 = tmp+4;
		tmp--;
		while (tmp != current_dir && *tmp != '/') tmp--;
		tmp++;
		while (*tmp2 != '\0') *tmp++ = *tmp2++;
		*tmp='\0';
	}

fprintf (stderr, "loaddir(%s) = (%s)\n",path,current_dir);

	// Update directory tree
//	dirtree->set_current(current_dir);

	// variables used later
	int size=0;
	dirent **files;

	// List all files in directory
	if (ignorecase) 
		size = scandir(current_dir, &files, 0, myversionsort);
	else
		size = scandir(current_dir, &files, 0, versionsort);

	if (size<1) { // there should always be . and ..
		fl_alert(_("Permission denied!"));
		strncpy(current_dir,old_dir,strlen(current_dir));
		semaphore=false;
		return;
	}

	// set window label
	win->label(tasprintf(_("%s - File manager"), current_dir));

	view->clear();

	FileItem **item_list = new FileItem*[size];
	int fsize=0;

	for (int i=0; i<size; i++) {
		char *n = files[i]->d_name; //shortcut

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
		item->permissions = ""; // todo
		if (strcmp(n,"..")==0) {
			item->icon = "undo";
			item->description = "Go up";
			item->size = "";
		} else if (S_ISDIR(stat_buffer.st_mode)) { // directory
			item->icon = "folder";
			item->description = "Directory";
			// item->name += "/";
			item->size = "";
		} else {
			item->icon = "unknown";
			item->description = "Unknown";
			item->size = nice_size(stat_buffer.st_size);
		}

		item_list[fsize++] = item;
	}


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
	// TODO Attempt to cache the results in a meaningful way
	static struct statfs statfs_buffer;
	if (statfs(current_dir, &statfs_buffer)==0) {
		double totalsize = double(statfs_buffer.f_bsize) * statfs_buffer.f_blocks;
		double freesize = double(statfs_buffer.f_bsize) * statfs_buffer.f_bavail; // This is what df returns
										// f_bfree is size available to root
		double percent = double(statfs_buffer.f_blocks-statfs_buffer.f_bavail)/statfs_buffer.f_blocks*100;
		char *tmp = strdup(nice_size(totalsize)); // nice_size() operates on a static char buffer, we can't use two calls at the same time
		statusbar->label(tasprintf(_("Filesystem %s, Size %s, Free %s (%4.1f%% used)"), find_fs_for(current_dir), tmp, nice_size(freesize), percent));
		free(tmp);
	} else
		statusbar->label(_("Error reading filesystem information!"));
}



/*-----------------------------------------------------------------
	File moving and copying operations
-------------------------------------------------------------------*/


/*-----------------------------------------------------------------
	Main menu callbacks
-------------------------------------------------------------------*/

// Open callback
void open_cb(Fl_Widget*w, void*data) {
fprintf (stderr,"cb\n");
	if (Fl::event_clicks() || Fl::event_key() == FL_Enter || w==main_menu) {
fprintf (stderr,"enter\n");
//if (Fl::event_clicks()) fprintf(stderr, "clicks\n");
//if (Fl::event_key()==FL_Enter) fprintf(stderr, "ekey\n");
		static timeval tm = {0,0};
		timeval newtm;
		gettimeofday(&newtm,0);
		if (newtm.tv_sec - tm.tv_sec < 1 || (newtm.tv_sec-tm.tv_sec==1 && newtm.tv_usec<tm.tv_usec)) return; // no calling within 1 second
		tm=newtm;
		if (view->value()==0) return; // This can happen while efiler is loading

		char* filename = strdup(view->text(view->value()));
		if (char*k = strchr(filename, view->column_char())) *k='\0';

		char* path = (char*)view->data(view->value());
		fprintf(stderr, "Path: %s (ev %d)\n",path,Fl::event());

		if (stat(path,&stat_buffer)) return; // error
		if (S_ISDIR(stat_buffer.st_mode)) {  // directory
			loaddir(path);
			return;
		}

		// Call opener
		mime_resolver.set(path);
		char* opener = simpleopener(mime_resolver.type().c_str());

	// dump core
	struct rlimit *rlim = (struct rlimit*)malloc(sizeof(struct rlimit));
	getrlimit (RLIMIT_CORE, rlim);
	rlim_t old_rlimit = rlim->rlim_cur; // keep previous rlimit
	rlim->rlim_cur = RLIM_INFINITY;
	setrlimit (RLIMIT_CORE, rlim);

		const char *o2 = tsprintf(opener,path);
		fprintf (stderr, "run_program: %s\n", o2);
		if (opener) { 
			int k=edelib::run_program(o2,false); fprintf(stderr, "retval: %d\n", k); 
		} else
			statusbar->label(tasprintf(_("No program to open %s!"), filename));

		free(filename);

	rlim->rlim_cur = old_rlimit;
	setrlimit (RLIMIT_CORE, rlim);

	}
} // open_cb

void location_cb(Fl_Widget*, void*) {
	const char *dir = fl_dir_chooser(_("Choose location"),current_dir);
	if (dir) loaddir(dir);
}

void new_cb(Fl_Widget*, void*) { edelib::run_program(tsprintf("efiler %s",current_dir),false); }

void quit_cb(Fl_Widget*, void*) {exit(0);}

void cut_cb(Fl_Widget*, void*) { do_cut_copy(false); }
void copy_cb(Fl_Widget*, void*) { do_cut_copy(true); }
void paste_cb(Fl_Widget*, void*) { do_paste(); }
void delete_cb(Fl_Widget*, void*) { do_delete(); }


void showhidden_cb(Fl_Widget*, void*) { showhidden=!showhidden; loaddir(current_dir); }

void refresh_cb(Fl_Widget*, void*) { 
	loaddir(current_dir); 
	// TODO: reload directory tree as well-
}

void case_cb(Fl_Widget*, void*) { ignorecase=!ignorecase; loaddir(current_dir); }

void dirsfirst_cb(Fl_Widget*, void*) { dirsfirst=!dirsfirst; loaddir(current_dir); }

// dummy callbacks
void ow_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); } // make a list of openers
void pref_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void iconsview_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void listview_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void showtree_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void about_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }
void aboutede_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); }




/*-----------------------------------------------------------------
	GUI design
-------------------------------------------------------------------*/

// Main window menu - definition
Fl_Menu_Item main_menu_definition[] = {
	{_("&File"),	0, 0, 0, FL_SUBMENU},
		{_("&Open"),		FL_CTRL+'o',	open_cb},
		{_("Open &with..."),	0,		ow_cb,		0,FL_MENU_DIVIDER},
		{_("Open &location"),	0,		location_cb,	0,FL_MENU_DIVIDER},
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
		{_("&Detailed list"),	FL_F+8,		listview_cb,	0,	FL_MENU_RADIO|FL_MENU_VALUE|FL_MENU_DIVIDER}, 
		{_("&Show hidden"),	0,		showhidden_cb,	0,	FL_MENU_TOGGLE},
		{_("Directory &tree"),	FL_F+9,		showtree_cb,	0,	FL_MENU_TOGGLE|FL_MENU_DIVIDER}, // coming soon
		{_("&Refresh"),		FL_F+5,		refresh_cb},
		{_("S&ort"),	0, 0, 0, FL_SUBMENU},
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

int main (int argc, char **argv) {

fl_register_images();
edelib::IconTheme::init("crystalsvg");


	win = new Fl_Double_Window(default_window_width, default_window_height);
//	win->color(FL_WHITE);
	win->begin();
		main_menu = new Fl_Menu_Bar(0,0,default_window_width,menubar_height);
		main_menu->menu(main_menu_definition);
		main_menu->textsize(12); // hack for label size

		view = new FileDetailsView(0, menubar_height, default_window_width, default_window_height-menubar_height-statusbar_height, 0);
		view->callback(open_cb);
		// callback for renaming
		view->rename_callback(do_rename);

		Fl_Group *sbgroup = new Fl_Group(0, default_window_height-statusbar_height, default_window_width, statusbar_height);
			statusbar = new Fl_Box(2, default_window_height-statusbar_height+2, statusbar_width, statusbar_height-4);
			statusbar->box(FL_DOWN_BOX);
			statusbar->labelsize(12); // hack for label size
			statusbar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);
			statusbar->label(_("Ready."));

			Fl_Box* filler = new Fl_Box(statusbar_width+2, default_window_height-statusbar_height+2, default_window_width-statusbar_width, statusbar_height-4);
		sbgroup->end();
		sbgroup->resizable(filler);

	win->end();
	win->resizable(view);
//	win->icon(Icon::get("folder",Icon::TINY));
	win->show(argc,argv);

	// TODO remember previous configuration
	showhidden=false; dirsfirst=true; ignorecase=true; semaphore=false;

	if (argc==1) // No params
		loaddir ("");
	else 
		loaddir (argv[1]);

	return Fl::run();
}
