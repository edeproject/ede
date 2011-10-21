#ifndef __EDE_PREFERRED_APPLICATIONS_APPS_H__
#define __EDE_PREFERRED_APPLICATIONS_APPS_H__

class Fl_Choice;

struct KnownApp {
	const char *name;
	const char *cmd;
};

/* _n_ and _b_ are special predefined values so we from callback knows what to do */
#define KNOWN_APP_START {_("None"), "_n_"}
#define KNOWN_APP_END \
{"Browse...", "_b_"}, \
{0, 0} 

typedef void (*KnownAppCb)(const KnownApp &app, void *data);

void      app_for_each(KnownApp *lst, KnownAppCb cb, void *data = 0);
KnownApp *app_find_by_name(KnownApp *lst, const char *name);
KnownApp *app_find_by_cmd(KnownApp *lst, const char *cmd);
KnownApp *app_get(KnownApp *lst, int index);

int        app_get_index(KnownApp *lst, const char *cmd);
inline int app_get_index(KnownApp *lst, KnownApp &a) { return app_get_index(lst, a.cmd); }

void      app_populate_menu(KnownApp *lst, Fl_Choice *c);

bool        app_is_magic_cmd(const char *cmd);
inline bool app_is_magic_cmd(const KnownApp &a) { return app_is_magic_cmd(a.cmd); }

bool app_is_browse_item(KnownApp *lst, const char *name);
#endif
