#include <fltk/run.h>
#include <fltk/Window.h>
#include <fltk/DoubleBufferWindow.h>
#include <fltk/Button.h>
#include <fltk/SharedImage.h>
#include <fltk/ScrollGroup.h>
#include <fltk/Widget.h>
#include <fltk/filename.h>
#include <fltk/events.h>
#include <fltk/PopupMenu.h>
#include <fltk/Item.h>
#include <fltk/Divider.h>
#include <fltk/Divider.h>
#include <fltk/file_chooser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


using namespace fltk;



// Automatic detection of image type still doesn't work in fltk :(
static struct {
	const char* filename;
	SharedImage* (*func)(const char*, const uchar*);
} supported[] = {
	{ "*.png", pngImage::get},
	{"*.jpg", jpegImage::get},
	{"*.gif", gifImage::get},
	{"*.bmp", bmpImage::get},
	{"*.xpm", xpmFileImage::get},
	{0, 0}
};




// Making these global vastly simplifies the code
// cause they're used a lot
// The proper thing to do is to make the whole program one big class
char filename[PATH_MAX], directory[PATH_MAX];
Window* w;
class CenteredInScroll;
CenteredInScroll* c;
float zoomfactor;
bool autozoom=false;

// Variables used in nextpic & prevpic
dirent **files;
int nfiles;


// Forward declaration of funcs for use from event handler
void nextpic();
void prevpic();
void loadimage();
void newdir();


// Callbacks for popup menu
void next_cb(Widget*) { nextpic(); }
void prev_cb(Widget*) { prevpic(); }
void open_cb(Widget*) {
	const char* f = file_chooser("Choose image or directory","Image Files (*.{bmp,gif,jpg,png,xpm})",directory);
	if (!f) return;
	strncpy(filename,f,PATH_MAX);
	newdir();
	loadimage();
}
void manage_cb(Widget* b) {} // call file manager
void fullscr_cb(Widget* b) {
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
void exit_cb(Widget* b) { exit(0); } 
void zoomin_cb(Widget* b) { 
	if (zoomfactor>=1) zoomfactor += 0.2; else zoomfactor += zoomfactor/5; 
	autozoom=false; 
	loadimage(); 
}
void zoomout_cb(Widget* b) { 
	if (zoomfactor>=1) zoomfactor -= 0.2; else zoomfactor -= zoomfactor/5; 
	autozoom=false; 
	loadimage(); 
}
void zoomrestore_cb(Widget* b) { zoomfactor = 1; autozoom=false; loadimage(); }
void zoomauto_cb(Widget *b) { autozoom = !autozoom; loadimage(); } 




class CenteredInScroll : public Button {
private:
	PopupMenu* popup;
public:
  CenteredInScroll(int x, int y, int w, int h, const char*l = 0)
    : Button(x,y,w,h,l) {
	popup = new PopupMenu(0, 0, 0, 0);
	if(popup->parent())
		popup->parent()->remove(popup);
	popup->parent(0);
	popup->type(PopupMenu::POPUP3);
	popup->begin();

	{Item *i = new Item("&Open...");
	i->callback(open_cb);
	i->shortcut(CTRL+'o');
	}
	
	{Item *i = new Item("&Manage...");
	i->callback(manage_cb);
	}
	
	new Divider();
	
	{Item *i = new Item("&Next");
	i->callback(next_cb);
	i->shortcut(DownKey);
	}

	{Item *i = new Item("&Previous");
	i->callback(prev_cb);
	i->shortcut(UpKey);
	}

	new Divider();
	
	{Item *i = new Item("&Zoom in");
	i->callback(zoomin_cb);
	i->shortcut('+');
	}

	{Item *i = new Item("Zoom &out");
	i->callback(zoomout_cb);
	i->shortcut('-');
	}

	{Item *i = new Item("Zoom &auto");
	i->callback(zoomauto_cb);
	i->shortcut(CTRL+'a');
	}

	{Item *i = new Item("&Restore");
	i->callback(zoomrestore_cb);
	i->shortcut('/');
	}

	new Divider();

	{Item *i = new Item("&Fullscreen");
	i->callback(fullscr_cb);
	i->shortcut(F11Key);
	}

	new Divider();
	
	{Item *i = new Item("A&bout");
	i->callback(exit_cb);
	}
	
	{Item *i = new Item("&Exit");
	i->callback(exit_cb);
	i->shortcut(EscapeKey);
	}



	popup->end();
  }

  virtual void draw() { 
	int ix, iy;
	if (w() < parent()->w()) { ix=((parent()->w() - w())/2); } else { ix=0; }
	if (h() < parent()->h()) { iy=((parent()->h() - h())/2); } else { iy=0; }
	Image *i = (Image*)image();
	i->draw(Rectangle(ix,iy,w(),h()));
  }

  virtual int handle(int event) {
	if (event == KEY) {
		int key = event_key();
		const char* text = event_text(); // needed to detect some keys
		if (key == LeftKey || key == UpKey) prevpic();
		else if (key == RightKey || key == DownKey) nextpic();
		// Shortcuts from popup menu wont work unless we define them here
		// The 'this' is just to pass something to callback, it wont be used
		else if (key == CTRL+'o') open_cb(this);
		else if (strcmp(text,"+")==0 || key == AddKey) zoomin_cb(this);
		else if (strcmp(text,"-")==0 || key == SubtractKey) zoomout_cb(this);
		else if (strcmp(text,"/")==0 || key == DivideKey) zoomrestore_cb(this);
		else if (key == CTRL+'a') zoomauto_cb(this);
		else if (key == F11Key) fullscr_cb(this);
	}
	else if (event == PUSH) {
		if(event_button()==3) {
			popup->popup();
			return 1;
		}
	}
	return Button::handle(event);
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
	fprintf(stderr, "Loadimage() - file: %s\n",filename);

	// Load image
	SharedImage *im;
	for (int j=0; supported[j].filename; j++)
		if (filename_match(filename,supported[j].filename)) {
			im = supported[j].func(filename,0);
			break;
		}

	int W,H;
	im->measure(W,H); // Should this wait until image is loaded?
	if (autozoom) {
		// Adjust zoom factor so picture fits on screen
		// When switch to manual zooming, this factor will be used
		float pw=c->parent()->w()+1, ph=c->parent()->h()+1; 
			// use float to avoid rounding
		if (pw/W < ph/H) zoomfactor=pw/W; else zoomfactor=ph/H;
	}

	c->w((int)W*zoomfactor-1);	// cast to int and -1 help avoid 
	c->h((int)H*zoomfactor-1);	// scrollbars when autozoom is on
	c->image(im);

	c->parent()->relayout(); // remove scrollbars if necessary
	c->parent()->redraw(); // remove traces of old picture

	c->label(""); // clear any previous labels

	char tmp[PATH_MAX]; // set window title
	if (zoomfactor==1)
		snprintf(tmp,PATH_MAX,"%s (%dx%d) - View picture",filename_name(filename),W,H);
	else
		snprintf(tmp,PATH_MAX,"%s (%dx%d) - zoom %1.1fx - View picture",filename_name(filename),W,H,zoomfactor);
	w->label(strdup(tmp));
}


// Get next/previous picture file in directory d
// (universal func. to be called from nextpic() and prevpic()
void prevnext(int direction) {
	fprintf(stderr, "Nextpic() - file: %s dir: %s direction: %d\n",filename,directory,direction);

	if (nfiles == 0) { // read directory
		nfiles = filename_list(directory,&files);
	}

	// Select next picture after current
	bool found=false;
	const char* justname = filename_name(filename);
	// this basically means: if direction is 1 go from first to last, else from last to first
	for (int i=(direction?0:nfiles-1); (direction?i<nfiles:i>=0); i+=(direction?1:-1)) {
		if (strncmp(justname,files[i]->d_name,PATH_MAX) == 0) {
			found=true;
			continue;
		}
		if (found) {
			for (int j=0; supported[j].filename; j++) {
				if (filename_match(files[i]->d_name,supported[j].filename)) {
					snprintf(filename,PATH_MAX,"%s/%s",directory,files[i]->d_name);
					loadimage();
					return;
				}
			}
		}
	}

	// Current picture not found, give first in directory
	for (int i=(direction?0:nfiles-1); (direction?i<nfiles:i>=0); i+=(direction?1:-1)) {
		for (int j=0; supported[j].filename; j++) {
			if (filename_match(files[i]->d_name,supported[j].filename)) {
				snprintf(filename,PATH_MAX,"%s/%s",directory,files[i]->d_name);
				loadimage();
				return;
			}
		}
	}

	// Nothing found...
	fprintf(stderr, "Nextpic() - nothing found\n");
	char tmp[PATH_MAX];
	snprintf(tmp,PATH_MAX,"No pictures in directory %s",directory);
	c->label(strdup(tmp));
	// Position label on center and redraw everything
	c->w(1); c->h(1);
	c->x(c->parent()->w()/2);
	c->y(c->parent()->h()/2);
	c->redraw();
	c->parent()->relayout();
	c->parent()->redraw();
	// Window title
	snprintf(tmp,PATH_MAX,"View picture - nothing found in %s",directory);
	w->label(strdup(tmp));
}

void nextpic() { prevnext(1); }
void prevpic() { prevnext(0); }



int main (int argc, char **argv) {
	filename[0]='\0'; directory[0]='\0'; zoomfactor=1;

	w = new Window(200, 200, "View picture");
	w->set_vertical();
	w->set_double_buffer();
	w->begin();
	{ScrollGroup* g = new ScrollGroup(0, 0, 200, 200);
		//g->set_vertical();
//		Group::current()->resizable(o);
		g->box(FLAT_BOX);
		g->color(GRAY05);
//		g->color(WHITE);
		g->align(ALIGN_RIGHT);
		g->resize_align(ALIGN_RIGHT);
		g->begin();
		{c = new CenteredInScroll(0,0,200,200);
			g->align(ALIGN_LEFT|ALIGN_TOP);
			c->box(NO_BOX);
			c->focusbox(NO_BOX);
			g->color(GRAY05);
			c->labelcolor(WHITE);
			c->labelsize(14);
			c->tooltip("Right click for menu");
			c->take_focus();
		}
		g->end();
	}
	w->end();
	w->resizable(w);

	// Analyze command line

	if (argc==1) { // No params
		strncpy (directory, getenv("HOME"), PATH_MAX);
		nextpic();

	} else if (filename_isdir(argv[1])) { // Param is directory
		strncpy (directory, argv[1], PATH_MAX);
		nextpic();
		argc--; argv++; // ignore this param and forward rest to fltk

	} else { // Param is file
		if (argv[1][0] == '~' && argv[1][1] == '/') // expand home dir
			snprintf (filename, PATH_MAX, "%s/%s", getenv("HOME"), argv[1]+2);
		else if (argv[1][0] != '/') // relative filename
			snprintf (filename, PATH_MAX, "%s/%s", getenv("PWD"), argv[1]);
		else // absolute filename
			strncpy (filename, argv[1], PATH_MAX);

		if (!filename_exist(argv[1])) {
			char tmp[PATH_MAX];
			snprintf(tmp,PATH_MAX,"File not found - %s",filename);
			c->label(tmp);
		} else 
			loadimage();

		newdir(); // rebuild char[] directory
		argc--; argv++; // ignore this param and forward rest to fltk
	}

	w->show(argc,argv);
	return run();
}

