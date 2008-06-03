/*
 * $Id$
 *
 * EDE Directory tree class
 * Part of edelib.
 * Copyright (c) 2005-2007 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */



#include "EDE_DirTree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>


// This library is hopelessly single-platform


#include <FL/Fl_Shared_Image.H>
#include <FL/fl_ask.H>

#include <edelib/Nls.h>
#include <edelib/IconTheme.h>
#include <edelib/StrUtil.h>


#define MAX_TREE_DEPTH 100
#define FL_PATH_MAX 256




// Fn that makes sure path ends with slash
// (ensure that path allocation is large enough to take one extra char)
void make_end_with_slash(char* path) {
	int i=strlen(path);
	if (path[i-1] != '/') {
		path[i]='/';
		path[i+1]='\0';
	}
}







// Filesystem data
// TODO: find better icons for Amiga, Macintosh etc.
// TODO: use USB icon for removable media, regardless of filesystem

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
	{"ext2",	"Linux legacy (%s)",		"drive-harddisk"},
	{"ext2fs",	"Linux legacy (%s)",		"drive-harddisk"},
	{"ext3",	"Linux disk (%s)",		"drive-harddisk"},
	{"reiserfs",	"Linux alternative (%s)",	"drive-harddisk"},
	{"ffs",		"BSD disk (%s)",		"drive-harddisk"},
	{"ufs",		"Unix disk (%s)",		"drive-harddisk"},
	{"dos",		"Windows disk (%s)",		"drive-harddisk"}, // I would prefer a Windows icon
	{"fat",		"Windows disk (%s)",		"drive-harddisk"},
	{"msdos",	"Windows disk (%s)",		"drive-harddisk"},
	{"ntfs",	"Windows disk (%s)",		"drive-harddisk"},
	{"pcfs",	"Windows disk (%s)",		"drive-harddisk"},
	{"umsdos",	"Windows disk (%s)",		"drive-harddisk"},
	{"vfat",	"Windows disk (%s)",		"drive-harddisk"},
	{"cd9660",	"CD-ROM (%s)",			"media-cdrom"},
	{"cdfs",	"CD-ROM (%s)",			"media-cdrom"},
	{"cdrfs",	"CD-ROM (%s)",			"media-cdrom"},
	{"hsfs",	"CD-ROM (%s)",			"media-cdrom"},
	{"iso9660",	"CD-ROM (%s)",			"media-cdrom"},
	{"isofs",	"CD-ROM (%s)",			"media-cdrom"},
	{"udf",		"DVD-ROM (%s)",			"media-dvd"},
	{"cifs",	"Shared directory (%s)",	"folder-remote"},
	{"nfs",		"Shared directory (%s)",	"folder-remote"},
	{"nfs4",	"Shared directory (%s)",	"folder-remote"},
	{"smbfs",	"Shared directory (%s)",	"folder-remote"},
	{"cramfs",	"Virtual (RAM) disk (%s)",	"memory"}, // no icon for ramdisk in fd.o specification or edeneu
	{"mfs",		"Virtual (RAM) disk (%s)",	"memory"},
	{"ramfs",	"Virtual (RAM) disk (%s)",	"memory"},
	{"romfs",	"Virtual (RAM) disk (%s)",	"memory"},
	{"union",	"Virtual (RAM) disk (%s)",	"memory"}, // not accurate, but good enough
	{"unionfs",	"Virtual (RAM) disk (%s)",	"memory"}, // not accurate, but good enough
	{"jfs",		"IBM AIX disk (%s)",		"drive-harddisk"},
	{"xfs",		"SGI IRIX disk (%s)",		"drive-harddisk"},


// Other filesystems
	{"coherent",	"Unix disk (%s)",		"drive-harddisk"},
	{"sysv",	"Unix disk (%s)",		"drive-harddisk"},
	{"xenix",	"Unix disk (%s)",		"drive-harddisk"},
	{"adfs",	"RiscOS disk (%s)",		"drive-harddisk"},
	{"filecore",	"RiscOS disk (%s)",		"drive-harddisk"},
	{"ados",	"Amiga disk (%s)",		"drive-harddisk"},
	{"affs",	"Amiga disk (%s)",		"drive-harddisk"},
	{"afs",		"AFS shared directory (%s)",	"folder-remote"},
	{"befs",	"BeOS disk (%s)",		"drive-harddisk"},
	{"bfs",		"UnixWare disk (%s)",		"drive-harddisk"},
	{"efs",		"SGI IRIX disk (%s)",		"drive-harddisk"},
	{"ext",		"Old Linux disk (%s)",		"drive-harddisk"},
	{"hfs",		"Macintosh disk (%s)",		"drive-harddisk"}, // Also used for HP-UX filesystem!!
	{"hpfs",	"OS/2 disk (%s)",		"drive-harddisk"},
//	{"lfs",		"BSD LFS disk (%s)",		"drive-harddisk"}, // This is said to not work
	{"minix",	"Minix disk (%s)",		"drive-harddisk"},
	{"ncpfs",	"NetWare shared directory (%s)",	"folder-remote"},
	{"nwfs",	"NetWare shared directory (%s)",	"folder-remote"},
	{"qns",		"QNX disk (%s)",		"drive-harddisk"},
	{"supermount",	"Removable disk (%s)", 		"media-cdrom"}, // ???
	{"xiafs",	"Old Linux disk (%s)",		"drive-harddisk"},

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
// Initializator - loads level 1 of the directory tree
// number of items is size()
//

void DirTree::init()
{

	int num_files=0;
	char buffer[PATH_MAX];	// buffer for labels

	clear();

	// Top level icon
	add(_("System"));
	set_icon(1, Fl_Shared_Image::get(edelib::IconTheme::get("computer",edelib::ICON_SIZE_TINY).c_str()));
	// TODO: use OS icons: Tux for Linux, devil for FreeBSD...
	data(1, strdup("about:sysinfo"));

	// Home icon
	snprintf(buffer,PATH_MAX,_("%s's Home"), getenv("USER"));
	add(strdup(buffer));
	set_icon(2, Fl_Shared_Image::get(edelib::IconTheme::get("user-home",edelib::ICON_SIZE_TINY).c_str()));
	strncpy(buffer,getenv("HOME"),PATH_MAX);
	make_end_with_slash(buffer);
	data(2, strdup(buffer));
	indent(2,1);

	// Root icon
	add(_("Whole disk"));
	edelib::String root_icon = edelib::IconTheme::get("folder_red",edelib::ICON_SIZE_TINY);
	if (root_icon=="") // sigh...
		root_icon = edelib::IconTheme::get("folder",edelib::ICON_SIZE_TINY);
	set_icon(3, Fl_Shared_Image::get(root_icon.c_str()));
	data(3,strdup("/"));
	indent(3,1);

	// Read mtab/fstab/whatever
	num_files=3;
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
				add(strdup(buffer));
				set_icon(num_files+1, Fl_Shared_Image::get( edelib::IconTheme::get(filesystems[i].icon,edelib::ICON_SIZE_TINY).c_str()));
				data(num_files+1, strdup(mountpoint));
				indent(num_files+1, 1);
				num_files++;
			}
		}
	}
	fclose(mtab);
}



// FIXME: this is copied from efiler.cpp
// modification of versionsort which ignores case
int mmyversionsort(const void *a, const void *b) {
	struct dirent** ka = (struct dirent**)a;
	struct dirent** kb = (struct dirent**)b;
	char* ma = strdup((*ka)->d_name);
	char* mb = strdup((*kb)->d_name);
	edelib::str_tolower((unsigned char*)ma); edelib::str_tolower((unsigned char*)mb);
	int k = strverscmp(ma,mb);
	free(ma); free(mb);
	return k;
}


// Function that scans subdirectories of given entry and adds them to browser
// Returns false if entry has no subdirectories or it was already scanned
bool DirTree::subdir_scan(int line) {
	if (line==0) return false; // we don't allow scanning of first item
	if (indent(line+1) > indent(line)) return false; // apparently it's already scanned

	int size=0;
	dirent** files;
	struct stat buf;
	char fullpath[FL_PATH_MAX];
	char* directory = (char*)data(line);
	if (ignore_case_)
		size = scandir(directory, &files, 0, mmyversionsort);
	else
		size = scandir(directory, &files, 0, versionsort);

	if (size<1) return false; // Permission denied - but view will warn the user


	int pos=line+1;
	for(int i=0; i<size; i++) {
		char *n = files[i]->d_name; //shortcut
		if (i>0) free(files[i-1]); // see scandir(3)

		// don't show . (current directory)
		if (strcmp(n,".")==0 || strcmp(n,"..")==0) continue;

		// hide files with dot except .. (up directory)
		if (!show_hidden_ && (n[0] == '.') && (strcmp(n,"..")!=0)) continue;

		snprintf (fullpath,FL_PATH_MAX-1,"%s%s/",directory,n); // all dirs should end with /
		if (stat(fullpath,&buf)) continue; // happens when user has traverse but not read privilege
		if (!S_ISDIR(buf.st_mode)) continue; // not a directory

		insert(pos, strdup(n), strdup(fullpath));
		set_icon(pos, Fl_Shared_Image::get(edelib::IconTheme::get("folder", edelib::ICON_SIZE_TINY).c_str()));
		indent(pos++, indent(line)+1);
	}
	free(files[size-1]); free(files); // see scandir(3)

	return (pos>line+1);
}


// Recursively find tree entry whose user_data() (system path)
// contains the largest portion of given path
// Best match is focused, return value tells if perfect match was found
bool DirTree::set_current(const char* path) {
	int k=find_best_match(path,1);
	if (k==-1) { 
		fprintf(stderr, "This shouldn't happen! %s\n", path); 
		return false; 
	}
	expand(k);
	subdir_scan(k); // expand currently selected directory
	select(k,1);
	middleline(k);
	return (strcmp(path,(char*)data(k))==0);
}

// Find browser entry whose full path (data) is the closest match
// to the given path
// If parent is given, only entries below it will be scanned
int DirTree::find_best_match(const char* path, int parent) {
	uint bestlen=0;
	int bestindex=-1;
	for (int i=parent; i<=size(); i++) {
		if ((i!=parent) && (indent(i)<=indent(parent))) break;
		char* d = (char*)data(i);
		uint len = strlen(d);
		if ((len>bestlen) && (strncmp((char*)path, d, len)==0)) {
			bestlen=len;
			bestindex=i;
		}
	}

	if (strlen(path)!=bestlen && subdir_scan(bestindex))
		return find_best_match(path, bestindex);
	else
		return bestindex;
}



//
// End of "$Id: FileBrowser.cxx 5071 2006-05-02 21:57:08Z fabien $".
//
