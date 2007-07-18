//
// "$Id$"
//
// FileBrowser routines.
//
// Copyright 1999-2006 by Michael Sweet.
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
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//
// Contents:
//
//   FileBrowser::full_height()     - Return the height of the list.
//   FileBrowser::item_height()     - Return the height of a list item.
//   FileBrowser::item_width()      - Return the width of a list item.
//   FileBrowser::item_draw()       - Draw a list item.
//   FileBrowser::FileBrowser() - Create a FileBrowser widget.
//   FileBrowser::load()            - Load a directory into the browser.
//   FileBrowser::filter()          - Set the filename filter.
//

//
// Include necessary header files...
//

#include "EDE_DirTree.h"

#include <fltk/Browser.h>
#include <fltk/Item.h>
#include <fltk/draw.h>
#include <fltk/Color.h>
#include <fltk/Flags.h>
#include <fltk/Font.h>
#include <fltk/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// This library is hopelessly single-platform


#include "../edelib2/Icon.h"
#include "../edelib2/Util.h"
#include "../edelib2/NLS.h"

#include <fltk/run.h>


#define MAX_TREE_DEPTH 100



using namespace fltk;

//
// 'FileBrowser::FileBrowser()' - Create a FileBrowser widget.
//



// Fn that makes sure path ends with slash
// (ensure that path allocation is large enough to take one extra char)
void make_end_with_slash(char* path) {
	int i=strlen(path);
	if (path[i-1] != '/') {
		path[i]='/';
		path[i+1]='\0';
	}
}






DirTree::DirTree(int        X,  // I - Upper-lefthand X coordinate
                        	 int        Y,  // I - Upper-lefthand Y coordinate
				 int        W,  // I - Width in pixels
				 int        H,  // I - Height in pixels
				 const char *l)	// I - Label text
 : Browser(X, Y, W, H, l) {
  // Initialize the filter pattern, current directory, and icon size...
  //pattern_   = "*";
  //directory_ = "";
  //icon_size_  = 12.0f;
  //filetype_  = FILES;
  show_hidden_ = false;

  //Symbol* fileSmall = edelib::Icon::get(DEFAULT_ICON,edelib::Icon::SMALL);
  //set_symbol(Browser::LEAF, fileSmall, fileSmall);
}



// Filesystem data
// TODO: find better icons for Amiga, Macintosh etc.

static struct {
	char id[15];
	char uiname[50];
	char icon[20];
} filesystems[] = {
// Filesystems that should be ignored
	{"auto",	"", ""}, // removables will be added another way
	{"autofs",	"", ""}, // removables will be added another way
	{"binfmt_misc",	"", ""},
	{"debugfs",	"", ""},
	{"devfs",	"", ""},
	{"devpts",	"", ""},
	{"fd",		"", ""},
	{"fdesc",	"", ""},
	{"kernfs",	"", ""},
	{"proc",	"", ""},
	{"procfs",	"", ""},
	{"swap",	"", ""},
	{"sysfs",	"", ""},
	{"tmpfs",	"", ""},
	{"usbfs",	"", ""},

// Common filesystems
	{"ext2",	"Linux disk (%s)",		"hdd_unmount"},
	{"ext2fs",	"Linux disk (%s)",		"hdd_unmount"},
	{"ext3",	"Linux disk (%s)",		"hdd_unmount"},
	{"reiserfs",	"Linux disk (%s)",		"hdd_unmount"},
	{"ffs",		"BSD disk (%s)",		"hdd_unmount"},
	{"ufs",		"Unix disk (%s)",		"hdd_unmount"},
	{"dos",		"Windows disk (%s)",		"win_mount"},
	{"fat",		"Windows disk (%s)",		"win_mount"},
	{"msdos",	"Windows disk (%s)",		"win_mount"},
	{"ntfs",	"Windows disk (%s)",		"win_mount"},
	{"pcfs",	"Windows disk (%s)",		"win_mount"},
	{"umsdos",	"Windows disk (%s)",		"win_mount"},
	{"vfat",	"Windows disk (%s)",		"win_mount"},
	{"cd9660",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"cdfs",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"cdrfs",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"hsfs",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"iso9660",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"isofs",	"CD-ROM (%s)",			"cdrom_unmount"},
	{"udf",		"DVD-ROM (%s)",			"dvd_unmount"},
	{"cifs",	"Shared directory (%s)",	"server"},
	{"nfs",		"Shared directory (%s)",	"server"},
	{"nfs4",	"Shared directory (%s)",	"server"},
	{"smbfs",	"Shared directory (%s)",	"server"},
	{"cramfs",	"Virtual (RAM) disk (%s)",	"memory"},
	{"mfs",		"Virtual (RAM) disk (%s)",	"memory"},
	{"ramfs",	"Virtual (RAM) disk (%s)",	"memory"},
	{"romfs",	"Virtual (RAM) disk (%s)",	"memory"},
	{"union",	"Virtual (RAM) disk (%s)",	"memory"}, // not accurate, but good enough
	{"unionfs",	"Virtual (RAM) disk (%s)",	"memory"}, // not accurate, but good enough
	{"jfs",		"IBM AIX disk (%s)",		"hdd_unmount"},
	{"xfs",		"SGI IRIX disk (%s)",		"hdd_unmount"},


// Other filesystems
	{"coherent",	"Unix disk (%s)",		"hdd_unmount"},
	{"sysv",	"Unix disk (%s)",		"hdd_unmount"},
	{"xenix",	"Unix disk (%s)",		"hdd_unmount"},
	{"adfs",	"RiscOS disk (%s)",		"hdd_unmount"},
	{"filecore",	"RiscOS disk (%s)",		"hdd_unmount"},
	{"ados",	"Amiga disk (%s)",		"hdd_unmount"},
	{"affs",	"Amiga disk (%s)",		"hdd_unmount"},
	{"afs",		"AFS shared directory (%s)",	"server"},
	{"befs",	"BeOS disk (%s)",		"hdd_unmount"},
	{"bfs",		"UnixWare disk (%s)",		"hdd_unmount"},
	{"efs",		"SGI IRIX disk (%s)",		"hdd_unmount"},
	{"ext",		"Old Linux disk (%s)",		"hdd_unmount"},
	{"hfs",		"Macintosh disk (%s)",		"hdd_unmount"}, // Also used for HP-UX filesystem!!
	{"hpfs",	"OS/2 disk (%s)",		"hdd_unmount"},
//	{"lfs",		"BSD LFS disk (%s)",		"hdd_unmount"}, // This is said to not work
	{"minix",	"Minix disk (%s)",		"hdd_unmount"},
	{"ncpfs",	"NetWare shared directory (%s)",	"server"},
	{"nwfs",	"NetWare shared directory (%s)",	"server"},
	{"qns",		"QNX disk (%s)",		"hdd_unmount"},
	{"supermount",	"Removable disk (%s)", 		"cdrom_umnount"}, // ???
	{"xiafs",	"Old Linux disk (%s)",		"hdd_unmount"},

// Virtual filesystems (various views of existing files)
// How often are these used?
//	{"cachefs",	"Unix virtual disk ?? (%s)",	"memory"},
//	{"lofs",	"Solaris virtual disk ?? (%s)",	"memory"},
//	{"null",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"nullfs",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"overlay",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"portal",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"portalfs",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"umap",	"BSD virtual disk ?? (%s)",	"memory"},
//	{"umapfs",	"BSD virtual disk ?? (%s)",	"memory"},

	{"", "", ""}
};

//
// 'FileBrowser::load()' - Load basic directory list into browser
//

int						// O - Number of files loaded
DirTree::load()
{

	int num_files=0;
	char buffer[PATH_MAX];	// buffer for labels
	Group* o;		// generic pointer

	this->begin();

	// Top level icon
	Group* system = (Group*)add_group(_("System"), 0, 0, edelib::Icon::get("tux",edelib::Icon::TINY));

	// Home icon
	snprintf(buffer,PATH_MAX,_("%s's Home"), getenv("USER"));
	o = (Group*)add_group(strdup(buffer), system, 0, edelib::Icon::get("folder_home",edelib::Icon::TINY));
	strncpy(buffer,getenv("HOME"),PATH_MAX);
	// Home env. var. often doesn't end with slash
	make_end_with_slash(buffer);
	o->user_data(strdup(buffer));

	// Root icon
	o = (Group*)add_group(_("Whole disk"), system, 0, edelib::Icon::get("folder_red",edelib::Icon::TINY));
	o->user_data(strdup("/"));

	num_files=3;

	// Read mtab/fstab/whatever
	FILE	*mtab;		// /etc/mtab or /etc/mnttab file
	char	line[1024];	// Input line
	char	device[1024], mountpoint[1024], fs[1024];

	// mtab doesn't contain removable entries, but we will find another way
	// to add them (some day)

	mtab = fopen("/etc/mnttab", "r");	// Fairly standard
	if (mtab == NULL)
		mtab = fopen("/etc/mtab", "r");	// More standard
	if (mtab == NULL)
		mtab = fopen("/etc/fstab", "r");	// Otherwise fallback to full list
	if (mtab == NULL)
		mtab = fopen("/etc/vfstab", "r");	// Alternate full list file

	while (mtab!= NULL && fgets(line, sizeof(line), mtab) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (sscanf(line, "%s%s%s", device, mountpoint, fs) != 3)
			continue;

		make_end_with_slash(mountpoint);

		// Stuff that should be invisible cause we already show it
		if (strcmp(mountpoint,"/") == 0) continue; // root
		if (strcmp(mountpoint,getenv("HOME")) == 0) continue; // home dir

		// Device name without "/dev/"
		char *shortdev = strrchr(device,'/');
		if (shortdev==NULL) shortdev=device; else shortdev++;

		// Go through list of known filesystems
		for (int i=0; filesystems[i].id[0] != '\0'; i++) {
			if (strcmp(fs, filesystems[i].id) == 0) {
				if (filesystems[i].uiname[0] == '\0') break; // skipping this fs

				snprintf(buffer, PATH_MAX, filesystems[i].uiname, shortdev);
				o = (Group*)add_group(strdup(buffer), system, 0, edelib::Icon::get(filesystems[i].icon, edelib::Icon::TINY));
				o->user_data(strdup(mountpoint));
				num_files++;
			}
		}
	}

	this->end();
	system->set_flag(fltk::OPENED);

	this->redraw();

	return (num_files);
}

// Browser method for opening (expanding) a subtree
// We override this method so we could scan directory tree on demand
bool DirTree::set_item_opened(bool open) {
	// Eliminate bogus call
	if (!item()) return false;
	// This is what we do different from Browser:
//fprintf (stderr, "Children: %d\n",children(current_index(),current_level()+1));
	if (open && children(current_index(),current_level()+1)<=0) {
		dirent** files;
		char* directory = (char*)item()->user_data();
		char filename[PATH_MAX];

		// NOTE we default to casenumericsort!
		int num_files = filename_list(directory, &files, casenumericsort);
		Group* current = (Group*)item();

		for (int i=0; i<num_files; i++) {
			if (files[i]->d_name[0] == '.' && show_hidden_==false) continue;
			if (!strcmp(files[i]->d_name,"./") || !strcmp(files[i]->d_name,"../")) continue;

			snprintf(filename, PATH_MAX, "%s%s", directory, files[i]->d_name);
//fprintf(stderr, "Testing: %s\n",filename);
			if (filename_isdir(filename)) {
				// Strip slash from label
				char *n = strdup(files[i]->d_name);
				n[strlen(n)-1] = '\0';
				Group* o = (Group*)add_group(n, current, 0, edelib::Icon::get("folder", edelib::Icon::TINY));
				o->user_data(strdup(filename));
			}
		}
	}
	return Browser::set_item_opened(open);
}


// Recursively find tree entry whose user_data() (system path)
// contains the largest portion of given path
// Best match is focused, return value tells if perfect match was found
bool DirTree::set_current(const char* path) {
	int tree[MAX_TREE_DEPTH];

	// Copy of path where we add slash as needed
	char mypath[PATH_MAX];
	strncpy(mypath,path,PATH_MAX-2);
	make_end_with_slash(mypath);

	// There is always exactly one toplevel entry - "System"
	tree[0]=0;
	set_item(tree,1);
	bool t = find_best_match(mypath,tree,1);
	set_focus();
	// Expand current item
	set_item_opened(true);
	return t;
}

// Recursive function called by set_current()
bool DirTree::find_best_match(const char* path, int* tree, int level) {
	uint bestlen=0;
	int bestindex=-1;
	int n_children = children(tree,level);

	// Given entry has no children
	// let's forward this result to first caller
	if (n_children == -1) return false;

	// Look at all children of given entry and find the best match
	for (int i=0; i<n_children; i++) {
		tree[level]=i;
		Widget* w = child(tree,level);
		char* p = (char*)w->user_data();
		if (strlen(p)>strlen(path)) continue; // too specific
		if ((strncmp(path, p, strlen(p)) == 0) && (strlen(p)>bestlen)) {
			bestlen=strlen(p);
			bestindex=i;
		}
	}

	// No matches found
	if (bestindex==-1) return false;

	// Focus on best match
	tree[level]=bestindex;
	goto_index(tree,level);
//char* p = (char*)child(tree,level)->user_data();
//fprintf (stderr, "Found: %s\n", p);

	// Is this a perfect match?
	if (bestlen == strlen(path)) return true;

	// Can the best item be further expanded?
	set_item_opened(true);

	// Try its children (if none, it will return false)
	return find_best_match(path, tree, level+1);
}



//
// End of "$Id$".
//
