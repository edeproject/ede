#include <string.h>
#include <FL/Fl_File_Chooser.H>
#include <edelib/Debug.h>
#include <edelib/File.h>
#include "AppChoice.h"

#define STR_CMP(s1, s2) (strcmp(s1, s2) == 0)

EDELIB_NS_USING(file_path)
EDELIB_NS_USING(String)

/* TODO: move to edelib */
static char* get_basename(const char* path) {
	char* p = (char*)strrchr(path, '/');
	if(p) 
		return (p + 1);

	return (char*)path;
}

static void select_cb(Fl_Widget *widget, void *c) {
	AppChoice *o = (AppChoice*)c;
	o->on_select();
}

AppChoice::AppChoice(int X, int Y, int W, int H, const char *l) : Fl_Choice(X, Y, W, H, l), known_apps(0), pvalue(0) {
	align(FL_ALIGN_TOP_LEFT);
	callback(select_cb, this);
}

void AppChoice::add_programs(KnownApp *lst) {
	E_RETURN_IF_FAIL(lst != 0);
	String path;

	add(_("None"));

	for(int i = 0; lst[i].name; i++) {
		/* first check if file exists in system */
		path = file_path(lst[i].cmd);
		if(path.empty())
			continue;

		add(lst[i].name);
	}

	add(_("Browse..."));
	known_apps = lst;
}

void AppChoice::add_if_user_program(const char *cmd) {
	E_RETURN_IF_FAIL(known_apps != 0);
	bool found = false;

	for(int i = 0; known_apps[i].name; i++) {
		if(STR_CMP(known_apps[i].cmd, cmd)) {
			found = true;
			break;
		}
	}

	if(found) return;

	user_val = cmd;
	replace(size() - 2, get_basename(cmd));
	add(_("Browse..."));
}

void AppChoice::select_by_cmd(const char *cmd) {
	E_RETURN_IF_FAIL(cmd != 0);
	E_RETURN_IF_FAIL(known_apps != 0);

	int pos;

	if(!user_val.empty()) {
		const char *b = get_basename(user_val.c_str());
		pos = find_item_index(b);
		if(pos >= 0) {
			value(pos);
			pvalue = pos;
			return;
		}
	}

	for(int i = 0; known_apps[i].name; i++) {
		if(STR_CMP(cmd, known_apps[i].cmd)) {
			/* now find menu entry with this name */
			pos = find_item_index(known_apps[i].name);	
			if(pos >= 0) {
				value(pos);
				pvalue = pos;
				return;
			}
		}
	}
}

const char *AppChoice::selected(void) {
	E_RETURN_VAL_IF_FAIL(known_apps != 0, 0);
	int s = value();

	/* skip first/last elements */
	if(s == 0 || s == (size() - 2))
		return 0;

	const char *n = text();

	/* first check if user one was selected */
	if(!user_val.empty() && STR_CMP(get_basename(user_val.c_str()), n))
		return user_val.c_str();

	for(int i = 0; known_apps[i].name; i++) {
		if(STR_CMP(known_apps[i].name, n))
			return known_apps[i].cmd;
	}

	return 0;
}

void AppChoice::on_select(void) {
	if(value() == (size() - 2)) {
		const char *p = fl_file_chooser(_("Choose program"), "*", 0);
		if(p) {
			/* set it to last place where is 'Browse...', and later move to the end */
			replace(size() - 2, get_basename(p));
		   	user_val = p;
			add(_("Browse..."));

			/* select it */
			value(size() - 3);
			return;
		}

		value(pvalue);
	}

	pvalue = value();
}

int AppChoice::find_item_index(const char *p) {
	E_RETURN_VAL_IF_FAIL(p != NULL, -1);

	const Fl_Menu_Item *m;
	for(int i = 0; i < size(); i++) {
		m = menu() + i;
		if(m->flags & FL_SUBMENU) {
			E_WARNING(E_STRLOC ": Submenus are not supported\n");
			continue;
		}

		if(!m->label()) continue;
		if(strcmp(m->label(), p) == 0) return i;
	}

	return -1;
}
