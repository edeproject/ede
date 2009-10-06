#include <string.h>

#include <edelib/Util.h>
#include <edelib/StrUtil.h>
#include <edelib/Directory.h>
#include <edelib/DesktopFile.h>

#include "DesktopEntry.h"

EDELIB_NS_USING(DesktopFile)
EDELIB_NS_USING(build_filename)
EDELIB_NS_USING(stringtok)
EDELIB_NS_USING(DESK_FILE_TYPE_APPLICATION)

static int age_counter = 1;

DesktopEntry::~DesktopEntry() {
	delete path;
	delete id;

	delete categories;
	delete name;
	delete generic_name;
	delete comment;
	delete icon;
	delete exec;
}

void DesktopEntry::assign_path(const char *dir, const char *p, const char *basedir) {
	E_ASSERT(dir != NULL);
	E_ASSERT(p != NULL);

	/* make sure we do not assign things twice */
	E_RETURN_IF_FAIL(path == NULL);
	E_RETURN_IF_FAIL(id == NULL);

	const char *ptr;
	String *spath, *sid;

	/* full path */
	spath = new String(build_filename(dir, p));
	ptr = spath->c_str();

	/* if we have basedir, skip directory path by jumping the size of basedir */
	if(basedir) {
		ptr += strlen(basedir);

		/* skip ending delimiters */
		while(*ptr == E_DIR_SEPARATOR)
			ptr++;
	}

	sid = new String(ptr);

	/* replace existing separators; this is in the spec */
	sid->replace(E_DIR_SEPARATOR, '-');

	path = spath;
	id   = sid;
	age  = age_counter++;
}

bool DesktopEntry::load(void) {
	E_RETURN_VAL_IF_FAIL(path != NULL, false);

	DesktopFile df;
	if(!df.load(path->c_str())) {
		E_WARNING(E_STRLOC ": Unable to load %s\n", path->c_str());
		return false;
	}

	/* check if we are 'Hidden = true' or 'NoDisplay = true'; these must not be loaded */
	if(df.hidden() || df.no_display())
		return false;

	char buf[128];

	/* should it be shown in EDE */
	if(df.only_show_in(buf, sizeof(buf))) {
		if(strstr(buf, "EDE") == NULL)
			return false;
	}

	if(df.not_show_in(buf, sizeof(buf))) {
		if(strstr(buf, "EDE") != NULL)
			return false;
	}

	/* it must be application type */
	E_RETURN_VAL_IF_FAIL(df.type() == DESK_FILE_TYPE_APPLICATION, false);

	/* name must be present too */
	E_RETURN_VAL_IF_FAIL(df.name(buf, sizeof(buf)) == true, false);
	name = new String(buf);

	/* optional */
	if(df.get("Desktop Entry", "Categories", buf, sizeof(buf)))
		categories = new String(buf);

	if(df.generic_name(buf, sizeof(buf)))
		generic_name = new String(buf);

	if(df.comment(buf, sizeof(buf)))
		comment = new String(buf);

	if(df.icon(buf, sizeof(buf)))
		icon = new String(buf);

	if(df.exec(buf, sizeof(buf)))
		exec = new String(buf);

	return true;
}

bool DesktopEntry::in_category(const char *cat) {
	E_RETURN_VAL_IF_FAIL(cat != NULL, false);

	if(!categories)
		return false;

	StrListIt it, it_end;

	if(category_list.empty()) {
		stringtok(category_list, *categories, ";");

		/* remove starting/ending spaces if exists */
		it = category_list.begin();
		it_end = category_list.end();
		for(; it != it_end; ++it)
			(*it).trim();
	}

	it = category_list.begin();
	it_end = category_list.end();
	for(; it != it_end; ++it) {
		if((*it) == cat)
			return true;
	}

	return false;
}

/* TODO: bug in edelib */
static bool id_age_sorter(DesktopEntry* const& u1, DesktopEntry* const& u2) {
	return (strcmp(u1->get_id(), u2->get_id()) < 0) && (u1->get_age() < u2->get_age()); 
}

/* TODO: bug in edelib */
static bool name_sorter(DesktopEntry* const& u1, DesktopEntry* const& u2) {
	return (strcmp(u1->get_name(), u2->get_name()) < 0);
}

void desktop_entry_list_remove_duplicates(DesktopEntryList &lst) {
	if(lst.empty())
		return;

	/* sort them respecting name and age */
	lst.sort(id_age_sorter);

	DesktopEntryListIt it = lst.begin(), it_end = lst.end();
	DesktopEntryListIt next = it;

	/* do consecutive unique */
	while(++next != it_end) {
		if(strcmp((*it)->get_id(), (*next)->get_id()) == 0) {
			delete *next;
			lst.erase(next);
		} else {
			it = next;
		}

		next = it;
	}
}

void desktop_entry_list_load_all(DesktopEntryList &lst) {
	if(lst.empty())
		return;

	DesktopEntryListIt it = lst.begin(), it_end = lst.end();
	while(it != it_end) {
		if(!(*it)->load()) {
			delete *it;
			it = lst.erase(it);
		} else {
			++it;
		}
	}
}

void desktop_entry_list_sort(DesktopEntryList &lst) {
	lst.sort(name_sorter);
}
