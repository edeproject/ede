/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <stdlib.h>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Check_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl.H>

#include <edelib/String.h>
#include <edelib/StrUtil.h>
#include <edelib/List.h>
#include <edelib/Util.h>
#include <edelib/DesktopFile.h>
#include <edelib/Directory.h>
#include <edelib/Nls.h>
#include <edelib/Debug.h>
#include <edelib/Run.h>

#include "Autostart.h"
#include "icons/warning.xpm"

EDELIB_NS_USING(String)
EDELIB_NS_USING(DesktopFile)
EDELIB_NS_USING(list)
EDELIB_NS_USING(dir_list)
EDELIB_NS_USING(system_config_dirs)
EDELIB_NS_USING(user_config_dir)
EDELIB_NS_USING(str_ends)
EDELIB_NS_USING(run_async)

#ifdef DEBUG_AUTOSTART_RUN
 #define AUTOSTART_RUN(s) E_DEBUG("Executing %s\n", s)
#else
 #define AUTOSTART_RUN(s) run_async(s)
#endif

struct DialogEntry {
	String name;
	String exec;
};

typedef list<String>           StringList;
typedef list<String>::iterator StringListIter;

typedef list<DialogEntry*>                 DialogEntryList;
typedef list<DialogEntry*>::iterator       DialogEntryListIter;

static Fl_Window*        dialog_win;
static Fl_Check_Browser* cbrowser;
static Fl_Pixmap         warnpix((const char**)warning_xpm);

static char* get_basename(const char* path) {
	char* p = strrchr(path, '/');
	if(p) 
		return (p + 1);

	return (char*)path;
}

/*
 * 'Remove' duplicate entries by looking at their basename
 * (aka. filename, but ignoring directory path). Item is not actually removed from
 * the list (will mess up list pointers, but this can be resolved), but data it points
 * to is cleared, which is a sort of marker to caller to skip it. Dumb yes, but very simple.
 *
 * It will use brute force for lookup and 'removal' and (hopfully) it should not have 
 * a large impact on startup since, afaik, no one keeps hundreds of files in autostart 
 * directories (if keeps them, then that issue is not up to this program :-P).
 *
 * Alternative would be to sort items (by their basename) and apply consecutive unique on 
 * them, but... is it worth ?
 */
static void unique_by_basename(StringList& lst) {
	if(lst.empty())
		return;

	StringListIter first, last, first1, last1;
	first = lst.begin();
	last  = lst.end();

	if(first == last)
		return;

	const char* p1, *p2;
	for(; first != last; ++first) {
		for(first1 = lst.begin(), last1 = lst.end(); first1 != last1; ++first1) {
			p1 = (*first).c_str();
			p2 = (*first1).c_str();

			if(first != first1 && strcmp(get_basename(p1), get_basename(p2)) == 0)
				(*first1).clear();
		}
	}
}

static void entry_list_run_clear(DialogEntryList& l, bool run) {
	DialogEntryListIter dit = l.begin(), dit_end = l.end();
	for(; dit != dit_end; ++dit) {
		if(run)
			AUTOSTART_RUN((*dit)->exec.c_str());
		delete *dit;
	}

	l.clear();
}

static void dialog_runsel_cb(Fl_Widget*, void* e) {
	DialogEntryList* lst = (DialogEntryList*)e;
	E_ASSERT(lst->size() == (unsigned int)cbrowser->nitems() && "Size mismatch in local list and browser widget");

	DialogEntryListIter it = lst->begin();
	for(int i = 1; i <= cbrowser->nitems(); i++, ++it) {
		if(cbrowser->checked(i))
			AUTOSTART_RUN((*it)->exec.c_str());
	}

	dialog_win->hide();
	entry_list_run_clear(*lst, false);
}

static void dialog_runall_cb(Fl_Widget*, void* e) {
	DialogEntryList* lst = (DialogEntryList*)e;
	E_DEBUG("%i != %i\n", lst->size(), cbrowser->nitems());
	E_ASSERT(lst->size() == (unsigned int)cbrowser->nitems() && "Size mismatch in local list and browser widget");

	dialog_win->hide();
	entry_list_run_clear(*lst, true);
}

static void dialog_close_cb(Fl_Widget*, void* e) {
	dialog_win->hide();

	DialogEntryList* lst = (DialogEntryList*)e;
	entry_list_run_clear(*lst, false);
}

static void run_autostart_dialog(DialogEntryList& l) {
	DialogEntryList* ptr = (DialogEntryList*)&l;

	dialog_win = new Fl_Window(370, 305, _("Autostart warning"));
	dialog_win->begin();
		Fl_Box* img = new Fl_Box(10, 10, 65, 60);
		img->image(warnpix);
		Fl_Box* txt = new Fl_Box(80, 10, 280, 60, _("The following applications are registered for starting. Please chose what to do next"));
		txt->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_WRAP);
		cbrowser = new Fl_Check_Browser(10, 75, 350, 185);

		DialogEntryListIter it = l.begin(), it_end = l.end();
		for(; it != it_end; ++it)
			cbrowser->add((*it)->name.c_str());

		Fl_Button* rsel = new Fl_Button(45, 270, 125, 25, _("Run &selected"));
		rsel->callback(dialog_runsel_cb, ptr);

		Fl_Button* rall = new Fl_Button(175, 270, 90, 25, _("&Run all"));
		rall->callback(dialog_runall_cb, ptr);

		Fl_Button* cancel  = new Fl_Button(270, 270, 90, 25, _("&Cancel"));
		cancel->callback(dialog_close_cb, ptr);
		cancel->take_focus();
	dialog_win->end();
	dialog_win->show();

	while(dialog_win->shown())
		Fl::wait();
}

/*
 * This is implementation of Autostart Spec 
 * (http://standards.freedesktop.org/autostart-spec/autostart-spec-0.5.html).
 *
 * The Autostart Directories are $XDG_CONFIG_DIRS/autostart.
 * If the same filename is located under multiple Autostart Directories only the file under 
 * the most important directory should be used.
 *
 * Example: If $XDG_CONFIG_HOME is not set the Autostart Directory in the user's home directory 
 * is ~/.config/autostart/
 * Example: If $XDG_CONFIG_DIRS is not set the system wide Autostart Directory is /etc/xdg/autostart/
 * Example: If $XDG_CONFIG_HOME and $XDG_CONFIG_DIRS are not set and the two files 
 * /etc/xdg/autostart/foo.desktop and ~/.config/autostart/foo.desktop exist then only the file 
 * ~/.config/autostart/foo.desktop will be used because ~/.config/autostart/ is more important 
 * than /etc/xdg/autostart/.
 *
 * If Hidden key is set true in .desktop file, file MUST be ignored.
 * OnlyShowIn and NotShowIn (list of strings identifying desktop environments) if (or if not)
 * contains environment name, MUST not be started/not started.
 * TryExec is same as for .desktop spec.
 */
void perform_autostart(bool safe) {
	const char* autostart_dirname = "/autostart/";

	String adir = edelib::user_config_dir();
	adir += autostart_dirname;

	StringList dfiles, sysdirs, tmp;
	StringListIter it, it_end, tmp_it, tmp_it_end;

	dir_list(adir.c_str(), dfiles, true);

	system_config_dirs(sysdirs);
	if(!sysdirs.empty()) {
		for(it = sysdirs.begin(), it_end = sysdirs.end(); it != it_end; ++it) {
			*it += autostart_dirname;

			/*
			 * append content
			 * FIXME: too much of copying. There should be some way to merge list items
			 * probably via merge() member
			 */
			dir_list((*it).c_str(), tmp, true);
			for(tmp_it = tmp.begin(), tmp_it_end = tmp.end(); tmp_it != tmp_it_end; ++tmp_it)
				dfiles.push_back(*tmp_it);
		}
	}

	if(dfiles.empty())
		return;

	/*
	 * Remove duplicates where first one seen have priority to be keept. 
	 * This way is required by spec. 
	 *
	 * Also handle this case (noted in spec): 
	 * if $XDG_CONFIG_HOME/autostart/foo.desktop and $XDG_CONFIG_DIRS/autostart/foo.desktop
	 * exists, but $XDG_CONFIG_HOME/autostart/foo.desktop have 'Hidden = true',
	 * $XDG_CONFIG_DIRS/autostart/foo.autostart is ignored too.
	 *
	 * Latter is implied via unique_by_basename().
	 */
	unique_by_basename(dfiles);

	const char* name;
	char buff[1024];
	DesktopFile df;
	DialogEntryList entry_list;

	for(it = dfiles.begin(), it_end = dfiles.end(); it != it_end; ++it) {
		if((*it).empty())
			continue;

		name = (*it).c_str();

		if(!str_ends(name, ".desktop"))
			continue;

		if(!df.load(name)) {
			E_WARNING(E_STRLOC ": Can't load '%s'. Skipping...\n", name);
			continue;
		}
		
 		/* if Hidden key is set true in .desktop file, file MUST be ignored */
		if(df.hidden())
			continue;

		if(!(df.try_exec(buff, sizeof(buff)) || df.exec(buff, sizeof(buff))))
			continue;

		DialogEntry* en = new DialogEntry;
		en->exec = buff;

		/* figure out the name */
		if(df.name(buff, sizeof(buff)))
			en->name = buff;
		else
			en->name = name;

		entry_list.push_back(en);
	}

	if(safe)
		run_autostart_dialog(entry_list);
	else
		entry_list_run_clear(entry_list, true);
}
