
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
void do_paste();

// Delete currently selected file(s) or directory(es)
void do_delete();

// Rename the file with focus to given name
void do_rename(const char*);


extern FileDetailsView* view;
extern Fl_Box* statusbar;
extern char current_dir[];
extern void loaddir(const char*);
extern bool is_on_same_fs(const char*, const char*); // should be moved to another library
