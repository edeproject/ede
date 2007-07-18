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
#include <fltk/ProgressBar.h>

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




/*-----------------------------------------------------------------
	Icon View implementation

NOTE: This will eventually be moved into a separate class. We know
there are ugly/unfinished stuff here, but please be patient.
-------------------------------------------------------------------*/


// Prototype (loaddir is called from button_press and vice versa
void loaddir(const char* path);

// Callback for icons in icon view
void button_press(Widget* w, void*) {
	if (event_clicks() || event_key() == ReturnKey) {
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


// This function populates the icon view
// For convinience, it is now also called from other parts of code
// and also calls FileBrowser->load(path) which does the same 
// thing for listview if neccessary

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

	// Clean up window
	sgroup->remove_all();
	sgroup->begin();

	// List all files in directory
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

	// Init mimetypes
	MimeType *m = new MimeType;

	// Detect icon mimetypes etc.
	for (int i=0; i<icon_num; i++) {
		// ignored files
		if (!icon_array[i]) continue;
		fltk::check(); // update interface

		// get mime data
		char fullpath[PATH_MAX];
		snprintf (fullpath,PATH_MAX-1,"%s%s",current_dir,files[i]->d_name);
		m->set(fullpath);

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
	}
	// MimeType destructor
	delete m;

	sgroup->redraw();
	semaphore=false;
}


/*-----------------------------------------------------------------
	Directory tree

	(only callback, since most of the real work is done in class)
-------------------------------------------------------------------*/

void dirtree_cb(Widget* w, void*) {
	if (!event_clicks() && event_key() != ReturnKey) return;
	char *d = (char*) dirtree->item()->user_data();
	if (d && strlen(d)>0) loaddir(d);
}


/*-----------------------------------------------------------------
	List view

	(only callback, since most of the real work is done in class)
-------------------------------------------------------------------*/

void fbrowser_cb(Widget* w, void*) {
	// Take only proper callbacks
	if (!event_clicks() && event_key() != ReturnKey) return;

	// Construct filename
	const char *c = fbrowser->system_path(fbrowser->value());
	char filename[PATH_MAX];
	if (strncmp(c+strlen(c)-3,"../",3)==0) {
		// User clicked on "..", we go up
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



/*-----------------------------------------------------------------
	File moving and copying operations
-------------------------------------------------------------------*/

ProgressBar* cut_copy_progress;
bool stop_now;
bool overwrite_all, skip_all;

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

// (this one will become obsolete when IconBrowser class gets the same API
// as FileBrowser)

void do_cut_copy_sgroup(bool m_copy) {

	// Group doesn't support type(MULTI) so only one item can be selected

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


// Helper functions for paste:

// Tests if two files are on the same filesystem
bool is_on_same_fs(const char* file1, const char* file2) {
	FILE	*mtab;		// /etc/mtab or /etc/mnttab file
	static char filesystems[50][PATH_MAX];
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

		if (fs_number == 0) return false; // some kind of error
	}

	// Find filesystem for file1 (largest mount point match)
	char *max;
	int maxlen = 0;
	for (int i=0; i<fs_number; i++) {
		int mylen = strlen(filesystems[i]);
		if ((strncmp(file1,filesystems[i],mylen)==0) && (mylen>maxlen)) {
			maxlen=mylen;
			max = filesystems[i];
		}
	}
	if (maxlen == 0) return false; // some kind of error

	// See if file2 matches the same filesystem
	return (strncmp(file2,max,maxlen)==0);
}


// Copy single file. Returns true if operation should continue
bool my_copy(const char* src, const char* dest) {
	FILE *fold, *fnew;
	int c;

	if (strcmp(src,dest)==0)
		// this shouldn't happen
		return true;

	if (filename_exist(dest)) {
		// if both src and dest are directories, do nothing
		if (filename_isdir(src) && filename_isdir(dest))
			return true;

		int c = -1;
		if (!overwrite_all && !skip_all) {
			c = choice_alert(tsprintf(_("File already exists: %s. What to do?"), dest), _("&Overwrite"), _("Over&write all"), _("*&Skip"), _("Skip &all"), 0); // asterisk (*) means default
		}
		if (c==1) overwrite_all=true;
		if (c==3) skip_all=true;
		if (c==2 || skip_all) {
			return true;
		}
		// At this point either c==0 (overwrite) or overwrite_all == true

		// copy directory over file
		if (filename_isdir(src))
			unlink(dest);

		// copy file over directory
		// TODO: we will just skip this case, but ideally there should be
		// another warning
		if (filename_isdir(dest))
			return true;
	}

	if (filename_isdir(src)) {
		if (mkdir (dest, umask(0))==0)
			return true; // success
		int q = choice_alert(tsprintf(_("Cannot create directory %s"),dest), _("*&Continue"), _("&Stop"), 0);
		if (q == 0) return true; else return false;
	}

	if ( ( fold = fopen( src, "rb" ) ) == NULL ) {
		int q = choice_alert(tsprintf(_("Cannot read file %s"),src), _("*&Continue"), _("&Stop"), 0);
		if (q == 0) return true; else return false;
	}

	if ( ( fnew = fopen( dest, "wb" ) ) == NULL  )
	{
		fclose ( fold );
		int q = choice_alert(tsprintf(_("Cannot create file %s"),dest), _("*&Continue"), _("&Stop"), 0);
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

	if (filename_isdir(src)) {
		char new_src[PATH_MAX];
		dirent	**files;
		// FIXME: use same sort as currently used
		// FIXME: detect errors on accessing folder
		int num_files = fltk::filename_list(src, &files, casenumericsort);
		for (int i=0; i<num_files; i++) {
			if (strcmp(files[i]->d_name,"./")==0 || strcmp(files[i]->d_name,"../")==0) continue;
			snprintf(new_src, PATH_MAX, "%s%s", src, files[i]->d_name);
			fltk::check(); // update gui
			if (stop_now || !create_list(new_src, list, list_size, list_capacity))
				return false;
		}
	}
	return true;
}


// Callback for Stop button on progress window
void stop_copying_cb(Widget*,void* v) {
	stop_now=true;
	// Let's inform user that we're stopping...
	InvisibleBox* caption = (InvisibleBox*)v;
	caption->label(_("Stopping..."));
	caption->redraw();
	caption->parent()->redraw();
}

// Execute paste operation - this will copy or move files based on last 
// operation
void do_paste() {

	if (!cut_copy_buffer || !cut_copy_buffer[0]) return;

	overwrite_all=false; skip_all=false;

	// Moving files on same filesystem is trivial
	// We don't even need a progress bar
	if (!operation_is_copy && is_on_same_fs(current_dir, cut_copy_buffer[0])) {
		for (int i=0; cut_copy_buffer[i]; i++) {
			char *newname;
			asprintf(&newname, "%s%s", current_dir, filename_name(cut_copy_buffer[i]));
			if (filename_exist(newname)) {
				int c = -1;
				if (!overwrite_all && !skip_all) {
					c = choice_alert(tsprintf(_("File already exists: %s. What to do?"), newname), _("&Overwrite"), _("Over&write all"), _("*&Skip"), _("Skip &all")); // * means default
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
			alert(_("You cannot copy a file onto itself!"));
			return;
		}

		// Draw progress dialog
		// NOTE: sometimes when copying/moving just one file, the window
		// can get "stuck"
		// See: fltk STR 1255, http://www.fltk.org/str.php?L1255
		Window* progress_window = new Window(350,150);
		if (operation_is_copy)
			progress_window->label(_("Copying files"));
		else
			progress_window->label(_("Moving files"));
		progress_window->set_modal();
		progress_window->begin();
		InvisibleBox* caption = new InvisibleBox(20,20,310,25, _("Counting files in directories"));
		caption->align(ALIGN_LEFT|ALIGN_INSIDE);
		cut_copy_progress = new ProgressBar(20,60,310,20);
		Button* stop_button = new Button(145,100,60,40, _("&Stop"));
		stop_button->callback(stop_copying_cb,caption);
		progress_window->end();
		progress_window->show();

		// How many items do we copy?
		int copy_items=0;
		for (; cut_copy_buffer[copy_items]; copy_items++);
		// Set ProgressBar range accordingly
		cut_copy_progress->range(0,copy_items,1);
		cut_copy_progress->position(0);

		// Count files in directories
		int list_size = 0, list_capacity = 1000;
		char** files_list = (char**)malloc(sizeof(char**)*list_capacity);
		for (int i=0; i<copy_items; i++) {
			// We must ensure that cut_copy_buffer is deallocated
			// even if user clicked on Stop
			if (!stop_now) create_list(cut_copy_buffer[i], files_list, list_size, list_capacity);
			free(cut_copy_buffer[i]);
			cut_copy_progress->position(i+1);
			fltk::check(); // check to see if user pressed Stop
		}
		free (cut_copy_buffer);
		cut_copy_buffer=0;

		// Now copying those files
		if (!stop_now) {
			char label[150];
			char dest[PATH_MAX];
			cut_copy_progress->range(0, list_size, 1);
			cut_copy_progress->position(0);

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
				cut_copy_progress->position(cut_copy_progress->position()+1);
				fltk::check(); // check to see if user pressed Stop
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

// File menu callbacks

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


// Edit menu callbacks

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
	do_paste();
}


// View menu

void switchview() {
	if (sgroup->visible()) {
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

void iconsview_cb(Widget*,void*) {
	// When user presses F8 we switch view regardles of which is visible
	// However, when menu option is chosen we only switch *to* iconview
	if (fbrowser->visible() || event_key() == F8Key) switchview();
}
void listview_cb(Widget*,void*) { if (sgroup->visible()) switchview(); }

void showhidden_cb(Widget* w, void*) {
	// Presently, fbrowser has show_hidden() method while icon view
	// respects value of showhidden global variable

	Item *i = (Item*)w;
	if (showhidden) {
		showhidden=false;
		i->clear();
	} else {
		showhidden=true;
		i->set();
	}
	if (fbrowser->visible()) fbrowser->show_hidden(showhidden);
	// Reload current view
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
	// TODO: reload directory tree as well-
}
void case_cb(Widget*,void*) {
	fprintf(stderr,"Not implemented yet...\n");
}


// Help menu

void about_cb(Widget*, void*) { about_dialog("EFiler", "0.1", _("EDE File Manager"));}



/*-----------------------------------------------------------------
	GUI design
-------------------------------------------------------------------*/


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
				o->callback(copy_cb);
				o->shortcut(CTRL+'c');
			}
			{Item *o = new Item(_("&Paste"));
				o->callback(paste_cb);
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
