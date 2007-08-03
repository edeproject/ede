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

// This file implements copy / move / delete operation with files
// NOT TO BE CONFUSED WITH edelib::File.h !!!
// Functions here usually call stuff from File.h, but also update
// efiler interface, display warnings to user etc.

#include "EDE_FileView.h"
#include <FL/Fl_Box.H>

// Execute cut or copy operation when List View is active
void do_cut_copy(bool m_copy);

// Execute paste operation - this will copy or move files based on chosen
// operation
void do_paste(const char*to = 0);

// Delete currently selected file(s) or directory(es)
void do_delete();

// Rename the file that has focus to given name
void do_rename(const char*);


extern FileDetailsView* view;
extern Fl_Window* win;
extern Fl_Box* statusbar;
extern char current_dir[];
extern void loaddir(const char*);
extern bool is_on_same_fs(const char*, const char*); // should be moved to another library
