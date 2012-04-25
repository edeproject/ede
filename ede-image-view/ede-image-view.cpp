/*
 * $Id$
 *
 * ede-image-view, EDE image viewer
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>
#include <FL/x.H>
#include <edelib/Ede.h>

#define DEBUG 1

// Supported image types
const char* supported[] = {"bm","bmp","gif","jpg","pbm","pgm","png","ppm","xbm","xpm",0};

// Global variables used everywhere
char filename[FL_PATH_MAX], directory[FL_PATH_MAX];
Fl_Double_Window* w;
float zoomfactor;
bool autozoom=false;
Fl_Shared_Image *im;

class ScrolledImage;
ScrolledImage* s;

int screen_x, screen_y, screen_w, screen_h;

// Directory list cache used in prevnext()
dirent **files;
int nfiles;


// Forward declaration of funcs for use from callbacks
void nextpic();
void prevpic();
void loadimage();
void newdir();



// Callbacks for main menu
void next_cb(Fl_Widget*,void*) { nextpic(); }
void prev_cb(Fl_Widget*,void*) { prevpic(); }
void open_cb(Fl_Widget*,void*) {
	// construct filename filter
	char filter[FL_PATH_MAX];
	snprintf(filter, FL_PATH_MAX, "%s(*.{%s", _("Image files"), supported[0]); // splitted for easier localization
	for (int i=1; supported[i]; i++)
		snprintf(filter, FL_PATH_MAX, "%s,%s", filter, supported[i]);
	snprintf(filter, FL_PATH_MAX, "%s})", filter);

	const char* f = fl_file_chooser(_("Choose image or directory"),filter,directory);
	if (!f) return;

	strncpy(filename,f,FL_PATH_MAX);
	newdir();
	loadimage();
}

void fullscreen_cb(Fl_Widget* b,void*) {
	static bool isfull=false;
	static int X,Y,W,H;
	if (isfull) {
		w->fullscreen_off(X,Y,W,H);
		isfull=false;
	} else {
		X=w->x(); Y=w->y(); W=w->w(); H=w->h();
		w->fullscreen();
		isfull=true;
	}
} 

void zoomin_cb(Fl_Widget* b,void*) { 
	if (zoomfactor>=1) zoomfactor += 0.2; else zoomfactor += zoomfactor/5; 
	autozoom=false; 
	loadimage(); 

	if((im->w() > w->w() || im->h() > w->h()) && (w->w() < screen_w && w->h() < screen_h))
		w->size(im->w(), im->h());
}

void zoomout_cb(Fl_Widget* b,void*) { 
	if (zoomfactor>=1) zoomfactor -= 0.2; else zoomfactor -= zoomfactor/5; 
	autozoom=false; 
	loadimage(); 

	// some minimal size
	if(w->w() > 10 && w->h() > 10)
		w->size(im->w(), im->h());
}
void zoomrestore_cb(Fl_Widget* b,void*) { zoomfactor = 1; autozoom=false; loadimage(); }
void zoomauto_cb(Fl_Widget *b,void*) { autozoom = !autozoom; loadimage(); } 
void exit_cb(Fl_Widget* b,void*) { w->hide(); }

// Main popup menu
Fl_Menu_Item mainmenu[] = {
	{_("&Open"),		FL_CTRL+'o',open_cb, 0, FL_MENU_DIVIDER},
	{_("&Previous"),	FL_Page_Up,	  prev_cb},
	{_("&Next"),		FL_Page_Down, next_cb,	0, 	FL_MENU_DIVIDER},
	{_("&Zoom in"),		'+',    	 zoomin_cb},
	{_("Zoom &out"),	'-',    	 zoomout_cb},
	{_("Zoom &auto"),	FL_CTRL+'a', zoomauto_cb},
	{_("&Restore"),		'/',    	 zoomrestore_cb,	0,	FL_MENU_DIVIDER},
	{_("&Fullscreen"),	FL_F+11,    fullscreen_cb,	0,	FL_MENU_DIVIDER},
	{_("&Exit"),		FL_Escape,  exit_cb},
	{0}
};

class ScrolledImage : public Fl_Scroll {
private:
	Fl_Box* b;
	Fl_Menu_Button* mb;

public:
	ScrolledImage(int x, int y, int w, int h, const char*l = 0)
	: Fl_Scroll(x,y,w,h,l) {
		align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
		begin();
	
		b = new Fl_Box(w/2,h/2,0,0);

		mb = new Fl_Menu_Button (0,0,0,0);
		mb->type(Fl_Menu_Button::POPUP3);
		mb->menu(0);
		mb->box(FL_NO_BOX);
	
		end();
		redraw();
	}

	void image(Fl_Image* a) { b->image(a); }
	void image(Fl_Image& a) { b->image(a); }
	Fl_Image* image() { return b->image(); }
	void label(const char* a) { 
		if (a) resizebox(0,0); // center label
		b->label(a); 
	}

	// Resize the box containing image
	void resizebox(int W, int H) {
		int X=0,Y=0;
		int aw = w()-scrollbar.w(); int ah=h()-hscrollbar.h();
		if(aw>W) X=(aw-W)/2;
		if(ah>H) Y=(ah-H)/2;
		b->resize(X,Y,W,H);
	}
	
	void resize(int x,int y,int w,int h) {
		Fl_Scroll::resize(x,y,w,h);
		resizebox(b->w(),b->h()); // refresh image position
		redraw();
	}
	
	virtual int handle(int event) {
		if (event == FL_PUSH) {
			if(Fl::event_button()==3 && mb->menu()!=0) {
				mb->popup();
				return 1;
			}
		}
		// sometimes PgUp and PgDown aren't handled
		else if (event == FL_SHORTCUT && mb->menu()!=0) {
			int key = Fl::event_key();
			if (key==FL_Page_Up || key==FL_Page_Down)
				return mb->handle(event);
		}
		return Fl_Scroll::handle(event);
	}

	void setmenu(Fl_Menu_Item* menu) {
		if (menu!=0) {
			mb->menu(menu);
		}
	}
};


// Directory changed, get new directory from filename
void newdir() {
	int p=0;
	for (uint i=0; i<strlen(filename); i++) 
		if (filename[i] == '/') p=i;
	// There must be at least one '/'
	strncpy(directory, filename, p);
	directory[p]='\0';
	// make prevnext() reread directory
	nfiles=0;
}


// Load the image given in char[] filename
void loadimage() {
	char tmp[FL_PATH_MAX]; // the string buffer

	// Load image
	if (im) { im->release(); im=0; }
	im = Fl_Shared_Image::get(filename); // image type is autodetected now

	if (!im) {
		if (DEBUG) fprintf(stderr, "Fl_Shared_Image::get() failed!\n");
		s->image(0);
		snprintf(tmp, FL_PATH_MAX, _("Can't load image %s\n"),filename);
		s->copy_label(tmp);
		s->redraw();
		return;
	}

	// Measure image
	int realw=im->w(), realh=im->h();
	int scaledw,scaledh;

	if (autozoom) {
		// Adjust zoom factor so picture fits inside window
		// When user switches to manual zooming, this factor will remain
		float fw=(float)s->w()/realw; float fh=(float)s->h()/realh;
		if (fw < fh) zoomfactor=fw; else zoomfactor=fh;
	}

	// Resample image to new size
	scaledw=int(realw*zoomfactor);
	scaledh=int(realh*zoomfactor);

	if (zoomfactor!=1) {
		Fl_Image *temp = im->copy(scaledw,scaledh);
		im = (Fl_Shared_Image*) temp;
	}

	// Set image
	s->resizebox(scaledw,scaledh); 
	s->image(im);

	s->label(0); // clear any previous labels
	s->redraw();

	// set window title
	if (zoomfactor==1)
		snprintf(tmp,FL_PATH_MAX, "%s (%dx%d) - %s", fl_filename_name(filename),realw,realh, _("View picture")); // splitted for easier localization
	else
		snprintf(tmp,FL_PATH_MAX, "%s (%dx%d) - %s %1.1fx - %s", fl_filename_name(filename),realw,realh,_("zoom"),zoomfactor, _("View picture"));
	w->copy_label(tmp);
}


// Get next/previous picture file in directory
// (universal func. to be called from nextpic() and prevpic() )
void prevnext(int direction) {
	char tmp[FL_PATH_MAX]; // the string buffer

	if (nfiles == 0) { // read directory
		nfiles = fl_filename_list(directory,&files);
	}

	// Select next picture after current
	bool found=false;
	if (filename[0]) {
		const char* justname = fl_filename_name(filename);

		// this basically means: if direction is 1 go from first to last, else from last to first
		for (int i=(direction?0:nfiles-1); (direction?i<nfiles:i>=0); i+=(direction?1:-1)) {
			if (strncmp(justname,files[i]->d_name,FL_PATH_MAX) == 0) {
				found=true;
				continue; // skip to next file
			}
			if (found) {
				for (int j=0; supported[j]; j++) {
					snprintf(tmp,FL_PATH_MAX,"*.%s",supported[j]);
					if (fl_filename_match(files[i]->d_name,tmp)) {
						snprintf(filename,FL_PATH_MAX,"%s/%s",directory,files[i]->d_name);
						loadimage();
						return;
					}
				}
			}
		}
	}

	if (found) { //this means that the current picture is the last/first in directory
		if (im) { im->release(); im=0; }
		s->image(0);
		filename[0]=0;

		if (direction)
			s->label(_("This was the last picture.\nPress 'Next' again for first one."));
		else
			s->label(_("This was the first picture.\nPress 'Previous' again for last one."));
		s->redraw();
		return;

	} else {
		// Just give first (or last) picture in directory
		for (int i=(direction?0:nfiles-1); (direction?i<nfiles:i>=0); i+=(direction?1:-1)) {
			for (int j=0; supported[j]; j++) {
				snprintf(tmp,FL_PATH_MAX,"*.%s",supported[j]);
				if (fl_filename_match(files[i]->d_name,tmp)) {
					snprintf(filename,FL_PATH_MAX,"%s/%s",directory,files[i]->d_name);
					loadimage();
					return;
				}
			}
		}
	}

	// Nothing found...
	if (DEBUG) fprintf(stderr, "Nextpic() - nothing found\n");

	if (im) { im->release(); im=0; }
	s->image(0);
	filename[0]=0;
	snprintf(tmp,FL_PATH_MAX, _("No pictures in directory %s"),directory);
	s->copy_label(tmp);
	s->redraw();

	// Window title
	snprintf(tmp,FL_PATH_MAX, _("View picture - nothing found in %s"),directory);
	w->copy_label(tmp);
}

void nextpic() { prevnext(1); }
void prevpic() { prevnext(0); }

int main (int argc, char **argv) {
	// Parse command line - this must come first
	int unknown=0;
	Fl::args(argc,argv,unknown);
	filename[0]='\0'; directory[0]='\0';
	if (unknown==argc)
		snprintf(directory, FL_PATH_MAX, "%s", getenv("HOME"));
	else {
		if (strcmp(argv[unknown],"--help")==0) {
			printf(_("ede-image-view - EDE Image Viewer\nPart of Equinox Desktop Environment (EDE).\nCopyright (c) 2000-2007 EDE Authors.\n\nThis program is licenced under terms of the\nGNU General Public Licence version 2 or newer.\nSee COPYING for details.\n\n"));
			printf(_("Usage: ede-image-view [OPTIONS] [IMAGE_FILE]\n\n"));
			printf(_("Available options:\n%s\n"),Fl::help);
			return 1;
		}

		if (fl_filename_isdir(argv[unknown])) { // Param is directory
			snprintf(directory, FL_PATH_MAX, "%s", argv[unknown]);
			if (directory[0] == '~' && directory[1] == '/') // expand home dir
				snprintf (directory, FL_PATH_MAX, "%s%s", getenv("HOME"), argv[unknown]+1);
			else if (directory[0] != '/') // relative path
				snprintf (directory, FL_PATH_MAX, "%s/%s", getenv("PWD"), argv[unknown]);

		} else {
			snprintf (filename, FL_PATH_MAX, "%s", argv[unknown]);
			if (filename[0] == '~' && filename[1] == '/') // expand home dir
				snprintf (filename, FL_PATH_MAX, "%s%s", getenv("HOME"), argv[unknown]+1);
			else if (filename[0] != '/') // relative filename
				snprintf (filename, FL_PATH_MAX, "%s/%s", getenv("PWD"), argv[unknown]);

			newdir(); // rebuild char[] directory from filename
		}
	}

	EDE_APPLICATION("ede-image-view");

	fl_open_display();
	fl_register_images();
	zoomfactor=1; im=0; // defaults

	Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h);
	
	// Main window

	w = new Fl_Double_Window(400, 200, _("View picture"));
	s = new ScrolledImage(0,0,400,200);
	s->color(33);
	s->labelcolor(FL_WHITE);
	s->setmenu(mainmenu);
	w->resizable(s);
	w->end();
	w->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);


	// Check that file exists and open
	struct stat mstat;
	char tmp[FL_PATH_MAX];
	if (!filename[0]) { // Load directory
		if (stat(directory, &mstat)!=0) {
			snprintf(tmp,FL_PATH_MAX, _("Directory not found:\n%s"),directory);
			s->label(tmp);
		} else
			nextpic();
	} else {
		if (stat(filename, &mstat)!=0) {
			snprintf(tmp,FL_PATH_MAX, _("File not found:\n%s"),filename);
			s->label(tmp);
		} else
			loadimage();
	}

	// Resize window to image size or screen
	int W,H;
	if (im) { // skip if file not found
		if (im->w()>Fl::w()) W=Fl::w(); else W=im->w();
		if (im->h()>Fl::h()) H=Fl::h(); else H=im->h();
		w->resize(0,0,W,H);
	}
	// Window manager should make sure that window is fully visible

	w->show(argc,argv);
	return Fl::run();
}

