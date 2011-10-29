#ifndef __APPCHOICE_H__
#define __APPCHOICE_H__

#include <FL/Fl_Choice.H>
#include <edelib/String.h>
#include "PredefApps.h"

EDELIB_NS_USING(String)

/* choices with name/value pair */
class AppChoice : public Fl_Choice {
private:
	KnownApp *known_apps;
	/* previous selected val */
	int       pvalue; 
	/* user selected program; for now allow only one */
	String    user_val; 
public:
	AppChoice(int X, int Y, int W, int H, const char *l = 0);
	void add_programs(KnownApp *lst);
	void add_if_user_program(const char *cmd);
	void select_by_cmd(const char *cmd);
	const char *selected(void);
	void on_select(void);

	/* 
	 * FLTK 1.3 has this function, but not previous one versions.
	 * Here, we are not traversing submenus, e.g. Edit/Copy, but only top level ones
	 */
	int find_item_index(const char *p);
};

#endif
