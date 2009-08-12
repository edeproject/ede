/*
 * $Id$
 *
 * ede-dialog, a dialog displayer
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2005-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <FL/Fl.h>
#include <edelib/MessageBox.h>
#include <edelib/Window.h>
#include <edelib/Nls.h>

EDELIB_NS_USING(MessageBox)
EDELIB_NS_USING(MessageBoxType)
EDELIB_NS_USING(MessageBoxIconType)
EDELIB_NS_USING(MSGBOX_PLAIN)
EDELIB_NS_USING(MSGBOX_INPUT)
EDELIB_NS_USING(MSGBOX_INPUT_SECRET)
EDELIB_NS_USING(MSGBOX_ICON_TYPE_INFO)
EDELIB_NS_USING(MSGBOX_ICON_TYPE_ALERT)
EDELIB_NS_USING(MSGBOX_ICON_TYPE_QUESTION)
EDELIB_NS_USING(MSGBOX_ICON_TYPE_INPUT)
EDELIB_NS_USING(MSGBOX_ICON_TYPE_PASSWORD)

EDELIB_NS_USING_AS(Window, EdelibWindow)

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))
#define CHECK_ARGV_LONG(argv, plong)    (strcmp(argv, plong) == 0)

/* kdialog returns this in the case of error and I'm using it too */
#define EDE_DIALOG_ERROR_RET 254

static MessageBox *mbox;

enum {
	OPT_NONE,
	OPT_YESNO,
	OPT_YESNOCANCEL,
	OPT_ERROR,
	OPT_SORRY,
	OPT_MSGBOX,
	OPT_INPUTBOX,
	OPT_PASSWORD
};

static const char* next_param(int curr, char **argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

static void help(void) {
	puts("Usage: ede-dialog [OPTIONS]");
	puts("Display a message in a window from shell scripts");
	puts("");
	puts("Options:");
	puts("  -h, --help            this help");
	puts("  --yesno [TEXT]        question message with yes/no buttons");
	puts("  --yesnocancel [TEXT]  question message with yes/no/cancel buttons");
	puts("  --error [TEXT]        error message dialog");
	puts("  --sorry [TEXT]        sorry message dialog");
	puts("  --msgbox [TEXT]       message dialog");
	puts("  --inputbox [TEXT]     dialog with input box");
	puts("  --password [TEXT]     dialog with input box for password input (text will be hidden)");
	puts("  --title [TEXT]        set dialog title");
}

int main(int argc, char **argv) {
	const char *title, *txt;
	int         opt = OPT_NONE;
	int         ret = EDE_DIALOG_ERROR_RET;
	int         nbuttons = 0;

	MessageBoxType     mtype = MSGBOX_PLAIN;
	MessageBoxIconType itype = MSGBOX_ICON_TYPE_INFO;

	title = txt = NULL;

	if(argc <= 1) {
		help();
		return EDE_DIALOG_ERROR_RET;
	}

	for(int i = 1; i < argc; i++) {
		if(CHECK_ARGV(argv[i], "-h", "--help")) {
			help();
			return EDE_DIALOG_ERROR_RET;
		} 

		/* optional flags */
		if(CHECK_ARGV_LONG(argv[i], "--title")) {
			title = next_param(i, argv, argc);
			if(!title) {
				puts("'title' option requires a parameter");
				return EDE_DIALOG_ERROR_RET;
			}
			
			i++;
			continue;
		} 
		
		/* mandatory flags */
		if(CHECK_ARGV_LONG(argv[i], "--yesno")) {
			itype = MSGBOX_ICON_TYPE_QUESTION;
			opt = OPT_YESNO;
		} else if(CHECK_ARGV_LONG(argv[i], "--yesnocancel")) {
			itype = MSGBOX_ICON_TYPE_QUESTION;
			opt = OPT_YESNOCANCEL;
		} else if(CHECK_ARGV_LONG(argv[i], "--error")) {
			itype = MSGBOX_ICON_TYPE_ALERT;
			opt = OPT_ERROR;
		} else if(CHECK_ARGV_LONG(argv[i], "--sorry")) {
			itype = MSGBOX_ICON_TYPE_ALERT;
			opt = OPT_SORRY;
		} else if(CHECK_ARGV_LONG(argv[i], "--msgbox")) {
			opt = OPT_MSGBOX;
			/* for else do nothing; use default values */
		} else if(CHECK_ARGV_LONG(argv[i], "--inputbox")) {
			itype = MSGBOX_ICON_TYPE_INPUT;
			mtype = MSGBOX_INPUT;
			opt   = OPT_INPUTBOX;
		} else if(CHECK_ARGV_LONG(argv[i], "--password")) {
			itype = MSGBOX_ICON_TYPE_PASSWORD;
			mtype = MSGBOX_INPUT_SECRET;
			opt   = OPT_PASSWORD;
		} else {
			printf("Unknown '%s' parameter\n", argv[i]);
			return EDE_DIALOG_ERROR_RET;
		}

		/* every above option requres additional parameter */
		txt = next_param(i, argv, argc);
		if(!txt) {
			printf("'%s' option requires a parameter\n", argv[i]);
			return EDE_DIALOG_ERROR_RET;
		}

		/* skip parameter */
		i++;
	}

	if(opt == OPT_NONE) {
		puts("Missing one of the flags that will describe the dialog. Run program with '--help' for the details");
		return EDE_DIALOG_ERROR_RET;
	}

	/* 
	 * Use a trick to load icon theme and colors using xsettings stuff. edelib::Window will load them
	 * and fill static FLTK values; every further window will use those values, including our will use them
	 *
	 * TODO: this hack needs appropriate solution in edelib.
	 */
	EdelibWindow *win = new EdelibWindow(-100, -100, 0, 0);
	win->show();
	Fl::wait(0.2);
	win->hide();

	mbox = new MessageBox(mtype);
	mbox->set_icon_from_type(itype);
	mbox->set_text(txt);
	mbox->label(title);

	/* determine buttons */
	switch(opt) {
		case OPT_YESNO:
			mbox->add_button(_("&No"));
			mbox->add_button(_("&Yes"));
			nbuttons = 2;
			break;
		case OPT_YESNOCANCEL:
			mbox->add_button(_("&Cancel"));
			mbox->add_button(_("&No"));
			mbox->add_button(_("&Yes"));
			nbuttons = 3;
			break;
		case OPT_ERROR:
		case OPT_MSGBOX:
		case OPT_SORRY:
			mbox->add_button(_("&Close"));
			nbuttons = 1;
			break;
		case OPT_INPUTBOX:
		case OPT_PASSWORD:
			mbox->add_button(_("&Cancel"));
			mbox->add_button(_("&OK"));
			nbuttons = 2;
			break;
	}

	ret = mbox->run();

	/* check box type and 'Cancel' wasn't present */
	if((opt == OPT_INPUTBOX || opt == OPT_PASSWORD) && ret != 0)
		printf("%s\n", mbox->get_input());

	/* 
	 * Now emulate kdialog return values; e.g. 'OK' will be 0 and 'Cancel' will be 1 if two buttons
	 * are exists. Because MessageBox already do this in reversed way, here value is just re-reversed again if
	 * we have more that one button.
	 */
	if(nbuttons > 1) {
		/* xor-ing with 2 is different so button number can't be decreased */
		if(nbuttons == 3) {
			ret ^= nbuttons;
			ret -= 1;
		} else {
			ret ^= (nbuttons - 1);
		}
	}

	return ret;
}
