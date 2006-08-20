// Copyright (c) 2000. - 2005. EDE Authors
// This program is licenced under terms of the
// GNU General Public Licence version 2 or newer.
// See COPYING for details.

#include "workpanel.h"
#include "aboutdialog.h"

#include "panelbutton.h"
#include "taskbutton.h"
#include "cpumonitor.h"
#include "dock.h"
#include "mainmenu.h"
#include "keyboardchooser.h"

/*#include <efltk/Fl_WM.h>
#include <efltk/Fl_String.h>
#include <efltk/Fl_Pack.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Input_Browser.h>*/

#include <fltk/WM.h>
#include <fltk/String.h>
#include <fltk/Pack.h>
#include <fltk/Divider.h>
#include <fltk/Input_Browser.h>

#include "item.h"
#include <errno.h>

static fltk::Image desktop_pix((const char **)desktop_xpm);
static fltk::Image sound_pix((const char **)sound_xpm);
static fltk::Image run_pix((const char **)run_xpm);
static fltk::Image showdesktop_pix((const char **)showdesktop_xpm);

class Fl_Update_Window : public fltk::Window
{
public:
	Fl_Update_Window(int, int, int, int, char*);
	void setAutoHide(int a) { autoHide = a; }

	int handle(int event);

	int autoHide;
	int initX, initY, initW, initH;
};

Fl_Update_Window *mPanelWindow;

fltk::Button	 *mClockBox;
fltk::Group	 *mModemLeds;
fltk::Box		 *mLedIn, *mLedOut;

fltk::Input_Browser *runBrowser;

PanelMenu	 *mProgramsMenu;
PanelMenu	 *mWorkspace;
MainMenu	 *mSystemMenu;
KeyboardChooser	 *mKbdSelect;

Dock	*dock;
TaskBar *tasks;

EDE_Config pGlobalConfig(find_config_file("ede.conf", true));

void		 updateStats(void *);


/////////////////////////////////////////////////////////////////////////////


Fl_Update_Window::Fl_Update_Window(int X, int Y, int W, int H, char *label=0)
	: fltk::Window(X, Y, W, H, label)
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
	case fltk::SHOW: {
		int ret = fltk::Window::handle(FL_SHOW);
		int is_shown = shown();

		if(is_shown && !autoHide) 
			Fl_WM::set_window_strut(fltk::xid(mPanelWindow), 0, 0, 0, h());

		if(is_shown && autoHide) 
			position(initX, initY);

		return ret;
	  }

	case fltk::ENTER:
		if(!autoHide) {
			position(initX, initY);
			if(shown()) Fl_WM::set_window_strut(fltk::xid(mPanelWindow), 0, 0, 0, h());
		}
		else 
			position(initX, initY);

		take_focus();
		return 1;

	case FL_LEAVE:
		if(autoHide && fltk::event() == fltk::LEAVE) {
			position(initX, initY+h()-2);
			if(shown()) Fl_WM::set_window_strut(fltk::xid(mPanelWindow), 0, 0, 0, 2);
		}
		throw_focus();
		return 1;
	}
	return fltk::Window::handle(event);
}

void setWorkspace(fltk::Button *, void *w)
{
	Fl_WM::set_current_workspace((int) w);
}

void restoreRunBrowser() {
	// Read the history of commands in runBrowser on start
	runBrowser->clear();
	char* historyString;
	pGlobalConfig.get("Panel", "RunHistory", historyString,"");
	char* token = strtok(historyString,"|");
	do {
		runBrowser->add(token);
	} while (token = strtok(NULL, "|"));
}


bool showseconds = true;

void clockRefresh(void *)
{
	mClockBox->label(fltk::Date_Time::Now().time_string().sub_str(0, 5));

	strncpy(fltk::Date_Time::datePartsOrder, _("MDY"), 3);
	char* pClockTooltip = strdup(Fl_Date_Time::Now().day_name());
	pClockToolTip = strdupcat(pClockToolTip, ", ");
	pClockTooltip = strdupcat(pClockToolTip, Fl_Date_Time::Now().date_string());
	pClockToolTip = strdupcat(pClockToolTip, ", ");
	pClockToolTip = strdupcat(Fl_Date_Time::Now().time_string());
	mClockBox->tooltip(pClockTooltip);

	mClockBox->redraw();
	fltk::add_timeout(1.0, clockRefresh);
	free(pClockToolTip);
}

void runUtility(fltk::Widget *, char *pCommand)
{
	char cmd[256];
	snprintf (cmd, sizeof(cmd)-1, "elauncher %s", pCommand);
	fltk::start_child_process(cmd, false);
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
			if(mPanelWindow->shown()) Fl_WM::set_window_strut(fltk::xid(mPanelWindow), 0, 0, 0, mPanelWindow->h());
		} else {
			mPanelWindow->position(mPanelWindow->initX, mPanelWindow->initY+mPanelWindow->h()-4);
			if(mPanelWindow->shown()) Fl_WM::set_window_strut(fltk::xid(mPanelWindow), 0, 0, 0, 4);
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
	fltk::repeat_timeout(1.0f, updateStats);
}


// Start utility, like "time/date" or "volume"
void startUtility(fltk::Button *, void *pName)
{
	char* value;
	pGlobalConfig.get("Panel", pName, value, "");

	if(!pGlobalConfig.error() && strlen(value)>0) {
		char* value2 = strdup("elauncher \"");
		value2 = strdupcat(value2,value);
		value2 = strdupcat(value2,"\"");
		fltk::start_child_process(value2, false);
		free(value2);
	}
	free(value);
}

void updateWorkspaces(fltk::Widget*,void*)
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
		fltk::Item *i = new fltk::Item();
		i->callback( (fltk::Callback *) setWorkspace, (void*)n);
		i->type(fltk::Item::RADIO);
		if(n<names_count && names[n]) {
			i->label(names[n]);
			delete []names[n];
		} else {
			char* tmp;
			sprintf(tmp,"%s %d", _("Workspace") ,n+1);
			i->label(tmp);
			free(tmp);
		}
		if(current==n) i->set_value();
	}
	if(names) delete []names;

	mWorkspace->end();
}

void FL_WM_handler(fltk::Widget *w, void *d)
{
	if(fltk::WM::action()==fltk::WM::WINDOW_NAME || fltk::WM::action()==fltk::WM::WINDOW_ICONNAME) {
		tasks->update_name(fltk::WM::window());
	}
	if(fltk::WM::action()==fltk::WM::WINDOW_ACTIVE) {
		tasks->update_active(fltk::WM::get_active_window());
	}
	else if(fltk::WM::action() >= fltk::WM::WINDOW_LIST && fltk::WM::action() <= fltk::WM::WINDOW_DESKTOP) {
		tasks->update();
	}
	else if(fltk::WM::action()>0 && fltk::WM::action()<fltk::WM::DESKTOP_WORKAREA) {
		updateWorkspaces(w, d);
		tasks->update();
	}
}

#define DEBUG
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
	char* exec = strdup(i->value());
	if (strcmp(exec,"")==0 || strcmp(exec," ")==0) 
		return;
	char* exec2 = strdup("elauncher \"");
	exec2 = strdupcat(exec2,exec);
	exec2 = strdupcat(exec2,"\"");
	fltk::start_child_process(exec, false);
	
	fltk::Input_Browser *ib = (fltk::Input_Browser *)i->parent();
	if (!ib->find(i->value())) {
		ib->add(i->value());
		int c = 0;
		if (ib->children() > 15) c = ib->children() - 15;
		char* history;
		for (; c < ib->children()-1; c++) {
			 history = strdupcat(ib->child(c)->label());
			 history = strdupcat("|");
		}
		history = strdupcat(ib->child(ib->children()-1)->label());
		pGlobalConfig.set("Panel", "RunHistory", history);
		pGlobalConfig.flush();
	}

	i->value("");
}
void cb_run_app2(fltk::Input_Browser *i, void*) { cb_run_app(i->input(), 0); }

void cb_showdesktop(fltk::Button *) {
// spagetti code - workpanel.cpp needs to be cleaned up of extraneous code
	tasks->minimize_all();
}

int main(int argc, char **argv)
{
	signal(SIGCHLD, SIG_IGN);
	signal(SIGSEGV, terminationHandler);
//	fl_init_locale_support("eworkpanel", PREFIX"/share/locale");
	fltk::init_images_lib();

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
	bool doRunBrowser;
	pGlobalConfig.get("Panel", "RunBrowser", doRunBrowser, true);	 
	bool doSoundMixer;
	pGlobalConfig.get("Panel", "SoundMixer", doSoundMixer, true);
	bool doCpuMonitor;
	pGlobalConfig.get("Panel", "CPUMonitor", doCpuMonitor, true);
	
	// Group that holds everything..
	fltk::Group *g = new fltk::Group(0,0,0,0);
	g->box(fltk::DIV_UP_BOX);
	g->layout_spacing(2);
	g->layout_align(fltk::ALIGN_CLIENT);
	g->begin();

	mSystemMenu = new MainMenu();

	fltk::VertDivider *v = new fltk::VertDivider(0, 0, 5, 18, "");
	v->layout_align(fltk::ALIGN_LEFT);
	substract = 5;

	if ((doShowDesktop) || (doWorkspaces)) {
	//this is ugly:
	int size;
	if ((doShowDesktop) && (doWorkspaces)) { size=48; } else { size=24; }
	fltk::Group *g2 = new fltk::Group(0,0,size,22);
	g2->box(fltk::FLAT_BOX);
	g2->layout_spacing(0);
	g2->layout_align(fltk::ALIGN_LEFT);

	// Show desktop button
	if (doShowDesktop) {
		PanelButton *mShowDesktop;
		mShowDesktop = new PanelButton(0, 0, 24, 22, fltk::NO_BOX, fltk::DOWN_BOX, "ShowDesktop");
		mShowDesktop->layout_align(fltk::ALIGN_LEFT);
		mShowDesktop->label_type(fltk::NO_LABEL);
		mShowDesktop->align(fltk::ALIGN_INSIDE|fltk::ALIGN_CENTER);
		mShowDesktop->image(showdesktop_pix);
		mShowDesktop->tooltip(_("Show desktop"));
		mShowDesktop->callback( (fltk::Callback*)cb_showdesktop);
		mShowDesktop->show();
		
		substract += 26;
	}
	
	// Workspaces panel
	if (doWorkspaces) {
		mWorkspace = new PanelMenu(0, 0, 24, 22, fltk::NO_BOX, fltk::DOWN_BOX, "WSMenu");
		mWorkspace->layout_align(fltk::ALIGN_LEFT);
		mWorkspace->label_type(fltk::NO_LABEL);
		mWorkspace->align(fltk::ALIGN_INSIDE|fltk::ALIGN_CENTER);
		mWorkspace->image(desktop_pix);
		mWorkspace->tooltip(_("Workspaces"));
		mWorkspace->end();
		
		substract += 26;
	}
	
	g2->end();
	g2->show();
	g2->resizable();

	v = new fltk::VertDivider(0, 0, 5, 18, "");
		v->layout_align(fltk::ALIGN_LEFT);
	substract += 5;
	}
	
	// Run browser
	if (doRunBrowser) {
		runBrowser = new fltk::Input_Browser("",100,fltk::ALIGN_LEFT,30);
		//runBrowser->image(run_pix);
		//runBrowser->box(FL_THIN_DOWN_BOX);

		// Added _ALWAYS so callback is in case:
		// 1) select old command from input browser
		// 2) press enter to execute. (this won't work w/o _ALWAYS)
//		  runBrowser->input()->when(FL_WHEN_ENTER_KEY_ALWAYS | FL_WHEN_RELEASE_ALWAYS); 
		// Vedran: HOWEVER, with _ALWAYS cb_run_app will be called way
		// too many times, causing fork-attack
		runBrowser->input()->when(fltk::WHEN_ENTER_KEY); 
		runBrowser->input()->callback((fltk::Callback*)cb_run_app);
		runBrowser->callback((fltk::Callback*)cb_run_app2);

		v = new fltk::VertDivider(0, 0, 5, 18, "");
		v->layout_align(fltk::ALIGN_LEFT);
		substract += 105;
	}


	// Popup menu for the whole taskbar
	fltk::Menu_Button *mPopupPanelProp = new fltk::Menu_Button( 0, 0, W, 28 );
	mPopupPanelProp->type( fltk::Menu_Button::POPUP3 );
	mPopupPanelProp->anim_flags(fltk::Menu_::LEFT_TO_RIGHT);
	mPopupPanelProp->anim_speed(0.8);
	mPopupPanelProp->begin();

	fltk::Item *mPanelSettings = new fltk::Item(_("Settings"));
	mPanelSettings->x_offset(12);
	mPanelSettings->callback( (fltk::Callback*)runUtility, (void*)"epanelconf" );
	new fltk::Divider(10, 5);

	fltk::Item *mAboutItem = new fltk::Item(_("About EDE..."));
	mAboutItem->x_offset(12);
	mAboutItem->callback( (fltk::Callback *)AboutDialog );

	mPopupPanelProp->end();

	// Subgroup to properly align everything
/*	  Fl_Group *subgroup;
	{
		subgroup = new Fl_Group(0, 0, W-substract, 18);
		subgroup->box(FL_FLAT_BOX);
		subgroup->layout_align(FL_ALIGN_RIGHT);
		subgroup->show();
		subgroup->begin(); */
	
		// Taskbar...
		tasks = new TaskBar();
		dock = new Dock();

		v = new fltk::VertDivider(0, 0, 5, 18, "");
		v->layout_align(fltk::ALIGN_RIGHT);
	
/*		  subgroup->end();
	}*/

	{
		// MODEM
		mModemLeds = new fltk::Group(0, 0, 25, 18);
		mModemLeds->box(fltk::FLAT_BOX);
		mModemLeds->hide();
		mLedIn = new fltk::Box(2, 5, 10, 10);
		mLedIn->box( fltk::OVAL_BOX );
		mLedIn->color( (fltk::Color)968701184);
		mLedOut = new fltk::Box(12, 5, 10, 10);
		mLedOut->box( fltk::OVAL_BOX);
		mLedOut->color( (fltk::Color)968701184);
		mModemLeds->end();
	}

	{
		// KEYBOARD SELECT
		mKbdSelect = new KeyboardChooser(0, 0, 20, 18, fltk::NO_BOX, fltk::DOWN_BOX, "us");
		mKbdSelect->hide();
		mKbdSelect->anim_speed(4);
		mKbdSelect->label_font(mKbdSelect->label_font()->bold());
		mKbdSelect->highlight_color(mKbdSelect->selection_color());
		mKbdSelect->highlight_label_color( mKbdSelect->selection_text_color());
	}

	{
		// CLOCK
		mClockBox = new fltk::Button(0, 0, 50, 20);
		mClockBox->align(fltk::ALIGN_INSIDE|fltk::ALIGN_LEFT);
		mClockBox->hide();
		mClockBox->box(fltk::FLAT_BOX);
		mClockBox->callback( (Fl_Callback*)startUtility, (void*)"Time and date");
	}
 
	dock->add_to_tray(mClockBox);
	dock->add_to_tray(mKbdSelect);
	
	// SOUND applet
	if (doSoundMixer) {
		fltk::Button *mSoundMixer;
		mSoundMixer = new fltk::Button(0, 0, 20, 18);
		mSoundMixer->hide();
		mSoundMixer->box(fltk::NO_BOX);
		mSoundMixer->focus_box(fltk::NO_BOX);
		mSoundMixer->image(sound_pix);
		mSoundMixer->tooltip(_("Volume control"));
		mSoundMixer->align(fltk::ALIGN_INSIDE);
		mSoundMixer->callback( (fltk::Callback*)startUtility, (void*)"Volume Control" );
		dock->add_to_tray(mSoundMixer);
	}

	// CPU monitor
	if (doCpuMonitor) {
		CPUMonitor *cpumon;
		cpumon = new CPUMonitor();
		cpumon->hide();
		dock->add_to_tray(cpumon);
	}


	fltk::focus(mSystemMenu);

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

	fltk::add_timeout(0, clockRefresh);
	fltk::add_timeout(0, updateStats);

	while(mPanelWindow->shown())
		fltk::wait();
}
