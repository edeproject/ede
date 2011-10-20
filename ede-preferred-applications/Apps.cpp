#include <string.h>
#include <FL/Fl_Choice.H>
#include <edelib/Debug.h>
#include <edelib/File.h>
#include "Apps.h"

EDELIB_NS_USING(file_path)
EDELIB_NS_USING(String)

void app_for_each(KnownApp *lst, KnownAppCb cb, void *data) {
	E_RETURN_IF_FAIL(lst != 0);

	for(int i = 0; lst[i].name; i++)
		cb(lst[i], data);
}

KnownApp *app_find_by_cmd(KnownApp *lst, const char *name) {
	E_RETURN_VAL_IF_FAIL(lst != 0, 0);
	E_RETURN_VAL_IF_FAIL(name, 0);

	for(int i = 0; lst[i].name; i++)
		if(strcmp(lst[i].cmd, name) == 0)
			return &lst[i];
	return 0;
}

KnownApp *app_find_by_name(KnownApp *lst, const char *cmd) {
	E_RETURN_VAL_IF_FAIL(lst != 0, 0);
	E_RETURN_VAL_IF_FAIL(cmd, 0);

	for(int i = 0; lst[i].name; i++)
		if(strcmp(lst[i].cmd, cmd) == 0)
			return &lst[i];
	return 0;
}

KnownApp *app_get(KnownApp *lst, int index) {
	E_RETURN_VAL_IF_FAIL(lst != 0, 0);
	return &lst[index];
}

int app_get_index(KnownApp *lst, const char *cmd) {
	E_RETURN_VAL_IF_FAIL(lst != 0, -1);
	E_RETURN_VAL_IF_FAIL(cmd != 0, -1);

	for(int i = 0; lst[i].name; i++) {
		if(strcmp(lst[i].cmd, cmd) == 0)
			return i;
	}

	return -1;
}

static void populate_menu(const KnownApp &item, void *data) {
	Fl_Choice    *c = (Fl_Choice*)data;
	const char *cmd = item.cmd;

	/* check some magic values first */
	if(app_is_magic_cmd(cmd)) {
		c->add(item.name);
		return;
	}

	/* check if binary exists */
	String ret = file_path(cmd);
	if(ret.empty()) return;

	c->add(item.name);
}

void app_populate_menu(KnownApp *lst, Fl_Choice *c) {
	E_RETURN_IF_FAIL(lst != 0);
	E_RETURN_IF_FAIL(c != 0);

	app_for_each(lst, populate_menu, c);
}

bool app_is_magic_cmd(const char *cmd) {
	E_RETURN_VAL_IF_FAIL(cmd != 0, false);

	if(cmd[0] == '_' && strlen(cmd) == 3 && (cmd[1] == 'n' || cmd[1] == 'b'))
		return true;
	return false;
}
