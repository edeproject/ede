/*
 * $Id$
 *
 * Copyright (C) 2012 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <FL/Fl_Shared_Image.H>
#include <edelib/TiXml.h>
#include <edelib/Debug.h>
#include <edelib/StrUtil.h>
#include <edelib/Util.h>
#include <edelib/FileTest.h>
#include <edelib/Directory.h>
#include <edelib/DesktopFile.h>
#include <edelib/IconLoader.h>
#include <edelib/Nls.h>
#include <edelib/Run.h>

#include "DesktopEntry.h"
#include "MenuRules.h"
#include "XdgMenuReader.h"

EDELIB_NS_USING(DesktopFile)
EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(system_config_dirs)
EDELIB_NS_USING(system_data_dirs)
EDELIB_NS_USING(user_data_dir)
EDELIB_NS_USING(build_filename)
EDELIB_NS_USING(file_test)
EDELIB_NS_USING(str_ends)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(FILE_TEST_IS_DIR)
EDELIB_NS_USING(DESK_FILE_TYPE_DIRECTORY)
EDELIB_NS_USING(ICON_SIZE_SMALL)

#define DOT_OR_DOTDOT(base)    (base[0] == '.' && (base[1] == '\0' || (base[1] == '.' && base[2] == '\0')))
#define ELEMENT_IS(elem, val)  (strcmp(elem->Value(), val) == 0)
#define ELEMENT_GET_TEXT(elem) (elem->FirstChild() ? elem->FirstChild()->ToText() : NULL)

/* max. name size */
#define NAME_BUFSZ 128

/* do not allow empty menus to be shown */
#define NO_EMPTY_MENUS 1

/* in FLTK, a default size */
extern int FL_NORMAL_SIZE;

struct MenuParseContext;
struct MenuContext;

typedef list<MenuParseContext*> MenuParseList;
typedef list<MenuParseContext*>::iterator MenuParseListIt;

typedef list<MenuContext*> MenuContextList;
typedef list<MenuContext*>::iterator MenuContextListIt;

struct MenuParseContext {
	/* for <Deleted> <NotDeled> tags */
	bool deleted;

	/* for <OnlyUnallocated> and <NotOnlyUnallocated> (default) */
	bool only_unallocated;

	/* menu tag content */
	String *name;

	/* a stack of .directory names; the top one is used */
	StrList dir_files;

	/* directories where to find .directory file */
	StrList dir_dirs;

	/* a list of .desktop files */
	DesktopEntryList desk_files;

	/* include rules */
	MenuRulesList include_rules;

	/* exclude rules */
	MenuRulesList exclude_rules;

	/* nested menus */
	MenuParseList submenus;
};

struct MenuContext {
	/* menu label */
	String *name;

	/* should this entry be displayed */
	bool display_it;

	/* menu icon */
	String *icon;

	/* a list of .desktop files; at the same time also items in menu list */
	DesktopEntryList items;

	/* nested menus */
	MenuContextList submenus;
};

/* TODO: bug in edelib */
static bool menu_context_sorter(MenuContext* const& c1, MenuContext* const& c2) {
	return *(c1->name) < *(c2->name);
}

static MenuParseContext *menu_parse_context_new(void) {
	MenuParseContext *m = new MenuParseContext;
	m->name = NULL;
	m->deleted = false;
	m->only_unallocated = false;
	return m;
}

static void menu_parse_context_delete(MenuParseContext *m) {
	E_RETURN_IF_FAIL(m != NULL);

	delete m->name;

	/* delete rules */
	if(!m->include_rules.empty()) {
		MenuRulesListIt it = m->include_rules.begin(), it_end = m->include_rules.end();
		while(it != it_end) {
			menu_rules_delete(*it);
			it = m->include_rules.erase(it);
		}
	}

	if(!m->exclude_rules.empty()) {
		MenuRulesListIt it = m->exclude_rules.begin(), it_end = m->exclude_rules.end();
		while(it != it_end) {
			menu_rules_delete(*it);
			it = m->exclude_rules.erase(it);
		}
	}

	/* recurse for nested menus */
	if(!m->submenus.empty()) {
		MenuParseListIt it = m->submenus.begin(), it_end = m->submenus.end();

		while(it != it_end) {
			menu_parse_context_delete(*it);
			it = m->submenus.erase(it);
		}
	}

	delete m;
}

static void menu_parse_context_append_default_dir_dirs(MenuParseContext *ctx) {
	StrList lst;

	int ret = system_data_dirs(lst);
	if(ret < 1)
		return;

	StrListIt it = lst.begin(), it_end = lst.end();

	for(; it != it_end; ++it)
		ctx->dir_dirs.push_back(build_filename((*it).c_str(), "desktop-directories"));
}

static void menu_parse_context_append_desktop_files(MenuParseContext *ctx, const char *dir, const char *basedir) {
	DIR *ds = opendir(dir);
	if(!ds)
		return;

	dirent       *dp;
	DesktopEntry *entry;

	while((dp = readdir(ds)) != NULL) {
		/* skip dots and (possibly) hidden files */
		if(dp->d_name[0] == '.' || DOT_OR_DOTDOT(dp->d_name))
			continue;

		entry = new DesktopEntry;
		entry->assign_path(dir, dp->d_name, basedir);

		if(file_test(entry->get_path(), FILE_TEST_IS_DIR)) {
			/* recurse if needed; the spec said we must */
			menu_parse_context_append_desktop_files(ctx, entry->get_path(), basedir);

			/* delete it */
			delete entry;
			continue;
		}

		/* files must ends with this extension */
		if(str_ends(entry->get_path(), ".desktop")) {
			ctx->desk_files.push_back(entry);
		} else {
			/* clear non .desktop items */
			delete entry;
		}
	}

	closedir(ds);
}

static void menu_parse_context_append_desktop_files_from_xdg_data_dirs(MenuParseContext *ctx) {
	StrList lst;
	xdg_menu_applications_location(lst);

	StrListIt it = lst.begin(), it_end = lst.end();
	for(; it != it_end; ++it)
		menu_parse_context_append_desktop_files(ctx, it->c_str(), it->c_str());
}

static void scan_include_exclude_tag(TiXmlNode *elem, MenuRulesList &rules) {
	E_RETURN_IF_FAIL(elem != NULL);

	TiXmlNode *child;
	TiXmlText *txt;

	for(child = elem->FirstChildElement(); child; child = child->NextSibling()) {
		/* assure we do not include/exclude insinde include/exclude */
		if(ELEMENT_IS(child, "Include") || ELEMENT_IS(child, "Exclude")) {
			E_WARNING(E_STRLOC ": Nesting <Include> and <Exclude> tags are not supported\n");
			continue;
		}

		if(ELEMENT_IS(child, "Filename")) {
			txt = ELEMENT_GET_TEXT(child);
			menu_rules_append_rule(rules, MENU_RULES_OPERATOR_FILENAME, txt->Value());
			continue;
		}

		if(ELEMENT_IS(child, "Category")) {
			txt = ELEMENT_GET_TEXT(child);
			menu_rules_append_rule(rules, MENU_RULES_OPERATOR_CATEGORY, txt->Value());
			continue;
		}

		if(ELEMENT_IS(child, "All")) {
			menu_rules_append_rule(rules, MENU_RULES_OPERATOR_ALL, NULL);
			continue;
		}

		if(ELEMENT_IS(child, "And")) {
			MenuRules *and_rule = menu_rules_append_rule(rules, MENU_RULES_OPERATOR_AND, NULL);
			/* recurse */
			scan_include_exclude_tag(child, and_rule->subrules);
			continue;
		}

		if(ELEMENT_IS(child, "Or")) {
			MenuRules *or_rule = menu_rules_append_rule(rules, MENU_RULES_OPERATOR_OR, NULL);
			/* recurse */
			scan_include_exclude_tag(child, or_rule->subrules);
			continue;
		}

		if(ELEMENT_IS(child, "Not")) {
			MenuRules *not_rule = menu_rules_append_rule(rules, MENU_RULES_OPERATOR_NOT, NULL);
			/* recurse */
			scan_include_exclude_tag(child, not_rule->subrules);
			continue;
		}
	}
}

static void scan_menu_tag(TiXmlNode *elem, MenuParseList &parse_list) {
	E_RETURN_IF_FAIL(elem != NULL);

	TiXmlText *txt;
	bool got_default_app_dirs = false, got_default_dir_dirs = false;

	MenuParseContext *ctx = menu_parse_context_new();

	for(elem = elem->FirstChildElement(); elem; elem = elem->NextSibling()) {
		/* in case we got '<Menu>' as submenu, dive in it recursively and fill submenus */
		if(ELEMENT_IS(elem, "Menu"))
			scan_menu_tag(elem, ctx->submenus);

		if(ELEMENT_IS(elem, "Name")) {
			txt = ELEMENT_GET_TEXT(elem);
			if(txt && !ctx->name)
				ctx->name = new String(txt->Value());

			continue;
		}
		
		if(ELEMENT_IS(elem, "Directory")) {
			txt = ELEMENT_GET_TEXT(elem);
			/* entries must ends with .directory */
			if(txt && str_ends(txt->Value(), ".directory")) {
				/* push it at the top */
				ctx->dir_files.push_front(txt->Value());
			}

			continue;
		} 
		
		if(ELEMENT_IS(elem, "AppDir")) {
			txt = ELEMENT_GET_TEXT(elem);
			if(txt)
				menu_parse_context_append_desktop_files(ctx, txt->Value(), NULL);

			continue;
		} 
		
		if(ELEMENT_IS(elem, "DirectoryDir")) {
			txt = ELEMENT_GET_TEXT(elem);
			if(txt) {
				/* push it at the top */
				ctx->dir_dirs.push_front(txt->Value());
			}

			continue;
		}

		/* spec: '<DefaultAppDirs>' expands to $XDG_DATA_DIRS/applications */
		if(ELEMENT_IS(elem, "DefaultAppDirs")) {
			if(!got_default_app_dirs) {
				menu_parse_context_append_desktop_files_from_xdg_data_dirs(ctx);
				/* scan it only once */
				got_default_app_dirs = true;
			}

			continue;
		}

		if(ELEMENT_IS(elem, "DefaultDirectoryDirs")) {
			if(!got_default_dir_dirs) {
				menu_parse_context_append_default_dir_dirs(ctx);
				got_default_dir_dirs = true;
			}

			continue;
		}

		if(ELEMENT_IS(elem, "Include")) {
			scan_include_exclude_tag(elem, ctx->include_rules);
			continue;
		}

		if(ELEMENT_IS(elem, "Exclude")) {
			scan_include_exclude_tag(elem, ctx->exclude_rules);
			continue;
		}

		if(ELEMENT_IS(elem, "Deleted")) {
			ctx->deleted = true;
			continue;
		}

		if(ELEMENT_IS(elem, "NotDeleted")) {
			ctx->deleted = false;
			continue;
		}

		if(ELEMENT_IS(elem, "OnlyUnallocated")) {
			ctx->only_unallocated = true;
			continue;
		}

		if(ELEMENT_IS(elem, "NotOnlyUnallocated")) {
			ctx->only_unallocated = false;
			continue;
		}
	}

	parse_list.push_back(ctx);
}

static bool menu_context_construct_name_and_get_icon(MenuParseContext *m, 
													 MenuParseContext *top, 
													 String **ret_name, 
													 String **ret_icon,
													 bool *should_be_displayed)
{
	E_RETURN_VAL_IF_FAIL(m != NULL, false);

	*ret_name = *ret_icon = NULL;
	*should_be_displayed = true;

	if(!m->dir_files.empty()) {
		/*
		 * We have two locations where are keeping directory list: node specific and top node
		 * specific, where often is put <DefaultDirectoryDirs/> tag. Here, first we will look in
		 * node specific directory list, then will go in top node <DefaultDirectoryDirs/>.
		 */
		StrListIt dir_it = m->dir_dirs.begin(), dir_it_end = m->dir_dirs.end();

		/* this list has 'stack-ed' items; the last one is on top */
		StrListIt file_it, file_it_end = m->dir_files.end();

		DesktopFile df;
		String      tmp;

		/* first try specific directory list */
		for(; dir_it != dir_it_end; ++dir_it) {
			for(file_it = m->dir_files.begin(); file_it != file_it_end; ++file_it) {
				tmp = build_filename((*dir_it).c_str(), (*file_it).c_str());
				//E_DEBUG("==> %s\n", tmp.c_str());

				/* load it and see if it is real .desktop file */
				df.load(tmp.c_str());

				if(df && (df.type() == DESK_FILE_TYPE_DIRECTORY)) {
					/* check if it can be displayed */
					if(df.no_display())
						*should_be_displayed = false;

					char buf[NAME_BUFSZ];

					/* try icon first */
					if(!(*ret_icon) && df.icon(buf, NAME_BUFSZ))
						*ret_icon = new String(buf);

					/* then name, so we can quit nicely */
					if(!(*ret_name) && df.name(buf, NAME_BUFSZ)) {
						*ret_name = new String(buf);
						return true;
					}
				}
			}
		}

		/* now try default ones */
		dir_it = top->dir_dirs.begin(), dir_it_end = top->dir_dirs.end();

		for(; dir_it != dir_it_end; ++dir_it) {
			for(file_it = m->dir_files.begin(); file_it != file_it_end; ++file_it) {
				tmp = build_filename((*dir_it).c_str(), (*file_it).c_str());
				//E_DEBUG("++> %s\n", tmp.c_str());

				/* load it and see if it is real .desktop file */
				df.load(tmp.c_str());
				if(df && (df.type() == DESK_FILE_TYPE_DIRECTORY)) {
					/* check if it can be displayed */
					if(df.no_display())
						*should_be_displayed = false;

					char buf[NAME_BUFSZ];

					/* try icon first */
					if(!(*ret_icon) && df.icon(buf, NAME_BUFSZ))
						*ret_icon = new String(buf);

					/* then name, so we can quit nicely */
					if(!(*ret_name) && df.name(buf, NAME_BUFSZ)) {
						*ret_name = new String(buf);
						return true;
					}
				}
			}
		}
	}	

	E_RETURN_VAL_IF_FAIL(m->name != NULL, false);

	/* if there are no files and can be displayed, use context name; let icon name be NULL */
	*ret_name = new String(*(m->name));
	return true;
}

static void apply_include_rules(MenuContext *ctx, DesktopEntryList &items, MenuRulesList &rules) {
	if(items.empty() || rules.empty())
		return;

	MenuRulesListIt     rit, rit_end = rules.end();
	DesktopEntryListIt  it = items.begin(), it_end = items.end();

	DesktopEntry *entry;
	bool         eval_true;

	for(; it != it_end; ++it) {
		for(rit = rules.begin(); rit != rit_end; ++rit) {
			entry = *it;

			eval_true = menu_rules_eval(*rit, entry);

			/* append entry if matches to the rule, and mark it as allocated */
			if(eval_true) {
				entry->mark_as_allocated();
				ctx->items.push_back(entry);

				/* do not scan rules any more; go to the next item */
				break;
			}
		}
	}
}

static void apply_exclude_rules(DesktopEntryList& items, MenuRulesList &rules) {
	if(items.empty() || rules.empty())
		return;

	MenuRulesListIt    rit, rit_end = rules.end();
	DesktopEntryListIt it = items.begin(), it_end = items.end();
	bool eval_true;

	while(it != it_end) {
		eval_true = false;

		for(rit = rules.begin(); rit != rit_end; ++rit) {
			eval_true = menu_rules_eval(*rit, *it);

			if(eval_true) {
				/* pop entry if matches */
				it = items.erase(it);
				break;
			}
		}

		if(!eval_true) 
			++it;
	}
}

#if NO_EMPTY_MENUS
/* forward decl */
static void menu_context_delete(MenuContext *c);
#endif

static MenuContext *menu_parse_context_to_menu_context(MenuParseContext *m, 
													   MenuParseContext *top,
													   DesktopEntryList *all_unallocated) 
{
	E_RETURN_VAL_IF_FAIL(m != NULL, NULL);

	/* make sure we are not processing only_unallocated node when not get all_unallocated */
	if(m->only_unallocated && !all_unallocated)
		return NULL;

	/* 
	 * figure out the name first; if returns false, either menu should not be displayed, or something
	 * went wrong
	 */
	String *n = NULL, *ic = NULL;
	bool   should_be_displayed = false;

	if(!menu_context_construct_name_and_get_icon(m, top, &n, &ic, &should_be_displayed))
		return NULL;

	/* 
	 * nodes marked as 'NoDisplay' (from .directory file) or '<Deleted>' (from applications.menu) must
	 * be processed as ordinary nodes, since this operation will correctly setup allocated
	 * (<OnlyUnallocated> and <NotOnlyUnallocated>) entries
	 */
	if(m->deleted)
		should_be_displayed = false;

	/* assure we got name here; icon can be NULL */
	E_RETURN_VAL_IF_FAIL(n != NULL, NULL);

	MenuContext *ctx = new MenuContext;
	ctx->name = n;
	ctx->icon = ic;
	ctx->display_it = should_be_displayed;
	//E_DEBUG("+ Menu: %s %i\n", ctx->name->c_str(), m->include_rules.size());

	/* fill MenuContext items, depending on what list was passed */
	if(all_unallocated) {
		apply_include_rules(ctx, *all_unallocated, m->include_rules);
	} else {
		apply_include_rules(ctx, m->desk_files, m->include_rules);
		/* check the rules for top list, but make sure we are not applying them on the same node again */
		if(m != top)
			apply_include_rules(ctx, top->desk_files, m->include_rules);
	}

	/* pop filled MenuContext items if match the rule */
	apply_exclude_rules(ctx->items, m->exclude_rules);

	//E_DEBUG("- Menu: %s %i\n", ctx->name->c_str(), ctx->items.size());
	
	/* sort entries via their full names */
	desktop_entry_list_sort(ctx->items);
	
	/* process submenus */
	if(!m->submenus.empty()) {
		MenuParseListIt mit = m->submenus.begin(), mit_end = m->submenus.end();
		MenuContext *sub_ctx;

		for(; mit != mit_end; ++mit) {
			sub_ctx = menu_parse_context_to_menu_context(*mit, top, all_unallocated);

			if(sub_ctx)
				ctx->submenus.push_back(sub_ctx);
		}
	}

#if NO_EMPTY_MENUS
	/* do not allow empty menus */
	if(ctx->items.empty() && ctx->submenus.empty()) {
		menu_context_delete(ctx);
		ctx = NULL;
	}
#endif

	return ctx;
}

static void menu_context_delete(MenuContext *c) {
	E_RETURN_IF_FAIL(c != NULL);

	if(!c->submenus.empty()) {
		MenuContextListIt it = c->submenus.begin(), it_end = c->submenus.end();
		for(; it != it_end; ++it)
			menu_context_delete(*it);
	}

	c->items.clear();
	delete c->name;
	delete c->icon;
	delete c;
}

static void menu_parse_context_list_get_all_unallocated_desk_files(MenuParseList &parse_list, DesktopEntryList &ret) {
	if(parse_list.empty())
		return;

	MenuParseListIt     it = parse_list.begin(), it_end = parse_list.end();
	DesktopEntryListIt  dit, dit_end;
	MenuParseContext    *parse_ctx;

	for(; it != it_end; ++it) {
		parse_ctx = *it;

		dit = parse_ctx->desk_files.begin();
		dit_end = parse_ctx->desk_files.end();

		for(; dit != dit_end; ++dit) {
			if((*dit)->is_allocated() == false)
				ret.push_back(*dit);
		}

		/* recurse */
		menu_parse_context_list_get_all_unallocated_desk_files(parse_ctx->submenus, ret);
	}
}

static void menu_context_list_sort(MenuContextList &lst) {
	if(lst.empty())
		return;

	lst.sort(menu_context_sorter);

	MenuContextListIt it = lst.begin(), it_end = lst.end();
	for(; it != it_end; ++it)
		menu_context_list_sort((*it)->submenus);
}

static void menu_parse_context_list_to_menu_context_list(MenuParseList &parse_list, 
														 MenuContextList &ret)
{
	MenuParseListIt it = parse_list.begin(), it_end = parse_list.end();
	MenuParseContext *parse_ctx;
	MenuContext      *ctx;

	for(; it != it_end; ++it) {
		parse_ctx = *it;

		/* remove duplicate id's */
		desktop_entry_list_remove_duplicates(parse_ctx->desk_files);

		/* read all .desktop files from disk */
		desktop_entry_list_load_all(parse_ctx->desk_files);

		/* now convert it to usable menu node */
		ctx = menu_parse_context_to_menu_context(parse_ctx, parse_ctx, NULL);

		if(ctx)
			ret.push_back(ctx);
	}

	/* now, pickup all unallocated items */
	DesktopEntryList all_unallocated;
	menu_parse_context_list_get_all_unallocated_desk_files(parse_list, all_unallocated);

	/* 
	 * second pass; process unallocated items, but put them in second list that will later be
	 * merged; this is to preserve the order got in the first list
	 */
	MenuContextList unallocated_list;

	for(it = parse_list.begin(); it != it_end; ++it) {
		parse_ctx = *it;

		/* now convert it to usable menu node */
		ctx = menu_parse_context_to_menu_context(parse_ctx, parse_ctx, &all_unallocated);

		if(ctx)
			unallocated_list.push_back(ctx);
	}

	/* 
	 * both list have the same root node, so we skip the first node and merge below it; the first node is 
	 * top level <Menu> and is often only menu name and description
	 */
	E_RETURN_IF_FAIL(ret.size() == 1);
	E_RETURN_IF_FAIL(unallocated_list.size() == 1);

	MenuContext *head = ret.front(), *unallocated_head = unallocated_list.front();
	MenuContextListIt uit = unallocated_head->submenus.begin(), uit_end = unallocated_head->submenus.end();

	for(; uit != uit_end; ++uit)
		head->submenus.push_back(*uit);

	/* sort everthing at the end */
	menu_context_list_sort(ret);
}

/* 
 * Count the number of items in each submenu + submenu node itself; used to allocate 
 * array for edelib::MenuItem list.
 */
static unsigned int menu_context_list_count(MenuContextList &lst) {
	if(lst.empty())
		return 0;

	unsigned int ret = lst.size();

	MenuContextListIt it = lst.begin(), it_end = lst.end();
	MenuContext *cc;

	for(; it != it_end; ++it) {
		cc = *it;
		ret += cc->items.size();

		ret += menu_context_list_count(cc->submenus);

		/* 
		 * a room for NULL to deduce submenus in edelib::MenuItem, no matter if submenus are empty
		 * in case empty menus are going to be displayed
		 */
		ret += 1;
	}

	return ret;
}

static void menu_all_parse_lists_clear(MenuParseList &parse_list, MenuContextList &ctx_list) {
	MenuContextListIt cit = ctx_list.begin(), cit_end = ctx_list.end();
	MenuParseListIt   pit = parse_list.begin(), pit_end = parse_list.end();

	MenuParseContext *cc;

	while(cit != cit_end) {
		menu_context_delete(*cit);
		cit = ctx_list.erase(cit);
	}

	while(pit != pit_end) {
		cc = *pit;
		/* 
		 * Desktop entries are shared among MenuContext and MenuParseContext, so they
		 * must be explicitly deleted. This sharing depends on 'Include' rules, so some MenuParseContext
		 * entries are on MenuContext list and all MenuContext entries are in MenuParseContext.
		 */
		DesktopEntryListIt it = cc->desk_files.begin(), it_end = cc->desk_files.end();
		while(it != it_end) {
			delete *it;
			it = cc->desk_files.erase(it);
		}

		menu_parse_context_delete(cc);
		pit = parse_list.erase(pit);
	}
}

static TiXmlNode *load_menu_file(TiXmlDocument &doc) {
	char   *menu_prefix = getenv("XDG_MENU_PREFIX");
	String  menu_file;

	if(menu_prefix) {
		menu_file = menu_prefix;
		menu_file += "applications.menu";
	} else {
		menu_file = "applications.menu";
	}

	StrList paths;
	if(system_config_dirs(paths) < 1)
		return NULL;

	String    tmp;
	StrListIt it = paths.begin(), it_end = paths.end();

	for(; it != it_end; ++it) {
		tmp = build_filename((*it).c_str(), "menus", menu_file.c_str());

		if(doc.LoadFile(tmp.c_str()))
			goto done;
	}

	return NULL;

done:
	return doc.FirstChild("Menu");
}

static void menu_context_list_dump(MenuContextList &lst) {
	if(lst.empty())
		return;

	MenuContextListIt  it = lst.begin(), it_end = lst.end();
	DesktopEntryListIt ds, de;

	for(; it != it_end; ++it) {
		if((*it)->display_it == false)
			continue;

		ds = (*it)->items.begin();
		de = (*it)->items.end();

		/* print each desktop entry with menu name */
		for(; ds != de; ++ds) {
			printf("%s/\t%s\t%s\n", (*it)->name->c_str(),
								    (*ds)->get_id(),
								    (*ds)->get_path());
		}

		menu_context_list_dump((*it)->submenus);
	}
}

static void menu_all_parse_lists_load(MenuParseList &parse_list, MenuContextList &content) {
	/* 
	 * TiXmlDocument object must be used externaly, so as long as this object is
	 * alive, the whole XML tree is alive too (see DOM reference).
	 */
	TiXmlDocument doc;

	TiXmlNode *elem = load_menu_file(doc);
	if(!elem)
		return;

	/* parse XML file */
	scan_menu_tag(elem, parse_list);

	/* convert it to our list */
	menu_parse_context_list_to_menu_context_list(parse_list, content);
}

void xdg_menu_dump_for_test_suite(void) {
	MenuParseList   pl;
	MenuContextList cl;

	/* load everything */
	menu_all_parse_lists_load(pl, cl);

	menu_context_list_dump(cl);

	/* clear everything */
	menu_all_parse_lists_clear(pl, cl);
}

/* public API */

struct XdgMenuContent {
	MenuItem *fltk_menu;

	/*
	 * We are keeping them as fltk_menu references some objects, like text.
	 * This is since FLTK menu can't free label strings as MenuItem is plain struct.
	 */
	MenuParseList   parse_list;
	MenuContextList context_list;
};

static void item_cb(Fl_Widget*, void *en) {
	DesktopEntry *entry = (DesktopEntry*)en;
	run_async("ede-launch %s", entry->get_exec());

	E_DEBUG(E_STRLOC ": ede-launch %s\n", entry->get_exec());
}

static void logout_cb(Fl_Widget*, void*) {
	run_async("ede-shutdown");
}

static unsigned int construct_edelib_menu(MenuContextList &lst, MenuItem *mi, unsigned int pos) {
	if(lst.empty())
		return pos;

	MenuContextListIt it = lst.begin(), it_end = lst.end();
	MenuContext *cc;

	DesktopEntryListIt ds, de;

	unsigned long initial_pos = pos;

	for(; it != it_end; ++it) {
		cc = *it;

		if(!cc->display_it)
			continue;

		mi[pos].text = cc->name->c_str();

		/* every MenuContext is submenu for itself */
		mi[pos].flags = FL_SUBMENU;

		//E_DEBUG("{%s submenu}\n", mi[pos].text);

		/* some default values that must be filled */
		mi[pos].shortcut_ = 0;
		mi[pos].callback_ = 0;
		mi[pos].user_data_ = 0;
		mi[pos].labeltype_ = FL_NORMAL_LABEL;
		mi[pos].labelfont_ = FL_HELVETICA;
		mi[pos].labelsize_ = FL_NORMAL_SIZE;
		mi[pos].labelcolor_ = FL_BLACK;

		MenuItem::init_extensions(&mi[pos]);

		/* set image for menu */
		if(cc->icon && IconLoader::inited()) {
			Fl_Image *img = IconLoader::get(cc->icon->c_str(), ICON_SIZE_SMALL);
			mi[pos].image(img);
		}

		/* a room for an item */
		pos++;

		/* try with nested submenus first, so submenus be before desktop entries in current menu node */
		pos = construct_edelib_menu(cc->submenus, mi, pos);

		/* now, add the real items if they exists*/
		if(!cc->items.empty()) {
			ds = cc->items.begin();
			de = cc->items.end();

			for(; ds != de; ++ds, ++pos) {
				mi[pos].text = (*ds)->get_name();
				mi[pos].flags = 0;
				
				//E_DEBUG("  {%s item}\n", mi[pos].text);

				/* some default values that must be filled */
				mi[pos].shortcut_ = 0;

				/* set callback and callback data to be current entry */
				mi[pos].callback_ = item_cb;
				mi[pos].user_data_ = *ds;

				mi[pos].labeltype_ = FL_NORMAL_LABEL;
				mi[pos].labelfont_ = FL_HELVETICA;
				mi[pos].labelsize_ = FL_NORMAL_SIZE;
				mi[pos].labelcolor_ = FL_BLACK;
				MenuItem::init_extensions(&mi[pos]);

				/* set image for menu item*/
				if((*ds)->get_icon() && IconLoader::inited()) {
					Fl_Image *img = IconLoader::get((*ds)->get_icon(), ICON_SIZE_SMALL);
					mi[pos].image(img);
				}

				/* set tooltip if we have; it is actually a comment from .desktop file */
				mi[pos].tooltip((*ds)->get_comment());
			}
		}

		/* to inject Logout button */
		if(initial_pos == 0) {
			//E_DEBUG("  {Logout item}\n");

			mi[pos].text = _("Logout");

			if(pos) 
				mi[pos - 1].flags |= FL_MENU_DIVIDER;

			mi[pos].flags = 0;
			mi[pos].shortcut_ = 0;
			mi[pos].labeltype_ = FL_NORMAL_LABEL;
			mi[pos].labelfont_ = FL_HELVETICA;
			mi[pos].labelsize_ = FL_NORMAL_SIZE;
			mi[pos].labelcolor_ = FL_BLACK;
			MenuItem::init_extensions(&mi[pos]);

			/* set callback and callback data to be current entry */
			mi[pos].callback_ = logout_cb;
			mi[pos].user_data_ = 0;
	
			if(IconLoader::inited()) {
				Fl_Image *img = IconLoader::get("system-log-out", ICON_SIZE_SMALL);
				mi[pos].image(img);
			}

			pos++;
		}

		/* end this menu */
		mi[pos].text = NULL;
		MenuItem::init_extensions(&mi[pos]);

		//E_DEBUG("{0}\n");

		/* make a room for the next item */
		pos++;

	}

	/* return position to next item */
	return pos;
}

void xdg_menu_applications_location(StrList &lst) {
	lst.clear();

	if(system_data_dirs(lst) < 1)
		return;

	StrListIt it = lst.begin(), it_end = lst.end();

	for(; it != it_end; ++it)
		*it = build_filename(it->c_str(), "applications");

	/* 
	 * Add user directory too; the spec is unclear about this, but official menu spec tests
	 * requires it. Also, users will be able to add menu items without superuser permissions.
	 */
	String user_dir = user_data_dir();
	lst.push_back(build_filename(user_dir.c_str(), "applications"));
}

XdgMenuContent *xdg_menu_load(void) {
	XdgMenuContent *content = new XdgMenuContent();

	/* load everything */
	menu_all_parse_lists_load(content->parse_list, content->context_list);

	unsigned int sz = menu_context_list_count(content->context_list);
	E_RETURN_VAL_IF_FAIL(sz > 0, NULL);

	MenuItem *mi = new MenuItem[sz + 2]; /* plus logout + ending NULL */

	unsigned int pos = construct_edelib_menu(content->context_list, mi, 0);
	mi[pos].text = NULL;

	/* 
	 * MenuItem does not have constructor, so everywhere where we access MenuItem object, image
	 * member must be NULL-ed too
	 */
	MenuItem::init_extensions(&mi[pos]);

	E_ASSERT(pos <= sz + 2);

	content->fltk_menu = mi;
	return content;
}

void xdg_menu_delete(XdgMenuContent *m) {
	E_RETURN_IF_FAIL(m != NULL);

	delete [] m->fltk_menu;
	menu_all_parse_lists_clear(m->parse_list, m->context_list);
	delete m;
}

MenuItem *xdg_menu_to_fltk_menu(XdgMenuContent *m) {
	E_RETURN_VAL_IF_FAIL(m != NULL, NULL);
	return m->fltk_menu;
}
