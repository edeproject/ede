/*
 * $Id$
 *
 * Program and URL opener
 * Provides startup notification, crash handler and other features
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "elauncher.h"
#include "../edeconf.h"
#include "../edelib2/process.h"
#include "../edelib2/MimeType.h"

using namespace fltk;
using namespace edelib;



// TODO: find where to replace magic constants with fltk::PATH_MAX





// globals used in forking
int fds_[3];
char *cmd_;
int pid_;

// command-line parameters
bool param_root = false;
bool param_secure = false;
bool param_term = false;

// from configuration file
bool use_sudo = false;

char *output;




/*char *
itoa(int value, char *string, int radix)
{
	char tmp[33];
	char *tp = tmp;
	int i;
	unsigned v;
	int sign;
	char *sp;
	
	if (radix > 36 || radix <= 1)
	{
		return 0;
	}
	
	sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;
	while (v || tp == tmp)
	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'a' - 10;
	}
	
	if (string == 0)
		string = (char *)malloc((tp-tmp)+sign+1);
	sp = string;
	
	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
	return string;
}*/



// --------------------------------------------
// Show busy cursor - not working
// --------------------------------------------
void show_busy_screen(bool busy)
{
	// We can't use fltk::Cursor cause it can be set only per widget...
	// and only if you overload that widget!
	// I hate OOP :)
	::Cursor xcursor;
	if (busy)
		xcursor = XCreateFontCursor(xdisplay, XC_watch);
	else
		xcursor = XCreateFontCursor(xdisplay, XC_arrow);
	// Hopefully this is desktop?
	XDefineCursor(xdisplay, CreatedWindow::first->xid, xcursor);
	sleep (3);
}



// --------------------------------------------
// Show a generic window for displaying output stream
// --------------------------------------------

void output_window_close(Widget *w)
{
	w->window()->hide();
}

void output_window(char *title, char *content)
{
	int height=0;
	TextBuffer buffer;
	buffer.text(content);
	
	for (unsigned i=0;i<strlen(content);i++) 
		if (content[i] == '\n') height+=20;
	height+=45;
	if (height>550) height=550;
	if (height<100) height=100;
	
	Window window(500, height);
	window.label(title);
	window.begin();
		
	TextDisplay message(0, 0, 500, height-23, content);
	window.resizable(message);
	message.color(WHITE);
	message.textcolor(BLACK);
	message.buffer(buffer);
	
	Button* button;
	button = new ReturnButton(410, height-23, 80, 23, _("&Ok"));
	button->callback(output_window_close);
	window.hotspot(button);
	//    window.focus(button);
	window.end();
	window.exec();
}


// --------------------------------------------
// Crash window - with details
// --------------------------------------------
#define GDBPATH "/usr/bin/gdb"

static xpmImage crash_pix((const char **)crash_xpm);
Window *crashWindow;
Button *crashDetailsButton, *crashCloseButton;
TextDisplay *backTraceTD;
TextBuffer *gdbbuffer;

void cb_crashDetails(Widget *w) {
	if (backTraceTD->visible()) {
		backTraceTD->hide();
		crashWindow->resize(450,110);
	} else {
		crashWindow->resize(450,395);
		backTraceTD->show();
	}
}

void cb_crashOk(Widget *w) {
  w->window()->hide();
}

// Execute gdb and place output into gdbbuffer
bool get_me_gdb_output(int crashpid)
{
	int pid, status;
	extern char **environ;
	
	status=0;
	pid = fork ();
	if (pid == -1)
		return false;
	if (pid == 0)
	{
		// child
		char *argv[4];
		char tmp[1000];
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = tmp;
		snprintf (argv[2], 999, "echo bt>/tmp/gdbbatch; echo q>>/tmp/gdbbatch; "GDBPATH" %s --core core.%d  --command /tmp/gdbbatch --quiet > /tmp/gdboutput", cmd_, crashpid);
		argv[3] = NULL;
		
		if (execve ("/bin/sh", argv, environ) == -1)
			perror ("/bin/sh");
		return false; // Error
	}
	do
	{
		if (waitpid (pid, &status, 0) == -1)
		{
			if (errno != EINTR)
				return false; // Error
		}
		else {
			gdbbuffer->loadfile("/tmp/gdboutput");
			// Take out the garbage
			char *corefile = (char*)malloc(20);
			snprintf (corefile, sizeof(corefile)-1, "./core.%d", crashpid);
			unlink(corefile);
			free(corefile);
			return true;
		}
	}
	while (1);
}

void crashmessage(char *command, int pid)
{
	gdbbuffer = new TextBuffer;
	Window* w;
	{
		Window* o = crashWindow = new Window(450, 110, _("The program has crashed"));
		w = o;
		o->shortcut(0xff1b);
		o->begin();
		{
			Button* o = crashDetailsButton = new Button(250, 75, 90, 25, _("&Details..."));
			o->callback((Callback*)cb_crashDetails); 
			o->type(Button::TOGGLE);
		}
		{
			Button* o = crashCloseButton = new Button(350, 75, 90, 25, _("&Close"));
			o->callback((Callback*)cb_crashOk); 
		}
		{
			InvisibleBox* o = new InvisibleBox(60, 5, 380, 16, _("An error occured in program:"));
			o->align(ALIGN_LEFT|ALIGN_INSIDE);
		}
		{
			InvisibleBox* o = new InvisibleBox(90, 20, 380, 16, command);
			o->labelfont(o->labelfont()->bold());
			o->align(ALIGN_LEFT|ALIGN_INSIDE);
		}
		{
			InvisibleBox* o = new InvisibleBox(60, 35, 380, 30, _("Please inform the authors of this program and provide the details below."));
			o->align(ALIGN_LEFT|ALIGN_INSIDE|ALIGN_WRAP);
		}
		{
			InvisibleBox* o = new InvisibleBox(15, 15, 35, 35, "");
			o->image(crash_pix);
		}
		{
			TextDisplay* o = backTraceTD = new TextDisplay(10, 110, 430, 275, "");
			o->hide();
			o->color(WHITE);
			o->textcolor(BLACK);
			o->buffer(gdbbuffer);
		}
		o->end();
	}
	w->show();
	flush();
	
	// Is there gdb on the system?
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));
	if (stat (GDBPATH, buf) != 0 || !get_me_gdb_output(pid))
		crashDetailsButton->deactivate();
	w->exec();
	
	return;
}


// --------------------------------------------
// Error message window
// --------------------------------------------

// This should be replaced with one of redesigned standard dialogs...

static xpmImage error_pix((const char **)error_xpm);

void cb_errOk(Widget *w) {
  w->window()->hide();
}

void errormessage(char *part1, char *part2, char *part3)
{
	Window* w;
	{
		Window* o = new Window(350, 100, _("Error"));
		w = o;
		o->shortcut(0xff1b);
		o->begin();
		{
			ReturnButton* o = new ReturnButton(250, 65, 90, 25, _("&OK"));
			o->callback((Callback*)cb_errOk); 
		}
		{
			InvisibleBox* o = new InvisibleBox(60, 5, 280, 16, part1);
			o->align(ALIGN_LEFT|ALIGN_INSIDE);
		}
		{
			InvisibleBox* o = new InvisibleBox(90, 20, 280, 16, part2);
			o->labelfont(o->labelfont()->bold());
			o->align(ALIGN_LEFT|ALIGN_INSIDE);
		}
		{
			InvisibleBox* o = new InvisibleBox(60, 35, 280, 30, part3);
			o->align(ALIGN_LEFT|ALIGN_INSIDE|ALIGN_WRAP);
		}
		{
			InvisibleBox* o = new InvisibleBox(15, 15, 35, 35, "");
			o->image(error_pix);
		}
		o->end();
	}
	w->exec();
	return;
}


// --------------------------------------------
// Depending on exit status, show some nice dialogs
// --------------------------------------------

void process_output_status(int exit_status, PtyProcess* child)
{
	char *messages1[257], *messages2[257];

	// FIXME: do we still need this init?
	for (int i=0;i<256;i++) { messages1[i] = ""; messages2[i] = ""; }
	
	if (exit_status == PtyProcess::Killed) exit_status = 256;

	messages1[127] = _("Program not found:");
	messages2[127] = _("Perhaps it is not installed properly. Check your $PATH value.");
//	messages1[14] = _("Segmentation fault in child process:");
//	messages2[14] = _("");
	messages1[126] = _("File is not executable:");
	messages2[126] = _("Is this really a program? If it is, you should check its permissions.");
	messages1[256] = _("Program was terminated:");
	messages2[256] = _("");

	if (exit_status == PtyProcess::Crashed) {
		// Nice bomb window
		crashmessage(cmd_,child->pid());
	} else if (!(messages1[exit_status] == "")) {
		// we have special message for this status
		errormessage(messages1[exit_status],cmd_,messages2[exit_status]);
        } else {
		fprintf(stderr, _("Elauncher: child's exited normally with status %d\n"), exit_status);

		if (exit_status>0) {
			// unknown status, display stdout & stderr
			char *buffer;
			char output[65535];

			bool changed=false;
			strcpy(output,"");
			while (buffer = child->readLine()) {
				strcat(output, buffer);
				changed=true;
			}
			if (changed) output_window(_("Program output"),output);
		}
	}
}


// --------------------------------------------
// Core function that handles su/sudo, waits for program to 
// finish and then calls the output handler
// --------------------------------------------

// this is our internal message:
#define CONTMSG "elauncher_ok_to_continue"
// these are part of sudo/su chat:
#define PWDQ "Password:"
#define BADPWD "/bin/su: incorrect password"
#define SUDOBADPWD "Sorry, try again."

// We can't use const char* because of strtok later
int start_child_process(char *cmd) 
{

	if (strlen(cmd)<1) return 0;
//	show_busy_screen(true);
//	return 0;

	// This is so that we can get a backtrace in case of crash
	struct rlimit *rlim = (struct rlimit*)malloc(sizeof(struct rlimit));
	getrlimit (RLIMIT_CORE, rlim);
	rlim_t old_rlimit = rlim->rlim_cur; // keep previous rlimit
	rlim->rlim_cur = RLIM_INFINITY;
	setrlimit (RLIMIT_CORE, rlim);

	// Prepare array as needed by exec()
	char *parts[4];
	if (param_root) {
		if (use_sudo) {
			parts[0] = "/bin/sudo";
			parts[1] = "";
		} else {
			parts[0] = "/bin/su";
			parts[1] = "-c";
		}
		// This "continue message" prevents accidentally exposing password
		int length = strlen("echo "CONTMSG)+strlen("; ")+strlen(cmd);
		parts[2] = (char*)malloc(length);
		snprintf(parts[2],length,"echo %s; %s",CONTMSG,cmd);
		parts[3] = NULL;
	} else { 
		parts[0] = "/bin/sh";
		parts[1] = "-c";
		parts[2] = strdup(cmd);
		parts[3] = NULL;
	}
	// the actual command is this:
	cmd_ = strtok(cmd," ");
	
tryagain:
	PtyProcess *child = new PtyProcess();
	child->setEnvironment((const char**)environ);
	if (child->exec(parts[0], (const char**)parts) < 0) {
		if (ask(_("Error starting program. Try again?")))
			goto tryagain;
		else
			return 0;
	}
	
	// Wait for process to actually start. Shouldn't last long
	while (1) {
		int p = child->pid();
		if (p != 0 && child->checkPid(p))
			break;
		int exit = child->checkPidExited(p);
		if (exit != -2) {
			// Process is DOA
			fprintf (stderr, "Elauncher: Process has died unexpectedly! Exit status: %d\n",exit);
			delete child;
			goto tryagain;
		}
		fprintf (stderr, "Elauncher: Not started yet...\n");
	}

	// Run program as root using su or sudo
	if (param_root) {
		char *line;

		const char *pwd = password(_("This program requires administrator privileges.\nPlease enter your password below:"));
		if (pwd == 0) { fprintf(stderr,"Canceled\n"); exit(1); }
		
		// Chat routine
		while (1) {
			line = child->readLine();
			
			// This covers other cases of failed process startup
			// Our su command should at least produce CONTMSG
			if (line == 0 && child->checkPidExited(child->pid()) != PtyProcess::NotExited) {
				// TODO: capture stdout? as in sudo error?
				fprintf (stderr, "Elauncher: su process has died unexpectedly in chat stage!\n");
				delete child;
				
				if (choice_alert (_("Failed to start authentication. Try again"), 0, _("Yes"), _("No")) == 2) return 0;
				goto tryagain;
			}
			
			if (strncasecmp(line,PWDQ,strlen(PWDQ))== 0)
				child->writeLine(pwd,true);

			if (strncasecmp(line,CONTMSG,strlen(CONTMSG)) == 0)
				break; // program starts...

			if ((strncasecmp(line,BADPWD,strlen(BADPWD)) == 0) || (strncasecmp(line,SUDOBADPWD,strlen(SUDOBADPWD)) == 0)) {
				 // end process
				child->waitForChild();
				delete child;
				
				if (choice_alert (_("The password is wrong. Try again?"), 0, _("Yes"), _("No")) == 2) return 0;

				goto tryagain;
			}
		}
	}
	
	// Wait for program to end, but don't lose the output
//	show_busy_screen(false);
//	cursor(CURSOR_ARROW);
	int child_val = child->runChild();
	process_output_status(child_val,child);
	
	// deallocate one string we mallocated 
	free(parts[2]);
	delete child;
	
	// Revert old rlimit
	rlim->rlim_cur = old_rlimit;
	setrlimit (RLIMIT_CORE, rlim);

	return 0;
}


// --------------------------------------------
// Analyze command and, if it's URL, call appropriate application
// Otherwise assume that it's executable and run it
// (Code mostly copied over from former eRun)
// --------------------------------------------


void run_resource(const char *cmd) {
	char pRun[256];
	char browser[256];

	// look up default browser in config
	Config pGlobalConfig(Config::find_file("ede.conf", 0));
	pGlobalConfig.get("Web", "Browser", browser, 0, sizeof(browser));
	if(pGlobalConfig.error() && !browser) {
		strncpy(browser, "mozilla", sizeof(browser));
	}

	// We might need this later, so try to optimize file reads
	pGlobalConfig.get("System","UseSudo", use_sudo, false);

	// split cmd to protocol and location
	char *protocol = strdup(cmd);
	char *location = strchr(protocol, ':');
	if (location) *(location++) = '\0';	// cut off at ':'

	// is cmd a proper URL?
	if((location) && (strchr(protocol, ' ') == NULL))
	{
		if (strcasecmp(protocol,"file") == 0)
		// use mimetypes
		{
			MimeType m(location);
			
			if (m.command())
				strncpy(pRun, m.command(), sizeof(pRun)-1);
			else 
			{	// unknown extension
				char m_printout[256];
				snprintf(m_printout, sizeof(m_printout)-1, _("Unknown file type:\n\t%s\nTo open this file in 'appname' please use\n 'appname %s'"), location, location);
				alert(m_printout);
				return;
			}
		}
		
		// TODO: split protocols into own file
		else if (strcasecmp(protocol, "http")==0 || strcasecmp(protocol, "ftp")==0)
		{
			snprintf(pRun, sizeof(pRun)-1, "%s %s", browser, cmd);
		}
		
		// search engine urls
		else if (strcasecmp(protocol, "gg")==0)
		{
			snprintf(pRun, sizeof(pRun)-1, "%s http://www.google.com/search?q=\"%s\"", browser, location);
		}
		
		else if (strcasecmp(protocol, "leo")==0)
		{
			snprintf(pRun, sizeof(pRun)-1, "%s http://dict.leo.org/?search=\"%s\"", browser, location);
		}
		
		else if (strcasecmp(protocol, "av")==0)
		{
			snprintf(pRun, sizeof(pRun)-1, "%s http://www.altavista.com/sites/search/web?q=\"%s\"", browser, location);
		}
		
		else	// Unkown URL type - let browser deal with it
		{
			snprintf(pRun, sizeof(pRun)-1, "%s %s", browser, cmd);
		}
	}
	else 
	// local executable 
	// TODO: parse the standard parameters to the executable if any exists in the *.desktop file.
	{
		if (param_secure) {
			char message[256];
			snprintf (message, sizeof(message)-1, _("You have requested to execute program %s via Elauncher. However, secure mode was enabled. Execution has been prevented."), cmd);
			alert (message);
			exit(1);
		} else {
			snprintf(pRun, sizeof(pRun)-1, "%s", cmd);
		}
	}
	delete [] protocol;
	
	// Additional parameters
	if (param_term) {
		char termapp[256];
		pGlobalConfig.get("Terminal", "Terminal", termapp, 0, sizeof(termapp));
		char tmp[256];
		snprintf (tmp, sizeof(pRun)-1, "%s -e %s",termapp,pRun);
		strcpy (pRun, tmp);
	}
	if (param_root) {
		// nothing special to do here
	}

	// continue with execution
	start_child_process(pRun);
}


// --------------------------------------------
// Draw GUI run dialog. This is shown when no parameters are given
// (Code mostly copied over from former eRun)
// --------------------------------------------

static xpmImage run_pix((const char **)run_xpm);
Window* windowRunDialog;
Input* inputRunDialog;
CheckButton* runAsRoot;

static void cb_OK(Button*, void*) {
	param_root = runAsRoot->value();
	windowRunDialog->hide();
	flush(); // Window will not hide without this...
	run_resource(inputRunDialog->value());
}

static void cb_Cancel(Button*, void*) {
	windowRunDialog->hide();
}

static void cb_Browse(Button*, void*) {
	const char *file_types = _("Executables (*.*), *, All files (*.*), *");
	const char *fileName = file_chooser(_("File selection..."), "*.*", inputRunDialog->value()); // TODO: fix file filter when we get a new dialog
	if (fileName) 
	{ 
		inputRunDialog->value(fileName);
	}
}

void run_dialog() {
	Window* w = windowRunDialog = new Window(350, 175, _("Open..."));
	w->when(WHEN_ENTER_KEY);
	w->begin();

	{ InvisibleBox* o = new InvisibleBox(5, 5, 55, 70);
	o->image(run_pix);
	o->align(ALIGN_CENTER|ALIGN_INSIDE); }
     
	{ InvisibleBox* o = new InvisibleBox(60, 5, 285, 70, _("Type the location you want to open or the name of the program you want to run\
. (Possible prefixes are: http:, ftp:, gg:, av:, leo:)"));
	o->align(132|ALIGN_INSIDE); }

	{ Input* o = inputRunDialog = new Input(60, 80, 180, 25, _("Open:"));
	o->align(132);
	//o->when(WHEN_ENTER_KEY); 
	}
     
	{ Button* o = new Button(250, 80, 90, 25, _("&Browse..."));
	o->callback((Callback*)cb_Browse); }

	{ CheckButton* o = runAsRoot = new CheckButton(60, 110, 90, 25, _("Run as &root"));
	 }

	{ ReturnButton* o = new ReturnButton(150, 140, 90, 25, _("&OK"));
	o->callback((Callback*)cb_OK); 
	o->shortcut(ReturnKey); // ENTER
	}
     
	{ Button* o = new Button(250, 140, 90, 25, _("&Cancel"));
	o->callback((Callback*)cb_Cancel); }
     
//	-- window manager should do this
//	w->x(fltk::xvisual->w / 2 - (w->w()/2));
//	w->y( (fltk::xvisual->h / 2) - (w->h()/2));
	w->end();
  
	w->show();
	run();
}


// Show console help on parameters

void showHelp() {
	printf ("ELauncher - ");
	printf (_("program and URL opener for EDE.\n"));
	printf ("Copyright (c) 2004,2005 EDE Authors\n");
	printf (_("Licenced under terms of GNU General Public Licence v2.0 or newer.\n\n"));
	printf (_("Usage:\n"));
	printf ("\telauncher [OPTIONS] [URL]\n");
	printf ("\telauncher [OPTIONS] [PROGRAM]\n\n");
	printf ("elauncher URL -\n");
	printf (_("\tParse URL in form protocol:address and open in appropriate program.\n\tURLs with protocol 'file' are opened based on their MIME type.\n"));
	printf ("elauncher PROGRAM -\n");
	printf (_("\tRun the program. If no path is given, look in $PATH. To give parameters\n\tto program, use quotes e.g.:\n"));
	printf ("\t\telauncher --term \"rm -rf /\"\n\n");
	printf (_("Options:\n"));
	printf ("  -h, --help\t- ");
	printf (_("This help screen.\n"));
	printf ("  --root\t- ");
	printf (_("Run as root. Dialog is opened to enter password.\n"));
	printf ("  --secure\t- ");
	printf (_("Prevent running programs. Only URLs are allowed.\n"));
	printf ("  --term\t- ");
	printf (_("Open in default terminal app.\n\n"));
}


// parse command line parameters

int main (int argc, char **argv) {
	char url[255];
	url[0] = '\0';

//	fl_init_locale_support("elauncher", PREFIX"/share/locale");

	for (int i=1; i<argc; i++) {
		// params
		if ((strcmp(argv[i],"--help") == 0) || (strcmp(argv[i],"-h") == 0)) {
			showHelp();
			exit(0);
		}
		else if (strcmp(argv[i],"--root") == 0) {
			param_root = true;
		}
		else if (strcmp(argv[i],"--secure") == 0) {
			param_secure = true;
		}
		else if (strcmp(argv[i],"--term") == 0) {
			param_term = true;
		}
		
		// someone is trying to run elauncher with elauncher
		else if (strcmp(argv[i],"elauncher") == 0 || strcmp(argv[i],PREFIX"/bin/elauncher") == 0) {
			// ignore
		}

		// there shouldn't be multiple url
		else if (url[0] == '\0') {
			strcpy (url, argv[i]);
		} else {
			fprintf (stderr, _("Elauncher: Wrong number of parameters...\n"));
			exit (1);
		}
	}
	
	if (url[0] == '\0') {
		run_dialog();
	} else {
		run_resource(url);
	}
}
