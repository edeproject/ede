#ifndef __XSCREENSAVER_H__
#define __XSCREENSAVER_H__

#include <edelib/List.h>
#include <edelib/String.h>

struct SaverHack {
	edelib::String name;
	int            sindex;
};

typedef edelib::list<SaverHack*>           HackList;
typedef edelib::list<SaverHack*>::iterator HackListIter;

struct SaverPrefs {
	HackList hacks;
	int      curr_hack;
	int      timeout;

	bool dpms_enabled;
	int  dpms_standby;
	int  dpms_suspend;
	int  dpms_off;
};

bool        xscreensaver_run(void);
SaverPrefs *xscreensaver_read_config(void);

#endif
