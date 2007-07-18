#include <Fl/Fl.h>
#include <Fl/Fl_Window.h>
#include <Fl/Fl_Button.h>
#include <Fl/Fl_Shared_Image.h>
#include <Fl/Fl_Scroll.h>
#include <Fl/Fl_Widget.h>
#include <Fl/Fl_File_Chooser.h>
#include <Fl/filename.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#define DEBUG 1



// Supported image types
const char* supported[] = {"bm","bmp","gif","jpg","pbm","pgm","png","ppm","xbm","xpm",0};



// Global variables used everywhere
char filename[FL_PATH_MAX], directory[FL_PATH_MAX];
Fl_Window* w;
float zoomfactor;
bool autozoom=false;
Fl_Shared_Image *im;

class ScrolledImage;
ScrolledImage* s;


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
	snprintf(filter, FL_PATH_MAX, "Image files(*.{%s", supported[0]);
	for (int i=1; supported[i]; i++)
		snprintf(filter, FL_PATH_MAX, "%s,%s", filter, supported[i]);
	snprintf(filter, FL_PATH_MAX, "%s})", filter);

	const char* f = fl_file_chooser("Choose image or directory",filter,directory);
	if (!f) return;

	strncpy(filename,f,FL_PATH_MAX);
	newdir();
	loadimage();
}

void manage_cb(Fl_Widget* b,void*) {} // call file manager
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
}
void zoomout_cb(Fl_Widget* b,void*) { 
	if (zoomfactor>=1) zoomfactor -= 0.2; else zoomfactor -= zoomfactor/5; 
	autozoom=false; 
	loadimage(); 
}
void zoomrestore_cb(Fl_Widget* b,void*) { zoomfactor = 1; autozoom=false; loadimage(); }
void zoomauto_cb(Fl_Widget *b,void*) { autozoom = !autozoom; loadimage(); } 
void about_cb(Fl_Widget* b,void*) {} // about window
void exit_cb(Fl_Widget* b,void*) { exit(0); } 



// Main popup menu
Fl_Menu_Item mainmenu[] = {
	{"&Open",	FL_CTRL+'o',	open_cb},
	{"&Manage",	0,      	manage_cb,	0, 	FL_MENU_DIVIDER},

	{"&Previous",	FL_Page_Up,	prev_cb},
	{"&Next",	FL_Page_Down,	next_cb,	0, 	FL_MENU_DIVIDER},

	{"&Zoom in",	'+',    	zoomin_cb},
	{"Zoom &out",	'-',    	zoomout_cb},
	{"Zoom &auto",	FL_CTRL+'a',	zoomauto_cb},
	{"&Restore",	'/',    	zoomrestore_cb,	0,	FL_MENU_DIVIDER},

	{"&Fullscreen",	FL_F+11,    	fullscreen_cb,	0,	FL_MENU_DIVIDER},

	{"A&bout",	0,      	about_cb},
	{"&Exit",	FL_Escape,    	exit_cb},
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

		mb = new Fl_Menu_Button (0,0,0,0,"");
		mb->type(Fl_Menu_Button::POPUP3);
		mb->menu(0);
	
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
	for (int i=0; i<strlen(filename); i++) 
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

	if (DEBUG) fprintf(stderr, "Loadimage() - file: %s\n",filename);

	// Load image
	if (im) { im->release(); im=0; }
	im = Fl_Shared_Image::get(filename); // image type is autodetected now

	if (!im) {
		if (DEBUG) fprintf(stderr, "Fl_Shared_Image::get() failed!\n");
		s->image(0);
		snprintf(tmp, FL_PATH_MAX, "Can't load image %s\n",filename);
		s->label(strdup(tmp));
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
	scaledw=realw*zoomfactor;
	scaledh=realh*zoomfactor;

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
		snprintf(tmp,FL_PATH_MAX,"%s (%dx%d) - View picture",fl_filename_name(filename),realw,realh);
	else
		snprintf(tmp,FL_PATH_MAX,"%s (%dx%d) - zoom %1.1fx - View picture",fl_filename_name(filename),realw,realh,zoomfactor);
	w->label(strdup(tmp));
}


// Get next/previous picture file in directory
// (universal func. to be called from nextpic() and prevpic() )
void prevnext(int direction) {
	char tmp[FL_PATH_MAX]; // the string buffer

	if (DEBUG) 
		fprintf(stderr, "Prevnext() - file: %s dir: %s direction: %d\n",filename,directory,direction);

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
			s->label("This was the last picture.\nPress 'Next' again for first one.");
		else
			s->label("This was the first picture.\nPress 'Previous' again for last one.");
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
	snprintf(tmp,FL_PATH_MAX,"No pictures in directory %s",directory);
	s->label(strdup(tmp));
	s->redraw();

	// Window title
	snprintf(tmp,FL_PATH_MAX,"View picture - nothing found in %s",directory);
	w->label(strdup(tmp));
}

void nextpic() { prevnext(1); }
void prevpic() { prevnext(0); }



int main (int argc, char **argv) {
	filename[0]='\0'; directory[0]='\0'; zoomfactor=1; im=0;
	fl_register_images();


	// Main window

	w = new Fl_Window(200, 200, "View picture");
	s = new ScrolledImage(0,0,200,200);
	s->color(33);
	s->labelcolor(FL_WHITE);
	s->setmenu(mainmenu);
	w->resizable(s);
	w->end();
	w->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);



	// Analyze command line

	if (argc==1) { // No params
		strncpy (directory, getenv("HOME"), FL_PATH_MAX);
		nextpic();

	} else if (fl_filename_isdir(argv[1])) { // Param is directory
		strncpy (directory, argv[1], FL_PATH_MAX);
		nextpic();
		argc--; argv++; // ignore this param and forward rest to fltk

	} else { // Param is file
		if (argv[1][0] == '~' && argv[1][1] == '/') // expand home dir
			snprintf (filename, FL_PATH_MAX, "%s/%s", getenv("HOME"), argv[1]+2);
		else if (argv[1][0] != '/') // relative filename
			snprintf (filename, FL_PATH_MAX, "%s/%s", getenv("PWD"), argv[1]);
		else // absolute filename
			strncpy (filename, argv[1], FL_PATH_MAX);

		struct stat last_stat;  // Does file exist?
		if (stat(argv[0], &last_stat)!=0) {
			char tmp[FL_PATH_MAX];
			snprintf(tmp,FL_PATH_MAX,"File not found - %s",filename);
			s->label(tmp);
		} else 
			loadimage();

		newdir(); // rebuild char[] directory
		argc--; argv++; // ignore this param and forward rest to fltk
	}

	// Resize window to image size or screen
	int W,H; 
	if (im->w()>Fl::w()) W=Fl::w(); else W=im->w();
	if (im->h()>Fl::h()) H=Fl::h(); else H=im->h();
	w->resize(0,0,W,H);
	// Window manager should make sure that window is fully visible

	w->show(argc,argv);
	return Fl::run();
}

