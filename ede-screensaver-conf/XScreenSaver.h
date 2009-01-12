#ifndef __XSCREENSAVER_H__
#define __XSCREENSAVER_H__

#include <X11/Xproto.h>
#include <edelib/List.h>
#include <edelib/String.h>

struct SaverHack {
	edelib::String name;
	edelib::String exec;
	unsigned int   sindex;
};

/* TODO: edelib list::sort() bug */
inline bool saver_hack_cmp(SaverHack* const& s1, SaverHack* const& s2) 
{ return s1->name < s2->name; }

typedef edelib::list<SaverHack*>           HackList;
typedef edelib::list<SaverHack*>::iterator HackListIter;

struct SaverPrefs {
	HackList     hacks;
	unsigned int curr_hack;
	int          timeout;

	bool dpms_enabled;
	int  dpms_standby;
	int  dpms_suspend;
	int  dpms_off;
};

bool        xscreensaver_run_daemon(Display* py);

SaverPrefs *xscreensaver_read_config(void);
void        xscreensaver_save_config(SaverPrefs *sp);

void        xscreensaver_preview(int id, const char* name);
void        xscreensaver_kill_preview(void);

#endif
