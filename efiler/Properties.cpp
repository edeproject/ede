/*
 * $Id$ *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


// File properties window

#include "Properties.h"

#include <stdlib.h> // for free()
#include <stdio.h> // for snprintf()
#include <sys/stat.h> // for stat
#include <unistd.h> // for getuid()
#include <sys/types.h> // for getuid()
#include <pwd.h> // for getpwent()
#include <grp.h> // for getgrent()

#include <edelib/Nls.h>

#include <FL/Fl_Tabs.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/filename.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>

#include "Util.h"

#define DIALOG_WIDTH 320
#define DIALOG_HEIGHT 290


void properties_cancel_cb(Fl_Widget*w, void*) {
	Fl_Window* win = (Fl_Window*)w->parent();
	win->hide();
}

void properties_ok_cb(Fl_Widget*w, void*i) {
}


Properties::Properties(const char* file)  : edelib::Window(DIALOG_WIDTH, DIALOG_HEIGHT, _("File properties")), _file(0) {
	_file=file;
	set_modal();

	Fl_Tabs *tabs;
	Fl_Group *file_tab, *perms_tab, *progs_tab;
	Fl_Button *ok_but, *canc_but;

	// Create path from filename
	int k=0;
	char filename[FL_PATH_MAX], filepath[FL_PATH_MAX];
	strncpy(filename, file, FL_PATH_MAX-1);

	// Fix bug in fl_filename_name which returns nothing for directories
	if (filename[strlen(filename)-1] == '/') {
		filename[strlen(filename)-1]='\0';
		k--;
	}
	const char *tmp = fl_filename_name(filename);
	strcpy(filename, tmp);

	k+=strlen(file)-strlen(filename);
	if (k>FL_PATH_MAX-1) k=FL_PATH_MAX-1;
	strncpy(filepath, file, k);
	filepath[k]='\0';


	// Init mimetype
	mime_resolver.set(file);

	// Stat for current file
	bool stat_succeeded = false; // shouldn't happen
	struct stat64 stat_buffer;

	const char* owner_name, *group_name;
	bool can_rename=false, can_chown=false, can_chmod=false;
	int is_executable=0, n_owner=0, n_group=0, n_others=0;
	char mode_nr[5];

	int owner_uid=0, owner_gid=0;

	int user_uid=getuid(), user_gid=getgid();
	if (user_uid==0) can_chown=true; // FIXME

	if (!stat64(file,&stat_buffer)) { 
		stat_succeeded = true;
		owner_uid=stat_buffer.st_uid;
		owner_gid=stat_buffer.st_gid;
		if (owner_uid==user_uid || owner_gid==user_gid) can_chmod=true;

		// Read modes
		mode_t mode = stat_buffer.st_mode;
		int m0=0, m1=0, m2=0, m3=0;
		if (mode&S_ISUID) m0+=4; if (mode&S_ISGID) m0+=2; if (mode&S_ISVTX) m0+=1;
		if (mode&S_IRUSR) m1+=4; if (mode&S_IWUSR) m1+=2; if (mode&S_IXUSR) m1+=1;
		if (mode&S_IRGRP) m2+=4; if (mode&S_IWGRP) m2+=2; if (mode&S_IXGRP) m2+=1;
		if (mode&S_IROTH) m3+=4; if (mode&S_IWOTH) m3+=2; if (mode&S_IXOTH) m3+=1;
		snprintf(mode_nr, 5, "%d%d%d%d", m0, m1, m2, m3);

		// Simple modes
		if (mode&S_IXUSR) is_executable=1;
		if (mode&S_IWUSR) n_owner=0;
		else if (mode&S_IRUSR) n_owner=1;
		else n_owner=2;
		if (mode&S_IWGRP) n_group=0;
		else if (mode&S_IRGRP) n_group=1;
		else n_group=2;
		if (mode&S_IWOTH) n_others=0;
		else if (mode&S_IROTH) n_others=1;
		else n_others=2;
	}

	// Stat for directory the file resides in
	// Needed to see if user can rename file
	if (!stat64(filepath,&stat_buffer)) { 
		if (stat_buffer.st_uid==user_uid || stat_buffer.st_gid==user_gid)
			can_rename=true;
	}

	// List of users
	char *users_list[65535];
	for (int i=0; i<65535; i++) users_list[i]=0;
	struct passwd *pwent;
	setpwent();
	while (pwent = getpwent()) {
		if (strcmp(pwent->pw_gecos, pwent->pw_name)==0 || strlen(pwent->pw_gecos)<2) // just one char
			users_list[pwent->pw_uid] = strdup(pwent->pw_name);
		else
			users_list[pwent->pw_uid] = tasprintf("%s (%s)", pwent->pw_gecos, pwent->pw_name);
	}
	endpwent();

	// List of groups
	char *groups_list[65535];
	for (int i=0; i<65535; i++) groups_list[i]=0;
	struct group *grent;
	setgrent();
	while (grent = getgrent()) {
		groups_list[grent->gr_gid] = strdup(grent->gr_name);
	}
	endgrent();

	// Window design
	begin();
	tabs = new Fl_Tabs(0, 2, w(), h()-37);
	tabs->begin();
		file_tab = new Fl_Group(0, 25, w(), h()-60, _("&File"));
		file_tab->begin();
			// Icon
			if(edelib::IconTheme::inited()) {
				Fl_Box *img = new Fl_Box(10, 35, 65, 65);
				//img->box(FL_DOWN_BOX);
				edelib::String icon = edelib::IconTheme::get(mime_resolver.icon_name().c_str(), edelib::ICON_SIZE_HUGE);
				if (icon=="") icon = edelib::IconTheme::get("empty", edelib::ICON_SIZE_HUGE, edelib::ICON_CONTEXT_MIMETYPE);
				Fl_Image *i = Fl_Shared_Image::get(icon.c_str());
				if(i) img->image(i);
			} else {
				// Icon theme doesn't work!?
			}

			// Filename
			{
				Fl_Input *fn = new Fl_Input (135, 35, w()-145, 20, _("Name:"));
				if (!can_rename) {
					// filename->deactivate();
					// this looks prettier
					fn->readonly(1);
					fn->color(FL_BACKGROUND_COLOR);
				}
				fn->value(filename);
			}

			// Directory
			{
				Fl_Input *fp = new Fl_Input (135, 65, w()-145, 20, _("Location:"));
				fp->readonly(1);
				fp->color(FL_BACKGROUND_COLOR);
				fp->value(filepath);
			}

			// Size
			{
				Fl_Box *sizelabel = new Fl_Box (75, 95, 60, 20, _("Size:"));
				sizelabel->align (FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

				Fl_Box *sizebox = new Fl_Box (135, 95, w()-145, 20);
				sizebox->align (FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				if (stat_succeeded) sizebox->copy_label(nice_size(stat_buffer.st_size));
			}

			// Type
			{
				Fl_Box *typelabel = new Fl_Box (10, 125, 60, 20, _("Type:"));
				typelabel->align (FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

				Fl_Box *typebox = new Fl_Box (70, 125, w()-80, 20);
				typebox->align (FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				typebox->copy_label(mime_resolver.type().c_str());

				Fl_Box *ntypebox = new Fl_Box (70, 145, w()-80, 20);
				ntypebox->align (FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
				ntypebox->copy_label(mime_resolver.comment().c_str());
			}
		file_tab->end();

		perms_tab = new Fl_Group(0, 25, w(), h()-60, _("&Permissions"));
		perms_tab->begin();
			// Owner
			{
				Fl_Choice *owner = new Fl_Choice (70, 35, w()-80, 20, _("Owner:"));
				int j=-1, selected=0;
				for (int i=0; i<65535; i++)
					if (users_list[i]!=0) {
						owner->add(users_list[i]);
						free(users_list[i]);
						j++;
						if (i==owner_uid) selected=j;
					}
				owner->value(selected);
				if (!can_chown) owner->deactivate();
			}

			// Group
			{
				Fl_Choice *group = new Fl_Choice (70, 65, w()-80, 20, _("Group:"));
				int j=-1, selected=0;
				for (int i=0; i<65535; i++)
					if (groups_list[i]!=0) {
						group->add(groups_list[i]);
						free(groups_list[i]);
						j++;
						if (i==owner_gid) selected=j;
					}
				group->value(selected);
				if (!can_chown) group->deactivate();
			}

			// Executable
			{
				Fl_Check_Button *exec = new Fl_Check_Button (10, 95, w()-20, 20, _("File is an executable program"));
				if (stat_succeeded) exec->value(is_executable);
				if (!can_chmod) exec->deactivate();
			}

			// Owner permissions
			{
				Fl_Choice *ownerperms = new Fl_Choice (135, 125, w()-145, 20, _("Owner permissions:"));
				ownerperms->add(_("Read and write"));
				ownerperms->add(_("Read only"));
				ownerperms->add(_("Not allowed"));
				if (stat_succeeded) ownerperms->value(n_owner);
				if (!can_chmod) ownerperms->deactivate();
			}

			// Group permissions
			{
				Fl_Choice *groupperms = new Fl_Choice (135, 155, w()-145, 20, _("Group permissions:"));
				groupperms->add(_("Read and write"));
				groupperms->add(_("Read only"));
				groupperms->add(_("Not allowed"));
				if (stat_succeeded) groupperms->value(n_group);
				if (!can_chmod) groupperms->deactivate();
			}

			// Others permissions
			{
				Fl_Choice *otherperms = new Fl_Choice (135, 185, w()-145, 20, _("Others permissions:"));
				otherperms->add(_("Read and write"));
				otherperms->add(_("Read only"));
				otherperms->add(_("Not allowed"));
				if (stat_succeeded) otherperms->value(n_others);
				if (!can_chmod) otherperms->deactivate();
			}

			// Number
			// TODO: move to advanced window
			{
				Fl_Input *modenr = new Fl_Input (135, 215, 50, 20, _("Mode number:"));
				if (stat_succeeded) modenr->value(mode_nr);
				if (!can_chmod) modenr->deactivate();
			}
		perms_tab->end();

		progs_tab = new Fl_Group(0, 25, w(), h()-60, _("P&rograms"));
		progs_tab->begin();
		progs_tab->end();
	tabs->end();

	// Ok & Cancel buttons
	ok_but = new Fl_Return_Button(w()-200, h()-30, 90, 25, "&Ok"); 
	canc_but = new Fl_Button(w()-100, h()-30, 90, 25, "&Cancel");
	ok_but->callback(properties_ok_cb);
	canc_but->callback(properties_cancel_cb);
	end();
}
