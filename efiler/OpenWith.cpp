#include "OpenWith.h"

#include <edelib/IconTheme.h>
#include <edelib/Run.h>

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_File_Chooser.H>

#include <stdio.h>
#include <stdlib.h> // getenv()
#include <dirent.h>
#include <sys/stat.h>

#include "Util.h"

void openwith_cancel_cb(Fl_Widget*w, void*) {
	Fl_Window* win = (Fl_Window*)w->parent();
	win->hide();
}

void openwith_ok_cb(Fl_Widget*w, void*i) {
	Fl_Input* inpt = (Fl_Input*)i;
	OpenWith* win = (OpenWith*)w->parent();
fprintf(stderr, "Opening %s '%s'\n", inpt->value(), win->file());
	int k = edelib::run_program(tsprintf("%s '%s'", inpt->value(), win->file()), /*wait=*/false);
fprintf(stderr, "retval: %d\n", k);
	win->hide();
}

void openwith_browse_cb(Fl_Widget*w, void*i) {
	Fl_Input* inpt = (Fl_Input*)i;
	char *file = fl_file_chooser(_("Choose program"),0,0);
	inpt->value(file);
}

void program_input_cb(Fl_Widget*w, void*p) {
	edelib::list<edelib::String>* progs = (edelib::list<edelib::String>*)p;
	Fl_Input *inpt = (Fl_Input*)w;
	Fl_Window *win = (Fl_Window*)w->parent();

	if (Fl::event()==FL_KEYDOWN && Fl::event_key()!=FL_BackSpace && Fl::event_key()!=FL_Delete) {
		const char* loc = inpt->value(); // shortcut
		if (strlen(loc)<1 || loc[strlen(loc)-1]=='/') return;
		uint pos = inpt->position();
		if (pos!=strlen(loc)) return; // cursor in the middle
		int mark = inpt->mark();

		if (Fl::event_key()==FL_Enter) { 
			inpt->mark(pos);
			// Move focus to Ok button
			win->child(3)->take_focus();
			return;
		}

		edelib::list<edelib::String>::iterator it1, it2;
		it1 = progs->begin();
		it2 = progs->end();
		while (it1 != it2) {
			if (strncmp(loc, (*it1).c_str(), strlen(loc))==0) break;
			++it1;
		}
		if (it1==it2) return;

		inpt->replace(pos, mark, (*it1).c_str()+pos);
		inpt->position(pos);
		inpt->mark(strlen((*it1).c_str()));
	}
}


OpenWith::OpenWith()  : edelib::Window(410,110, _("Choose program")), _file(0) {
	set_modal();

	edelib::list<edelib::String>::iterator it1, it2;
	dirent **files;
	struct stat buf;
	char pn[FL_PATH_MAX];

	Fl_Group *gr;
	Fl_Box *img, *txt;
	Fl_Button *but1, *but2, *but3;
	Fl_Image* i;

	// Extract all parts of $PATH
	char* path = strdup(getenv("PATH")); // original would get destroyed
	char *pathpart = strtok(path, ":");
	while (pathpart) {
		// Get list of files in pathpart
		int size = scandir(pathpart, &files,0,alphasort);
		for (int i=0; i<size; i++) {
			// Attach filename to directory to get full path
			char *n = files[i]->d_name;
			snprintf (pn,FL_PATH_MAX-1,"%s/%s",pathpart,n);
			if (stat(pn,&buf)) continue;
			if (!S_ISDIR(buf.st_mode) && (buf.st_mode&S_IXOTH)) {
				// Not directory, executable
				edelib::String name(n);
				it1=programs.begin(); it2=programs.end();
				bool exists=false;
				while (it1 != it2) {
					if (*it1==name) { exists=true;break;}
					++it1;
				}
				if (!exists) programs.push_back(n);
			}
		}
		pathpart=strtok(NULL, ":");
	}
	free(path);

	programs.sort();

	// Window design
	begin();
	img = new Fl_Box(10, 10, 60, 55);

	txt = new Fl_Box(75, 10, w()-85, 25, _("Enter the name of application to open this file with:"));
	inpt = new Fl_Input(75, 40, w()-175, 25);

	txt->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_WRAP);

	but1 = new Fl_Return_Button(w()-200, 75, 90, 25, "&Ok");
	but2 = new Fl_Button(w()-100, 75, 90, 25, "&Cancel");
	but3 = new Fl_Button(w()-90, 40, 80, 25, "&Browse...");

	end();

	but1->callback(openwith_ok_cb,inpt);
	but2->callback(openwith_cancel_cb);
	but3->callback(openwith_browse_cb,inpt);

	inpt->when(FL_WHEN_ENTER_KEY_CHANGED);
	inpt->callback(program_input_cb, &programs);

	// Set icon
	if(!edelib::IconTheme::inited()) return;
	i = Fl_Shared_Image::get(edelib::IconTheme::get("dialog-question", edelib::ICON_SIZE_MEDIUM).c_str());
	if(!i) return;

	img->image(i);
}
