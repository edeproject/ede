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

// Functions for filesystems

#include "filesystem.h"

#include <string.h>
#include <stdio.h>
#include <sys/vfs.h> // used for statfs()

// for fltk compatibility:
#define PATH_MAX 256

char filesystems[50][PATH_MAX]; // must be global


// Read filesystems - adapted from fltk file chooser
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
// This can't use statfs cause there is no way to compare 
// two "struct statfs" values
bool is_on_same_fs(const char* file1, const char* file2) {
	const char* fs1 = find_fs_for(file1);
	const char* fs2 = find_fs_for(file2);
	// See if file2 matches the same filesystem
	return (strcmp(fs1,fs2)==0);
}


// Find filesystem total size and free space (in bytes)
// dir is any directory on the filesystem
bool fs_usage(const char* dir, double &total, double &free) {
	static struct statfs statfs_buffer;
	if (statfs(dir, &statfs_buffer)!=0) return false;

	total = double(statfs_buffer.f_bsize) * statfs_buffer.f_blocks;
	free = double(statfs_buffer.f_bsize) * statfs_buffer.f_bavail; // This is what df returns
	return true;
}
