#ifndef __DESKTOPENTRY_H__
#define __DESKTOPENTRY_H__

#include <edelib/String.h>
#include <edelib/Debug.h>
#include <edelib/List.h>

EDELIB_NS_USING(String)
EDELIB_NS_USING(list)

class DesktopEntry;

typedef list<DesktopEntry*> DesktopEntryList;
typedef list<DesktopEntry*>::iterator DesktopEntryListIt;

typedef list<String> StrList;
typedef list<String>::iterator StrListIt;

/* Represents entry for a menu. Do not confuse it with DesktopFile from edelib */
class DesktopEntry {
private:
	/* used to load "later" found .desktop file, in case duplicate desktop file id's was found */
	unsigned int age;

	/* used for <OnlyUnallocated> and <NotOnlyUnallocated> */
	bool allocated;

	/* absolute path to .desktop file */
	String *path;

	/* Desktop File Id */
	String *id;

	/* Categories value from .desktop file; filled with load() */
	String *categories;

	/* Localized Name value from .desktop file; filled with load() */
	String *name;

	/* GenericName value from .desktop file; filled with load() */
	String *generic_name;

	/* Comment value from .desktop file; filled with load() */
	String *comment;

	/* Icon value from .desktop file; filled with load() */
	String *icon;

	/* Exec value from .desktop file; filled with load() */
	String *exec_cmd;

	/* tokenized 'categories' */
	StrList category_list;

	E_DISABLE_CLASS_COPY(DesktopEntry)
public:
	DesktopEntry() : age(0), allocated(false), path(NULL), id(NULL),
	categories(NULL), name(NULL), generic_name(NULL), comment(NULL), icon(NULL), exec_cmd(NULL) { }

	~DesktopEntry();

	/*
	 * Construct full path by using 'dir/p' and construct desktop file id according
	 * to the menu spec. If 'basedir' was given (can be NULL), 'dir' will be diffed against it
	 * so correct prefix for desktop file id can be calculated.
	 *
	 * The id is calculated like this:
	 *   $XDG_DATA_DIRS/applications/foo.desktop => foo.desktop
	 *   $XDG_DATA_DIRS/applications/ede/foo.desktop => ede-foo.desktop
	 *   $XDG_DATA_DIRS/applications/ede/menu/foo.desktop => ede-menu-foo.desktop
	 */
	void assign_path(const char *dir, const char *p , const char *basedir);

	/* loads actual .desktop file and reads it */
	bool load(void);

	/* check if Categories key contains given category */
	bool in_category(const char *cat);

	void mark_as_allocated(void) { allocated = true; }
	bool is_allocated(void) { return allocated; }

	const char   *get_path(void) { return path ? path->c_str() : NULL; }
	const char   *get_id(void)   { return id ? id->c_str() : NULL; }
	unsigned int  get_age(void)  { return age; }

	const char   *get_name(void)    { return name ? name->c_str() : NULL; }
	const char   *get_icon(void)    { return icon ? icon->c_str() : NULL; }
	const char   *get_exec(void)    { return exec_cmd ? exec_cmd->c_str() : NULL; }
	const char   *get_comment(void) { return comment ? comment->c_str() : NULL; }
};

/* remove duplicate items in the list, by looking at DesktopEntry id */
void desktop_entry_list_remove_duplicates(DesktopEntryList &lst);

/* call 'load()' on each member; if 'load()' fails, that member will be removed */
void desktop_entry_list_load_all(DesktopEntryList &lst);

/* sort a list of entries */
void desktop_entry_list_sort(DesktopEntryList &lst);

#endif
