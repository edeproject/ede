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
#include <unistd.h> // edelib/Run needs it :(
#include <sys/resource.h> // for core
#include <sys/vfs.h> // used for statfs()


#include <Fl/Fl.H>
#include <Fl/Fl_Double_Window.H>
#include <Fl/Fl_Menu_Bar.H>
#include <Fl/Fl_File_Chooser.H> // for fl_dir_chooser, used in "Open location"
#include <Fl/Fl_Progress.H>
#include <Fl/Fl_Button.H>
#include <Fl/filename.H>
#include <Fl/fl_ask.H>

#include <edelib/Nls.h>
#include <edelib/MimeType.h>
#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/Run.h>
#include <edelib/File.h>

#include "EDE_FileView.h"
#include "Util.h"


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

		if (stat(fullpath,&stat_buffer)) continue; // error

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
	if (dirsfirst) {
		for (int i=0; i<fsize; i++) {
			if (item_list[i]->description == "Directory" || item_list[i]->description == "Go up")
				view->add(item_list[i]);
		}
		for (int i=0; i<fsize; i++) 
			if (item_list[i]->description != "Directory" && item_list[i]->description != "Go up")
				view->add(item_list[i]);
	} else {
		for (int i=0; i<fsize; i++) 
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
			if (desc[0]>='a' && desc[0]<='z') desc[0] = desc[0]-'a'+'A';
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

Fl_Progress* cut_copy_progress;
bool stop_now;
bool overwrite_all, skip_all;

char **cut_copy_buffer = 0;
bool operation_is_copy = false;



// Execute cut or copy operation when List View is active
void do_cut_copy(bool m_copy) {

	// Count selected icons, for malloc
	int num = view->size();
	int nselected = 0;
	for (int i=1; i<=num; i++)
		if (view->selected(i)) {
			nselected++;

			// Can not cut/copy the up directory button (..)
			const char* tmp = view->text(i);
			if (tmp[0]=='.' && tmp[1]=='.' && (tmp[2]=='\0' || tmp[2]==view->column_char())) return;
		}

	// Clear cut/copy buffer and optionally ungray the previously cutted icons
	if (cut_copy_buffer) {
		for (int i=0; cut_copy_buffer[i]; i++)
			free(cut_copy_buffer[i]);
		free(cut_copy_buffer);
		if (!operation_is_copy) {
			for (int i=1; i<=num; i++)
				view->ungray(i);
		}
	}

	// Allocate buffer
	cut_copy_buffer = (char**)malloc(sizeof(char*) * (nselected+2));

	// Add selected files to buffer and optionally grey icons (for cut effect)
	int buf=0;
	for (int i=1; i<=num; i++) {
		if (view->selected(i)==1) {
			cut_copy_buffer[buf] = strdup((char*)view->data(i));
			if (!m_copy) view->gray(i);
			buf++;
		}
	}
	if (buf==0) { //nothing selected, use the focused item
		int i=view->get_focus();
		cut_copy_buffer[buf] = strdup((char*)view->data(i));
		if (!m_copy) view->gray(i);
		buf++;
		nselected=1;
	}

	cut_copy_buffer[buf] = 0;
	operation_is_copy = m_copy;

	// Deselect all
	int focused = view->get_focus();
	for (int i=1; i<=num; i++)
		if (view->selected(i)) view->select(i,0);
	view->set_focus(focused);

	// Update statusbar
	if (m_copy)
		statusbar->label(tasprintf(_("Selected %d items for copying"), nselected));
	else
		statusbar->label(tasprintf(_("Selected %d items for moving"), nselected));
}


// Helper functions for paste:

// Copy single file. Returns true if operation should continue
bool my_copy(const char* src, const char* dest) {
	FILE *fold, *fnew;
	int c;

	if (strcmp(src,dest)==0)
		// this shouldn't happen
		return true;

	if (edelib::file_exists(dest)) {
		// if both src and dest are directories, do nothing
		if (fl_filename_isdir(src) && fl_filename_isdir(dest))
			return true;

		int c = -1;
		if (!overwrite_all && !skip_all) {
			// here was choice_alert
			c = fl_choice(tsprintf(_("File already exists: %s. What to do?"), dest), _("&Overwrite"), _("Over&write all"), _("*&Skip"), _("Skip &all"), 0); // asterisk (*) means default
		}
		if (c==1) overwrite_all=true;
		if (c==3) skip_all=true;
		if (c==2 || skip_all) {
			return true;
		}
		// At this point either c==0 (overwrite) or overwrite_all == true

		// copy directory over file
		if (fl_filename_isdir(src))
			unlink(dest);

		// copy file over directory
		// TODO: we will just skip this case, but ideally there should be
		// another warning
		if (fl_filename_isdir(dest))
			return true;
	}

	if (fl_filename_isdir(src)) {
		if (mkdir (dest, umask(0))==0)
			return true; // success
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot create directory %s"),dest), _("*&Continue"), _("&Stop"), 0);
		if (q == 0) return true; else return false;
	}

	if ( ( fold = fopen( src, "rb" ) ) == NULL ) {
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot read file %s"),src), _("*&Continue"), _("&Stop"), 0);
		if (q == 0) return true; else return false;
	}

	if ( ( fnew = fopen( dest, "wb" ) ) == NULL  )
	{
		fclose ( fold );
			// here was choice_alert
		int q = fl_choice(tsprintf(_("Cannot create file %s"),dest), _("*&Continue"), _("&Stop"), 0);
		if (q == 0) return true; else return false;
	}
	
	while (!feof(fold)) {
		c = fgetc(fold);
		fputc(c, fnew);
	}
	// TODO: Add more error handling using ferror()
	fclose(fold);
	fclose(fnew);
	return true;
}


// Recursive function that creates a list of all files to be 
// copied, expanding directories etc. The total number of elements
// will be stored in listsize. Returns false if user decided to
// interrupt copying.
bool create_list(const char* src, char** &list, int &list_size, int &list_capacity) {
	// Grow list if neccessary
	if (list_size >= list_capacity-1) {
		list_capacity += 1000;
		list = (char**)realloc(list, sizeof(char**)*list_capacity);
	}

	// We add both files and diretories to list
	list[list_size++] = strdup(src);

	if (fl_filename_isdir(src)) {
		char new_src[PATH_MAX];
		dirent	**files;
		// FIXME: use same sort as currently used
		// FIXME: detect errors on accessing folder
		int num_files = fl_filename_list(src, &files, fl_casenumericsort);
		for (int i=0; i<num_files; i++) {
			if (strcmp(files[i]->d_name,"./")==0 || strcmp(files[i]->d_name,"../")==0) continue;
			snprintf(new_src, PATH_MAX, "%s%s", src, files[i]->d_name);
			Fl::check(); // update gui
			if (stop_now || !create_list(new_src, list, list_size, list_capacity))
				return false;
		}
	}
	return true;
}


// Callback for Stop button on progress window
void stop_copying_cb(Fl_Widget*,void* v) {
	stop_now=true;
	// Let's inform user that we're stopping...
	Fl_Box* caption = (Fl_Box*)v;
	caption->label(_("Stopping..."));
	caption->redraw();
	caption->parent()->redraw();
}

// Execute paste operation - this will copy or move files based on last 
// operation
void do_paste() {

	if (!cut_copy_buffer || !cut_copy_buffer[0]) return;

	overwrite_all=false; skip_all=false;

	// Moving files on same filesystem is trivial, just like rename
	// We don't even need a progress bar
	if (!operation_is_copy && is_on_same_fs(current_dir, cut_copy_buffer[0])) {
		for (int i=0; cut_copy_buffer[i]; i++) {
			char *newname;
			asprintf(&newname, "%s%s", current_dir, fl_filename_name(cut_copy_buffer[i]));
			if (edelib::file_exists(newname)) {
				int c = -1;
				if (!overwrite_all && !skip_all) {
					// here was choice_alert
					c = fl_choice(tsprintf(_("File already exists: %s. What to do?"), newname), _("&Overwrite"), _("Over&write all"), _("*&Skip"), _("Skip &all")); // * means default
				}
				if (c==1) overwrite_all=true;
				if (c==3) skip_all=true;
				if (c==2 || skip_all) {
					free(cut_copy_buffer[i]);
					continue; // go to next entry
				}
				// At this point c==0 (Overwrite) or overwrite_all == true
				unlink(newname);
			} 
			rename(cut_copy_buffer[i],newname);
			free(cut_copy_buffer[i]);
		}
		free(cut_copy_buffer);
		cut_copy_buffer=0;
		loaddir(current_dir); // Update display
		return;

	//
	// Real file moving / copying using recursive algorithm
	//

	} else {
		stop_now = false;

		// Create srcdir string
		char *srcdir = strdup(cut_copy_buffer[0]);
		char *p = strrchr(srcdir,'/');
		if (*(p+1) == '\0') { // slash is last - find one before
			*p = '\0';
			p = strrchr(srcdir,'/');
		}
		*(p+1) = '\0';

		if (strcmp(srcdir,current_dir)==0) {
			fl_alert(_("You cannot copy a file onto itself!"));
			return;
		}

		// Draw progress dialog
		// NOTE: sometimes when copying/moving just one file, the window
		// can get "stuck"
		// See: fltk STR 1255, http://www.fltk.org/str.php?L1255
		Fl_Window* progress_window = new Fl_Window(350,150);
		if (operation_is_copy)
			progress_window->label(_("Copying files"));
		else
			progress_window->label(_("Moving files"));
		progress_window->set_modal();
		progress_window->begin();
		Fl_Box* caption = new Fl_Box(20,20,310,25, _("Counting files in directories"));
		caption->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
		cut_copy_progress = new Fl_Progress(20,60,310,20);
		Fl_Button* stop_button = new Fl_Button(145,100,60,40, _("&Stop"));
		stop_button->callback(stop_copying_cb,caption);
		progress_window->end();
		progress_window->show();

		// How many items do we copy?
		int copy_items=0;
		for (; cut_copy_buffer[copy_items]; copy_items++);
		// Set ProgressBar range accordingly
		cut_copy_progress->minimum(0);
		cut_copy_progress->maximum(copy_items);
		cut_copy_progress->value(0);

		// Count files in directories
		int list_size = 0, list_capacity = 1000;
		char** files_list = (char**)malloc(sizeof(char**)*list_capacity);
		for (int i=0; i<copy_items; i++) {
			// We must ensure that cut_copy_buffer is deallocated
			// even if user clicked on Stop
			if (!stop_now) create_list(cut_copy_buffer[i], files_list, list_size, list_capacity);
			free(cut_copy_buffer[i]);
			cut_copy_progress->value(i+1);
			Fl::check(); // check to see if user pressed Stop
		}
		free (cut_copy_buffer);
		cut_copy_buffer=0;

		// Now copying those files
		if (!stop_now) {
			char label[150];
			char dest[PATH_MAX];
			cut_copy_progress->minimum(0);
			cut_copy_progress->maximum(list_size);
			cut_copy_progress->value(0);

			for (int i=0; i<list_size; i++) {
				// Prepare dest filename
				char *srcfile = files_list[i] + strlen(srcdir);
				snprintf (dest, PATH_MAX, "%s%s", current_dir, srcfile);

				snprintf(label, 150, _("Copying %d of %d files to %s"), i, list_size, current_dir);
				caption->label(label);
				caption->redraw();
				if (stop_now || !my_copy(files_list[i], dest))
					break;
				// Delete file after moving
				if (!operation_is_copy) unlink(files_list[i]);
				cut_copy_progress->value(cut_copy_progress->value()+1);
				Fl::check(); // check to see if user pressed Stop
			}
		}
		progress_window->hide();

		// Deallocate files_list[][]
		for (int i=0; i<list_size; i++) 
			free(files_list[i]);
		free(files_list);

		// Reload current dir
		loaddir(current_dir);
	}
}





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

void quit_cb(Fl_Widget*, void*) {exit(0);}

void cut_cb(Fl_Widget*, void*) { do_cut_copy(false); }
void copy_cb(Fl_Widget*, void*) { do_cut_copy(true); }
void paste_cb(Fl_Widget*, void*) { do_paste(); }


void showhidden_cb(Fl_Widget*, void*) { showhidden=!showhidden; loaddir(current_dir); }

void refresh_cb(Fl_Widget*, void*) { 
	loaddir(current_dir); 
	// TODO: reload directory tree as well-
}

void case_cb(Fl_Widget*, void*) { ignorecase=!ignorecase; loaddir(current_dir); }

void dirsfirst_cb(Fl_Widget*, void*) { dirsfirst=!dirsfirst; loaddir(current_dir); }

// dummy callbacks
void ow_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); } // make a list of openers
void delete_cb(Fl_Widget*, void*) { fprintf(stderr, "callback\n"); } // popup a warning
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
		{_("&Detailed list"),	FL_F+8,		listview_cb,	0,	FL_MENU_RADIO|FL_MENU_DIVIDER}, 
		{_("&Show hidden"),	0,		showhidden_cb,	0,	FL_MENU_TOGGLE},
		{_("Directory &tree"),	FL_F+9,		showtree_cb,	0,	FL_MENU_TOGGLE|FL_MENU_DIVIDER}, // coming soon
		{_("&Refresh"),		FL_F+5,		refresh_cb},
		{_("S&ort"),	0, 0, 0, FL_SUBMENU},
			{_("&Case sensitive"),	0,	case_cb,	0,	FL_MENU_TOGGLE},
			{_("D&irectories first"), 0,	dirsfirst_cb,	0,	FL_MENU_TOGGLE},
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
