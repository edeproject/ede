// EDialog - copyleft (c) Vedran Ljubovic 2005
// This program is licenced under GNU General Public License v2 or greater


#include <fltk/ask.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace fltk;


// --- compat. modes enum

enum {
	KDIALOGMODE = 0
};


// Common functions

void errormsg(char* msg) {
	fprintf (stderr, "edialog: %s\n", msg);
	exit(1);
}


void showhelp() {
	printf ("edialog - Show dialogs using FLTK2\n");
	printf ("Copyright (c) Vedran Ljubovic 2005\n");
	printf ("This program is licensed under GNU General Public License v2 or greater\n\n");
	printf ("Displays a dialog box. Return value corresponds to button pressed (e.g 0 = Ok, 1 = Cancel...)\n\n");
	printf ("Options:\n");
	printf ("    --kdialog  -  kdialog compatibility mode (default)\n");
	printf ("    (see kdialog --help for list)\n");
	exit(0);
}


// Functions for dialogs

void YesNo(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	exit(ask(param));
}

void YesNoCancel(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	exit(choice(param,yes,no,cancel));
}

void WarningYesNo(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	exit(choice_alert(param,yes,no,""));
}

void WarningContinueCancel(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	exit(choice_alert(param,"Continue",cancel,0));
}

void WarningYesNoCancel(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	exit(choice_alert(param,yes,no,cancel));
}

void Sorry(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	alert(param);
	exit(0);
}

void Error(char* param) {
	errormsg("Not implemented yet.");
	// Displays a red X and plays "error" sound

//	if (param[0] == '\0') errormsg ("Required parameter missing.");
//	alert(param);
//	exit(0);
}

void MsgBox(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	message(param);
	exit(0);
}

void InputBox(char* param) {
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	char *title = strtok(param," ");
	char *defval = strtok(NULL," ");
	printf ("%s\n",input(title,defval));
	exit(0);
}

void Password(char* param) {
	// NOTE: kdialog doesn't support default value for password
	// possibly for security reasons?
	if (param[0] == '\0') errormsg ("Required parameter missing.");
	char *title = strtok(param," ");
	char *defval = strtok(NULL," ");
	printf ("%s\n",password(title,defval));
	exit(0);
}

void TextBox(char* param) {
	errormsg("Not implemented yet.");
}

void ComboBox(char* param) {
	errormsg("Not implemented yet.");
}

void Menu(char* param) {
	errormsg("Not implemented yet.");
}

void CheckList(char* param) {
	errormsg("Not implemented yet.");
}

void RadioList(char* param) {
	errormsg("Not implemented yet.");
}

void PassivePopup(char* param) {
	errormsg("Not implemented yet.");
}

void GetOpenFilename(char* param) {
	errormsg("Not implemented yet.");
}

void GetSaveFilename(char* param) {
	errormsg("Not implemented yet.");
}

void GetExistingDirectory(char* param) {
	errormsg("Not implemented yet.");
}

void GetOpenUrl(char* param) {
	errormsg("Not implemented yet.");
}

void GetSaveUrl(char* param) {
	errormsg("Not implemented yet.");
}

void GetIcon(char* param) {
	errormsg("Not implemented yet.");
}

void ProgressBar(char* param) {
	errormsg("Not implemented yet.");
}



// ----------- These are charts for various compatibility modes

struct paramslist {
	char* option;
	void (*func)(char*);
};

// kdialog - KDE dialog
paramslist kdialogopts[] = {
	{"--yesno", 		YesNo},
	{"--yesnocancel", 	YesNoCancel},
	{"--warningyesno", 	WarningYesNo},
	{"--warningcontinuecancel", WarningContinueCancel},
	{"--warningyesnocancel", WarningYesNoCancel},
	{"--sorry", 		Sorry},
	{"--error", 		Error},
	{"--msgbox", 		MsgBox},
	{"--inputbox", 		InputBox},
	{"--password", 		Password},
	{"--textbox", 		TextBox},
	{"--combobox", 		ComboBox},
	{"--menu", 		Menu},
	{"--checklist", 	CheckList},
	{"--radiolist", 	RadioList},
	{"--passivepopup", 	PassivePopup},
	{"--getopenfilename", 	GetOpenFilename},
	{"--getsavefilename", 	GetSaveFilename},
	{"--getexistingdirectory", GetExistingDirectory},
	{"--getopenurl", 	GetOpenUrl},
	{"--getsaveurl", 	GetSaveUrl},
	{"--geticon", 		GetIcon},
	{"--progressbar", 	ProgressBar},
	{""}
};


// parse command line parameters
int main (int argc, char **argv) {
	int compat_mode = KDIALOGMODE;
	bool param_recognized[100];
	for (int i=0;i<100;i++) param_recognized[i]=false;

	// Switches and modifiers
	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i],"--kdialog") == 0) {
			compat_mode = KDIALOGMODE;
			param_recognized[i]=true;
		}
	}

	// Dialogs - only one dialog can be shown, so as soon as
	// we recognize parameter, we exit()
	paramslist* ptr;
	if (compat_mode == KDIALOGMODE) ptr = kdialogopts;
	for (int i=1; i<argc; i++) {
		while (ptr->option[0] != '\0') {
			if (strcmp(argv[i],ptr->option) == 0) {
				char *params = strdup("");
				i++;
				while ((i<argc) && (argv[i][0] != '-') && (argv[i][1] != '-')) {
					params = (char*) realloc(params, strlen(params)+strlen(argv[i])+2);
					params = strcat(params,argv[i++]);
					params = strcat(params," ");
				}
				ptr->func(params);
			}
			ptr++;
		}
		// function should exit()
		// so we can get here only if parameter isn't recognized
		if (param_recognized[i] == false) {
			showhelp();
		}
	}
	
	// No parameters passed or just modifiers
	showhelp();
}
