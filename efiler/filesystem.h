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

// Get mount point of filesystem for given file
const char* find_fs_for(const char* file);

// Tests if two files are on the same filesystem
bool is_on_same_fs(const char* file1, const char* file2);

// Find filesystem total size and free space (in bytes)
// dir is any directory on the filesystem
bool fs_usage(const char* dir, double &total, double &free);
