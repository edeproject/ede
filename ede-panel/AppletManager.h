#ifndef __APPLETMANAGER_H__
#define __APPLETMANAGER_H__

#include <edelib/List.h>
#include <edelib/String.h>
#include "Applet.h"

class  Panel;
class  Fl_Widget;
struct AppletData;

typedef edelib::list<AppletData*> AList;
typedef edelib::list<AppletData*>::iterator AListIter;

class AppletManager {
private:
	AList applet_list;
public:
	~AppletManager();
	bool load(const char *path);
	void clear(void);
	void fill_group(Panel *p);
	void unfill_group(Panel *p);

	bool get_applet_options(Fl_Widget *o, unsigned long &opts);
	unsigned int napplets(void) const { return applet_list.size(); }
};

#endif
