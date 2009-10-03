#ifndef __MENURULES_H__
#define __MENURULES_H__

#include <edelib/List.h>
#include <edelib/String.h>

EDELIB_NS_USING(list)
EDELIB_NS_USING(String)

struct MenuRules;
struct DesktopEntry;

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
