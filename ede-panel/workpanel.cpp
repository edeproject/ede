// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "workpanel.h"
#include "aboutdialog.h"

#include "panelbutton.h"
#include "taskbutton.h"
#include "cpumonitor.h"
#include "batterymonitor.h"
#include "dock.h"
#include "mainmenu.h"
#include "keyboardchooser.h"

#include <efltk/Fl_WM.h>
#include <efltk/Fl_String.h>
#include <efltk/Fl_Pack.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Input_Browser.h>

#include "item.h"
#include <errno.h>

static Fl_Image desktop_pix((const char **)desktop_xpm);
static Fl_Image sound_pix((const char **)sound_xpm);
static Fl_Image run_pix((const char **)run_xpm);
static Fl_Image showdesktop_pix((const char **)showdesktop_xpm);

Fl_Button	 *mClockBox;
Fl_Group	 *mModemLeds;
Fl_Box		 *mLedIn, *mLedOut;

Fl_Button *mSoundMixer;
CPUMonitor *cpumon;
BatteryMonitor *batmon;

Fl_Input_Browser *runBrowser;

PanelMenu	 *mProgramsMenu;
PanelMenu	 *mWorkspace;
MainMenu	 *mSystemMenu;
KeyboardChooser	 *mKbdSelect;

Dock	*dock;
TaskBar *tasks;

Fl_Config pGlobalConfig(fl_find_config_file("ede.conf", true));

void		 updateStats(void *);


/////////////////////////////////////////////////////////////////////////////
// Fl_Update_Window is the name of Workpanel class


class Fl_Update_Window : public Fl_Window
{
public:
	Fl_Update_Window(int, int, int, int, char*);
	void setAutoHide(int a) { autoHide = a; }

	int handle(int event);

	int autoHide;
	int initX, initY, initW, initH;
};


// This global variable is here for stupidity
Fl_Update_Window *mPanelWindow;


Fl_Update_Window::Fl_Update_Window(int X, int Y, int W, int H, char *label=0)
	: Fl_Window(X, Y, W, H, label)
{
	autoHide = 0;
	initX = X;
	initY = Y;
	initW = W;
	initH = H;
}

int Fl_Update_Window::handle(int event) 
{
	switch (event) {
	case FL_SHOW: {
		int ret = Fl_Window::handle(FL_SHOW);
		int is_shown = shown();

		if(is_shown && !autoHide) 
			Fl_WM::set_window_strut(fl_xid(mPanelWindow), 0, 0, 0, h());

		if(is_shown && autoHide) 
			position(initX, initY);

		return ret;
	  }

	case FL_ENTER:
		if(!autoHide) {
//			position(initX, initY);
			if(shown()) Fl_WM::set_window_strut(fl_xid(mPanelWindow), 0, 0, 0, h());
		}
		else 
			position(initX, initY);

//		take_focus();
		return 1;

	case FL_LEAVE:
		if(autoHide && Fl::event() == FL_LEAVE) {
			position(initX, initY+h()-2);
			if(shown()) Fl_WM::set_window_strut(fl_xid(mPanelWindow), 0, 0, 0, 2);
		}
//		throw_focus();
		return 1;

	case FL_MOUSEWHEEL:
		if (mWorkspace == Fl::belowmouse()) {
			int count = Fl_WM::get_workspace_count();
			int current = Fl_WM::get_current_workspace();

			current-=Fl::event_dy(); // direction of dy is opposite
			while (current>=count) current-=count;
			while (current<0) current+=count;

			Fl_WM::set_current_workspace(current);
		}
	}

	return Fl_Window::handle(event);
}



/////////////////////////////////////////////////////////////////////////////


void setWorkspace(Fl_Widget *, long w)
{
	Fl_WM::set_current_workspace(w);
}

void restoreRunBrowser() {
	// Read the history of commands in runBrowser on start
	runBrowser->clear();
	Fl_String historyString;
	pGlobalConfig.get("Panel", "RunHistory", historyString,"");
	Fl_String_List history(historyString,"|");
	for (unsigned i = 0; i < history.count(); i++) {
		runBrowser->add(history[i]);
	}
}


bool showseconds = true;

void clockRefresh(void *)
{
	// Handle user's format preference
	Fl_String timestr = Fl_Date_Time::Now().time_string();
	Fl_String timestrsec;
	Fl_String format;
	pGlobalConfig.get("Clock", "TimeFormat", format, "");
	Fl_String seconds = timestr.sub_str(6, 2);
	Fl_String minutes = timestr.sub_str(3, 2);
	int hours = atoi(timestr.sub_str(0, 2));
	if(format == "12") {
		if(hours > 12) {
			hours -= 12;
		}
	}
	timestr = Fl_String(hours) + ":" + minutes;
	timestrsec = timestr + ":" + seconds;
	mClockBox->label(timestr);

	strncpy(Fl_Date_Time::datePartsOrder, _("MDY"), 3);
	Fl_String pClockTooltip = Fl_Date_Time::Now().day_name() + ", ";
	pClockTooltip += Fl_Date_Time::Now().date_string() + ", " + timestrsec;
	mClockBox->tooltip(pClockTooltip);

	mClockBox->redraw();
	Fl::add_timeout(1.0, clockRefresh);
}

void runUtility(Fl_Widget *, char *pCommand)
{
	char cmd[256];
	snprintf (cmd, sizeof(cmd)-1, "elauncher %s", pCommand);
	fl_start_child_process(cmd, false);
}

void updateSetup()
{
	//printf("updateSetup()\n");
	struct stat s;

	// Check if configuration needs to be updated
	if(!stat(pGlobalConfig.filename(), &s)) {
		static long last_modified = 0;
		long now = s.st_mtime;

		if(last_modified>0 && last_modified==now) {
			// Return if not modified
			return;
		}
		// Store last modified time
		last_modified = s.st_mtime;
	}

	pGlobalConfig.read_file(false);
	if(pGlobalConfig.error()) {
		fprintf(stderr, "[EWorkPanel Error]: %s\n", pGlobalConfig.strerror());
		return;
	}

	bool auto_hide = false;
	static bool hiden = false;
	static bool last_state = false;
	static bool on_start = true;
	bool runbrowser;
	
	pGlobalConfig.get("Panel", "AutoHide", auto_hide, false);
	if (on_start) {
		last_state = auto_hide;
		pGlobalConfig.get("Panel", "RunBrowser", runbrowser, true);
		if (runbrowser) { restoreRunBrowser() ; }
	}

	bool old_hiden = hiden;
	if (auto_hide) {
		mPanelWindow->setAutoHide(1);
		hiden = true;
	} else {
		mPanelWindow->setAutoHide(0);
		hiden = false;
	}

	if(old_hiden!=hiden || on_start) {
		if(!hiden) {
			mPanelWindow->position(mPanelWindow->initX, mPanelWindow->initY);
			if(mPanelWindow->shown()) Fl_WM::set_window_strut(fl_xid(mPanelWindow), 0, 0, 0, mPanelWindow->h());
		} else {
			mPanelWindow->position(mPanelWindow->initX, mPanelWindow->initY+mPanelWindow->h()-4);
			if(mPanelWindow->shown()) Fl_WM::set_window_strut(fl_xid(mPanelWindow), 0, 0, 0, 4);
		}
	}

	on_start = false;
}

void updateStats(void *)
{
	char pLogBuffer[1024];
	static int log_len;
	static struct timeval last_time;
	static long last_rx=0; static long last_tx=0;  static long connect_time=0;
	char *ptr; int fd; char buf[1024];
	long rx = -1; long tx = -1;
	float spi = 0.0; float spo = 0.0;
	static bool modleds = false;
	int len = -1;

	fd = open("/proc/net/dev", O_RDONLY);
	if(fd > 0) {

		len = read(fd, buf, 1023);
		close(fd);

		if(len>0) {
			buf[len] = '\0';
			ptr = strstr( buf, "ppp0" );
		}
	}

	if(fd==-1 || len < 0 || ptr == NULL)
	{
		if (modleds) {
			dock->remove_from_tray(mModemLeds);
			modleds = false;
		}
		last_rx=0; last_tx=0; connect_time=0;

	}
	else
	{
		long dt; int ct; struct timeval tv;

		gettimeofday(&tv, NULL);
		 dt = (tv.tv_sec - last_time.tv_sec) * 1000;
		 dt += (tv.tv_usec - last_time.tv_usec) / 1000;

		 if (dt > 0) {
		 sscanf(ptr, "%*[^:]:%ld %*d %*d %*d %*d %*d %*d %*d %ld", &rx, &tx); 
		 spi = (rx - last_rx) / dt;
		 spi = spi / 1024.0 * 1000.0;
		 spo = (tx - last_tx) / dt;
		 spo = spo / 1024.0 * 1000.0;
		

		 if ( connect_time == 0 ) 
			 connect_time = tv.tv_sec;
		 ct = (int)(tv.tv_sec - connect_time);

		 snprintf(pLogBuffer, 1024,
				 _("Received: %ld kB (%.1f kB/s)\n"
				 "Sent:		%ld kB (%.1f  kB/s)\n"
				 "Duration: %d min %d sec"), 
				  rx / 1024, spi, tx / 1024, spo, ct / 60, ct % 60 );
		 last_rx = rx;
		 last_tx = tx;
		 last_time.tv_sec = tv.tv_sec;
		 last_time.tv_usec = tv.tv_usec;

		 log_len = 0;

		 if ((int)(spi) > 0) 
		 {
			 mLedIn->color( (Fl_Color)2 );
			 mLedIn->redraw();
		 }	  
		 else 
		 {
			 mLedIn->color( (Fl_Color)968701184 );
			 mLedIn->redraw();
		 }

		 if ( (int)(spo) > 0 ) {
			 mLedOut->color( (Fl_Color)2 );
			 mLedOut->redraw();
		 } else {
			 mLedOut->color( (Fl_Color)968701184 );
			 mLedOut->redraw();
		}
		mModemLeds->tooltip(pLogBuffer);
	}
		if (!modleds) 
		 {
			 dock->add_to_tray(mModemLeds);
			 modleds = true;
		 }	  
	}	 

	updateSetup();
	Fl::repeat_timeout(1.0f, updateStats);
}


// Start utility, like "time/date" or "volume"
void startUtility(Fl_Button *, void *pName)
{
	Fl_String value;
	pGlobalConfig.get("Panel", (char*)pName, value, "");

	if(!pGlobalConfig.error() && !value.empty()) {
		value = "elauncher \""+value;
		value += "\"";
		fl_start_child_process(value, false);
	}
}

void updateWorkspaces(Fl_Widget*,void*)
{
	bool showapplet;
	pGlobalConfig.get("Panel", "Workspaces", showapplet, true);
	if (!showapplet) { return; }
	mWorkspace->clear();
	mWorkspace->begin();

	char **names=0;
	int count = Fl_WM::get_workspace_count();
	int names_count = Fl_WM::get_workspace_names(names);
	int current = Fl_WM::get_current_workspace();

	for(int n=0; n<count; n++) {
		Fl_Item *i = new Fl_Item();
		i->callback(setWorkspace, (long)n);
		i->type(Fl_Item::RADIO);
		if(n<names_count && names[n]) {
			i->label(names[n]);
			free(names[n]);
		} else {
			Fl_String tmp;
			i->label(tmp.printf(tmp, "%s %d", _("Workspace") ,n+1));
		}
		if(current==n) i->set_value();
	}
	// add a divider if there are no workspaces
	if (count<1) {
		new Fl_Menu_Divider();
	}
	if(names) delete []names;

	mWorkspace->end();
}

void FL_WM_handler(Fl_Widget *w, void *d)
{
	int e = Fl_WM::action();
//	printf (" --- eworkpanel handling %d\n",e);
	if(e==Fl_WM::WINDOW_NAME || e==Fl_WM::WINDOW_ICONNAME) {
		tasks->update_name(Fl_WM::window());
	}
	else if(e==Fl_WM::WINDOW_ACTIVE) {
		tasks->update_active(Fl_WM::get_active_window());
	}
	else if(e >= Fl_WM::WINDOW_LIST && e <= Fl_WM::WINDOW_DESKTOP) {
		tasks->update();
	}
	else if(Fl_WM::action()>0 && Fl_WM::action()<Fl_WM::DESKTOP_WORKAREA) {
		updateWorkspaces(w, d);
		tasks->update();
	}
}

void terminationHandler(int signum)
{
#ifndef DEBUG
	// to crash the whole X session can make people worried so try it
	// to save anymore...
	execlp("eworkpanel", "eworkpanel", 0);
#else
	fprintf(stderr, "\n\nEWorkPanel has just crashed! Please report bug to efltk-bugs@fltk.net\n\n");
	abort();
#endif		  
}

void cb_run_app(Fl_Input *i, void*)
{
	Fl_String exec(i->value());
	if ((exec == "") || (exec == " ")) 
		return;
	exec = "elauncher \""+exec;
	exec += "\"";
	fl_start_child_process(exec, false);
	
	Fl_Input_Browser *ib = (Fl_Input_Browser *)i->parent();
	if (!ib->find(i->value())) {
		ib->add(i->value());
		Fl_String_List history;
		int c = 0;
		if (ib->children() > 15) c = ib->children() - 15;
		for (; c < ib->children(); c++) {
			 history.append(ib->child(c)->label());
		}
		pGlobalConfig.set("Panel", "RunHistory", history.to_string("|"));
		pGlobalConfig.flush();
	}

	i->value("");
}
void cb_run_app2(Fl_Input_Browser *i, void*) { cb_run_app(i->input(), 0); }

void cb_showdesktop(Fl_Button *) {
// spagetti code - workpanel.cpp needs to be cleaned up of extraneous code
	tasks->minimize_all();
}


/////////////////////////////////////////////////////////////////////////////
// Quick Lunch Bar stuff - integrating code from dzooli
// TODO: remove this in EDE 2.0


#define QLB_CONFNAME "/.ede/qlb.conf"


static Fl_Image 	**qlb_images;
static Fl_Button 	**qlb_buttons;
static char 		**qlb_commands;
static char 		**qlb_tooltips;
static char 		**qlb_icons;
static int 		qlb_count;
static int 		qlb_numbuttons;



/**
 * cb_button - Button callback
 * @button: The button
 * @data: button data (command for exec)
 *
 * This function is the button callback function and executes the
 * specified command with the 'system()' call.
 */
static void cb_qlb_taskbutton(Fl_Button *button, void *data)
{
	fl_start_child_process((char *)data, false);
}

/**
 * get_buttonnum - Count the buttons in the config file
 * @configname: configuration file name
 *
 * The function opens the configuration file for the toolbar and
 * counts the all of the lines. Returns an unsigned integer or zero
 * if no configuration file found.
 */
unsigned int qlb_get_buttonnum(const char *configname)
{
	if (!configname)
		return 0;

	FILE* ifile=fopen(configname, "r+");
	if (!ifile)
		return 0;

	unsigned int c = 0;
	char buff[256];
	while (!feof(ifile)) {
		fgets(buff, sizeof(buff)-1, ifile);
		c++;
	}
	fclose(ifile);
	return c;
}

/**
 * load_config - read the configuration file
 * @confname: configuration file name
 *
 * The function reads the given configuration file and scans for
 * commands, icon names and tooltips. No checks done in the configuration
 * file lines and memory is dinamically allocated as needed.
 */
int qlb_load_config(const char *confname)
{
	int c;
	int i;
	int j, ln;
	FILE *ifile;
	char *s = (char *)calloc(512, sizeof(char));

	if (!confname) {
		free(s);
		fprintf(stderr, "Cannot open configfile\n");
		return -1;
	}
	ifile = fopen(confname, "r+");
	if (!ifile) {
		free(s);
		return -2;
	}
	c = 0;
	//while (!feof(ifile)) {
	qlb_numbuttons--;
	for (ln=0; ln<qlb_numbuttons; ln++) {
		fgets(s, 512, ifile);
		qlb_icons[c]=(char *)calloc(256, sizeof(char));
		qlb_tooltips[c]=(char *)calloc(256, sizeof(char));
		qlb_commands[c]=(char *)calloc(512, sizeof(char));
		i = 0;
		j = 0;
		while (s[i]!=':' && s[i])
			qlb_icons[c][j++]=s[i++];
		i++;
		j=0;
		while (s[i]!=':' && s[i])
			qlb_tooltips[c][j++]=s[i++];
		i++;
		j=0;
		while (s[i])
			qlb_commands[c][j++]=s[i++];
		qlb_commands[c][j-1]='&';
		c++;
	}
	return 0;
}

bool qlb_create_toolbuttons(const char *confname)
{
  char * home;
  char * configfile=(char *)calloc(256, sizeof(char));

  /* Memory allocation for the toolbar buttons */  
  if (confname) {
	qlb_numbuttons = qlb_get_buttonnum(confname);
	qlb_icons = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_tooltips = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_commands = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_load_config(confname);
  }    
  else {
	home = getenv("HOME");
	strcpy(configfile, home);
	strcat(configfile, QLB_CONFNAME);  	
	qlb_numbuttons = qlb_get_buttonnum(configfile);
	qlb_icons = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_tooltips = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_commands = (char **)calloc(qlb_numbuttons, sizeof(char*));
	qlb_load_config(configfile);
	free(configfile);
  }
  
  /* Error message if no configuration */
  if (!qlb_numbuttons) {
//  	fl_alert("No toolbar configuration!"); 
  	fprintf(stderr,"No toolbar configuration!\n"); 
  	return false;
  }
  
  /* Allocate image and buttons */
  qlb_count = 0;
  qlb_images = (Fl_Image**)calloc(qlb_numbuttons, sizeof(Fl_Image*));
  qlb_buttons = (Fl_Button**)calloc(qlb_numbuttons, sizeof(Fl_Button*));
  return true;
}




/////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv)
{
	signal(SIGCHLD, SIG_IGN);
	signal(SIGSEGV, terminationHandler);
	fl_init_locale_support("eworkpanel", PREFIX"/share/locale");
	fl_init_images_lib();

	int X=0,Y=0,W=Fl::w(),H=Fl::h();
	int substract;

	// Get current workarea
	Fl_WM::get_workarea(X,Y,W,H);

	//printf("Free area: %d %d %d %d\n", X,Y,W,H);

	// We expect that other docks are moving away from panel :)
	mPanelWindow = new Fl_Update_Window(X, Y+H-30, W, 30, "Workpanel");
	mPanelWindow->layout_spacing(0);
	// Panel is type DOCK
	mPanelWindow->window_type(Fl_WM::DOCK);
	mPanelWindow->setAutoHide(0);

	// Read config
	bool doShowDesktop;
	pGlobalConfig.get("Panel", "ShowDesktop", doShowDesktop, false);
	bool doWorkspaces;
	pGlobalConfig.get("Panel", "Workspaces", doWorkspaces, true);
	bool doQLB;
	pGlobalConfig.get("Panel", "QuickLaunchBar", doQLB, true);
	bool doRunBrowser;
	pGlobalConfig.get("Panel", "RunBrowser", doRunBrowser, true);	 
	bool doSoundMixer;
	pGlobalConfig.get("Panel", "SoundMixer", doSoundMixer, true);
	bool doCpuMonitor;
	pGlobalConfig.get("Panel", "CPUMonitor", doCpuMonitor, true);
	bool doBatteryMonitor;
	doBatteryMonitor=true;	// blah
	
	// Group that holds everything..
	Fl_Group *g = new Fl_Group(0,0,0,0);
	g->box(FL_DIV_UP_BOX);
	g->layout_spacing(2);
	g->layout_align(FL_ALIGN_CLIENT);
	g->begin();

	mSystemMenu = new MainMenu();

	Fl_VertDivider *v = new Fl_VertDivider(0, 0, 5, 18, "");
	v->layout_align(FL_ALIGN_LEFT);
	substract = 5;

	//this kind of if-else block is ugly
	// - but users demand such features so...
	if ((doShowDesktop) || (doWorkspaces) || (doQLB)) {

	int size=0;
	if ((doShowDesktop) && (doWorkspaces)) { size=48; } else { size=24; }
	// Add size for QLB:
	if (doQLB && qlb_create_toolbuttons(0)) size += (qlb_numbuttons * 24);

	Fl_Group *g2 = new Fl_Group(0,0,size,22);
	g2->box(FL_FLAT_BOX);
	g2->layout_spacing(0);
	g2->layout_align(FL_ALIGN_LEFT);

	// Show desktop button
	if (doShowDesktop) {
		PanelButton *mShowDesktop;
		mShowDesktop = new PanelButton(0, 0, 24, 22, FL_NO_BOX, FL_DOWN_BOX, "ShowDesktop");
		mShowDesktop->layout_align(FL_ALIGN_LEFT);
		mShowDesktop->label_type(FL_NO_LABEL);
		mShowDesktop->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
		mShowDesktop->image(showdesktop_pix);
		mShowDesktop->tooltip(_("Show desktop"));
		mShowDesktop->callback( (Fl_Callback*)cb_showdesktop);
		mShowDesktop->show();
		
		substract += 26;
	}
	
	// Workspaces panel
	mWorkspace=0; // so we can detect it later
	if (doWorkspaces) {
		mWorkspace = new PanelMenu(0, 0, 24, 22, FL_NO_BOX, FL_DOWN_BOX, "WSMenu");
		mWorkspace->layout_align(FL_ALIGN_LEFT);
		mWorkspace->label_type(FL_NO_LABEL);
		mWorkspace->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
		mWorkspace->image(desktop_pix);
		mWorkspace->tooltip(_("Workspaces"));
		mWorkspace->end();
		
		substract += 26;
	}

	// "Quick Lunch" buttons
	for (int count=0; count<qlb_numbuttons; count++) {
		qlb_buttons[count] = new PanelButton(0, 0, 24, 22, FL_NO_BOX, FL_DOWN_BOX, qlb_tooltips[count]);
		qlb_buttons[count]->layout_align(FL_ALIGN_LEFT);
		qlb_buttons[count]->label_type(FL_NO_LABEL);
		qlb_buttons[count]->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
		qlb_images[count] = NULL;
		qlb_images[count] = Fl_Image::read(qlb_icons[count]);
		if (!qlb_images[count])
			fprintf(stderr, "skipping icon: %s\n", qlb_icons[count]);
		else
			qlb_buttons[count]->image(qlb_images[count]);
		qlb_buttons[count]->tooltip(qlb_tooltips[count]);
		qlb_buttons[count]->callback((Fl_Callback*)cb_qlb_taskbutton, (void *)qlb_commands[count]);
		qlb_buttons[count]->show();

		substract += 26;
  	}
	


	g2->end();
	g2->show();
	g2->resizable();

	v = new Fl_VertDivider(0, 0, 5, 18, "");
		v->layout_align(FL_ALIGN_LEFT);
	substract += 5;
	}
	
	// Run browser
	if (doRunBrowser) {
		Fl_Group *g3 = new Fl_Group(0,0,100,20);
		g3->box(FL_FLAT_BOX);
		g3->layout_spacing(0);
		g3->layout_align(FL_ALIGN_LEFT);
		runBrowser = new Fl_Input_Browser("",100,FL_ALIGN_LEFT,30);
		//runBrowser->image(run_pix);
		runBrowser->box(FL_THIN_DOWN_BOX); // This is the only box type which works :(

		// Added _ALWAYS so callback is in case:
		// 1) select old command from input browser
		// 2) press enter to execute. (this won't work w/o _ALWAYS)
//		  runBrowser->input()->when(FL_WHEN_ENTER_KEY_ALWAYS | FL_WHEN_RELEASE_ALWAYS); 
		// Vedran: HOWEVER, with _ALWAYS cb_run_app will be called way
		// too many times, causing fork-attack
		runBrowser->input()->when(FL_WHEN_ENTER_KEY); 
		runBrowser->input()->callback((Fl_Callback*)cb_run_app);
		runBrowser->callback((Fl_Callback*)cb_run_app2);
		g3->end();
		g3->show();
		g3->resizable();

		v = new Fl_VertDivider(0, 0, 5, 18, "");
		v->layout_align(FL_ALIGN_LEFT);
	substract += 105;
	}


	// Popup menu for the whole taskbar
	Fl_Menu_Button *mPopupPanelProp = new Fl_Menu_Button( 0, 0, W, 28 );
	mPopupPanelProp->type( Fl_Menu_Button::POPUP3 );
	mPopupPanelProp->anim_flags(Fl_Menu_::LEFT_TO_RIGHT);
	mPopupPanelProp->anim_speed(0.8);
	mPopupPanelProp->begin();

	Fl_Item *mPanelSettings = new Fl_Item(_("Settings"));
	mPanelSettings->x_offset(12);
	mPanelSettings->callback( (Fl_Callback*)runUtility, (void*)"epanelconf" );
	new Fl_Divider(10, 5);

	Fl_Item *mAboutItem = new Fl_Item(_("About EDE..."));
	mAboutItem->x_offset(12);
	mAboutItem->callback( (Fl_Callback *)AboutDialog );

	mPopupPanelProp->end();

	// Taskbar...
	tasks = new TaskBar();

	// Dock and various entries...
	dock = new Dock();

	v = new Fl_VertDivider(0, 0, 5, 18, "");
	v->layout_align(FL_ALIGN_RIGHT);

	{
		// MODEM
		mModemLeds = new Fl_Group(0, 0, 25, 18);
		mModemLeds->box(FL_FLAT_BOX);
		mModemLeds->hide();
		mLedIn = new Fl_Box(2, 5, 10, 10);
		mLedIn->box( FL_OVAL_BOX );
		mLedIn->color( (Fl_Color)968701184);
		mLedOut = new Fl_Box(12, 5, 10, 10);
		mLedOut->box( FL_OVAL_BOX);
		mLedOut->color( (Fl_Color)968701184);
		mModemLeds->end();
	}

	{
		// KEYBOARD SELECT
		mKbdSelect = new KeyboardChooser(0, 0, 20, 18, FL_NO_BOX, FL_DOWN_BOX, "us");
		mKbdSelect->hide();
		mKbdSelect->anim_speed(4);
		mKbdSelect->label_font(mKbdSelect->label_font()->bold());
		mKbdSelect->highlight_color(mKbdSelect->selection_color());
		mKbdSelect->highlight_label_color( mKbdSelect->selection_text_color());
	}

	{
		// CLOCK
		mClockBox = new Fl_Button(0, 0, 50, 20);
		mClockBox->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
		mClockBox->hide();
		mClockBox->box(FL_FLAT_BOX);
		mClockBox->callback( (Fl_Callback*)startUtility, (void*)"Time and date");
	}

	// SOUND applet
	if (doSoundMixer) {
		mSoundMixer = new Fl_Button(0, 0, 20, 18);
		mSoundMixer->hide();
		mSoundMixer->box(FL_NO_BOX);
		mSoundMixer->focus_box(FL_NO_BOX);
		mSoundMixer->image(sound_pix);
		mSoundMixer->tooltip(_("Volume control"));
		mSoundMixer->align(FL_ALIGN_INSIDE);
		mSoundMixer->callback( (Fl_Callback*)startUtility, (void*)"Volume Control" );
	}

	// CPU monitor
	if (doCpuMonitor) {
		cpumon = new CPUMonitor();
		cpumon->hide();
	}

	// Battery monitor
	if (doBatteryMonitor) {
		batmon = new BatteryMonitor(dock);
		batmon->hide();
	}


	dock->add_to_tray(new Fl_Box(0, 0, 5, 20));
	dock->add_to_tray(mClockBox);
	dock->add_to_tray(mKbdSelect);
	dock->add_to_tray(mSoundMixer);
	dock->add_to_tray(cpumon);
	dock->add_to_tray(batmon);

	// end Dock


	Fl::focus(mSystemMenu);

	mPanelWindow->end();
	mPanelWindow->show(argc, argv);

	Fl_WM::callback(FL_WM_handler, 0, Fl_WM::DESKTOP_COUNT | 
								   Fl_WM::DESKTOP_NAMES |
							   Fl_WM::DESKTOP_CHANGED|
							   Fl_WM::WINDOW_LIST|
							   Fl_WM::WINDOW_DESKTOP|
							   Fl_WM::WINDOW_ACTIVE|
							   Fl_WM::WINDOW_NAME|
							   Fl_WM::WINDOW_ICONNAME);

	updateWorkspaces(0,0);
	tasks->update();

	Fl::add_timeout(0, clockRefresh);
	Fl::add_timeout(0, updateStats);

	while(mPanelWindow->shown())
		Fl::wait();
}
