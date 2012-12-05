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

#ifndef __MENURULES_H__
#define __MENURULES_H__

#include <edelib/List.h>
#include <edelib/String.h>

EDELIB_NS_USING(list)
EDELIB_NS_USING(String)

struct MenuRules;
class DesktopEntry;

typedef list<MenuRules*> MenuRulesList;
typedef list<MenuRules*>::iterator MenuRulesListIt;

enum {
	MENU_RULES_OPERATOR_NONE,
	MENU_RULES_OPERATOR_FILENAME,
	MENU_RULES_OPERATOR_CATEGORY,
	MENU_RULES_OPERATOR_AND,
	MENU_RULES_OPERATOR_OR,
	MENU_RULES_OPERATOR_NOT,
	MENU_RULES_OPERATOR_ALL
};

struct MenuRules {
	short  rule_operator;
	String data;

	MenuRulesList subrules;
};

MenuRules *menu_rules_new(void);

MenuRules *menu_rules_append_rule(MenuRulesList &rules, short rule, const char *data);
void       menu_rules_delete(MenuRules *m);

bool       menu_rules_eval(MenuRules *m, DesktopEntry *en);

#endif
