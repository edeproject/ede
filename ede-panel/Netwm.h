#ifndef __NETWM_H__
#define __NETWM_H__

#include <X11/Xlib.h>

/* NETWM helpers for panel and applets */

class Fl_Window;

enum {
	NETWM_CHANGED_WORKSPACE_COUNT,
	NETWM_CHANGED_WORKSPACE_NAMES,
	NETWM_CHANGED_CURRENT_WORKSPACE,
	NETWM_CHANGED_CURRENT_WORKAREA,
	NETWM_CHANGED_ACTIVE_WINDOW,
	NETWM_CHANGED_WINDOW_NAME,
	NETWM_CHANGED_WINDOW_VISIBLE_NAME,
	NETWM_CHANGED_WINDOW_DESKTOP,
	NETWM_CHANGED_WINDOW_LIST
};

enum WmStateValue {
	WM_STATE_NONE      = -1,
	WM_STATE_WITHDRAW  = 0,
	WM_STATE_NORMAL    = 1,
	WM_STATE_ICONIC    = 3
};

typedef void (*NetwmCallback)(int action, Window xid, void *data);

/* register callback for NETWM_* changes */
void netwm_callback_add(NetwmCallback cb, void *data = 0);

/* remove callback if exists */
void netwm_callback_remove(NetwmCallback cb);

/* get current workare set by window manager; return false if fails */
bool netwm_get_workarea(int& x, int& y, int& w, int &h);

/* make 'dock' type from given window */
void netwm_make_me_dock(Fl_Window *win);

/* resize area by setting offsets to each side; 'win' will be outside that area */
void netwm_set_window_strut(Fl_Window *win, int left, int right, int top, int bottom);

/* return number of workspaces */
int netwm_get_workspace_count(void);

/* change current workspace */
void netwm_change_workspace(int n);

/* current visible workspace; workspaces are starting from 0 */
int netwm_get_current_workspace(void);

/* workspace names; return number of allocated ones; call on this XFreeStringList(names) */
int netwm_get_workspace_names(char**& names);

/* get a list of mapped windows; returns number in the list, or -1 if fails; call XFree when done */
int netwm_get_mapped_windows(Window **windows);

/* returns -2 on error, -1 on sticky, else is desktop */
int netwm_get_window_workspace(Window win);

/* return 1 if given window is manageable (desktop/dock/splash types are not manageable) */
int netwm_manageable_window(Window win);

/* return window title or NULL if fails; call free() on returned string */
char *netwm_get_window_title(Window win);

/* return ID of currently focused window; if fails, return -1 */
Window netwm_get_active_window(void);

/* try to focus given window */
void netwm_set_active_window(Window win);

/* maximize window */
void netwm_maximize_window(Window win);

/* close window */
void netwm_close_window(Window win);

/* edewm specific: restore window to previous state */
void wm_ede_restore_window(Window win);

/* not part of NETWM, but helpers until _NET_WM_STATE_* is implemented */
WmStateValue wm_get_window_state(Window win);
void         wm_set_window_state(Window win, WmStateValue state);

#endif
