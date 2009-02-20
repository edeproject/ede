#include "Frame.h"
#include "Winhints.h"
#include "Desktop.h"

#include "debug.h"

Atom _XA_UTF8_STRING;
Atom _XA_SM_CLIENT_ID;

//ICCCM
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_STATE;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;

//MWM
Atom _XATOM_MWM_HINTS;
Atom _XATOM_MOTIF_WM_INFO;

// Extended WM (NetWM)
Atom _XA_NET_SUPPORTED; 
Atom _XA_NET_CLIENT_LIST;
Atom _XA_NET_CLIENT_LIST_STACKING;
Atom _XA_NET_NUM_DESKTOPS;
Atom _XA_NET_DESKTOP_GEOMETRY;
Atom _XA_NET_DESKTOP_VIEWPORT;
Atom _XA_NET_CURRENT_DESKTOP;
Atom _XA_NET_DESKTOP_NAMES;
Atom _XA_NET_ACTIVE_WINDOW;
Atom _XA_NET_WORKAREA;
Atom _XA_NET_SUPPORTING_WM_CHECK;
Atom _XA_NET_VIRTUAL_ROOTS;
Atom _XA_NET_DESKTOP_LAYOUT;
Atom _XA_NET_SHOWING_DESKTOP;
Atom _XA_NET_CLOSE_WINDOW;
Atom _XA_NET_WM_MOVERESIZE;
Atom _XA_NET_RESTACK_WINDOW;
Atom _XA_NET_REQUEST_FRAME_EXTENTS;
Atom _XA_NET_WM_NAME;
Atom _XA_NET_WM_VISIBLE_NAME;
Atom _XA_NET_WM_ICON_NAME;
Atom _XA_NET_WM_ICON_VISIBLE_NAME;
Atom _XA_NET_WM_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
Atom _XA_NET_WM_WINDOW_TYPE_MENU;
Atom _XA_NET_WM_WINDOW_TYPE_UTIL;
Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;
Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;
Atom _XA_NET_WM_STATE;
Atom _XA_NET_WM_STATE_MODAL;
Atom _XA_NET_WM_STATE_STICKY;
Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
Atom _XA_NET_WM_STATE_SHADED;
Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
Atom _XA_NET_WM_STATE_SKIP_PAGER;
Atom _XA_NET_WM_STATE_HIDDEN; // new in spec. 1.3
Atom _XA_NET_WM_STATE_FULLSCREEN; // new in spec. 1.3
Atom _XA_NET_WM_STATE_ABOVE; // new in spec. 1.3
Atom _XA_NET_WM_STATE_BELOW; // new in spec. 1.3
Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION; // new in spec. 1.3
Atom _XA_NET_WM_ALLOWED_ACTIONS;
Atom _XA_NET_WM_ACTION_MOVE;
Atom _XA_NET_WM_ACTION_RESIZE;
Atom _XA_NET_WM_ACTION_MINIMIZE;
Atom _XA_NET_WM_ACTION_SHADE;
Atom _XA_NET_WM_ACTION_STICK;
Atom _XA_NET_WM_ACTION_MAXIMIZE_HORZ;
Atom _XA_NET_WM_ACTION_MAXIMIZE_VERT;
Atom _XA_NET_WM_ACTION_FULLSCREEN;
Atom _XA_NET_WM_ACTION_CHANGE_DESKTOP;
Atom _XA_NET_WM_ACTION_CLOSE;
Atom _XA_NET_WM_STRUT;
Atom _XA_NET_WM_STRUT_PARTIAL;
Atom _XA_NET_WM_ICON_GEOMETRY;
Atom _XA_NET_WM_ICON;
Atom _XA_NET_WM_PID;
Atom _XA_NET_WM_HANDLED_ICONS;
Atom _XA_NET_WM_USER_TIME;
Atom _XA_NET_FRAME_EXTENTS;
Atom _XA_NET_WM_PING;
Atom _XA_NET_WM_SYNC_REQUEST;
Atom _XA_NET_WM_STATE_STAYS_ON_TOP;

//KDE1
Atom _XA_KWM_WIN_ICON;

//EDE
Atom _XA_EDE_WM_ACTION;
Atom _XA_EDE_WM_LOGOUT;
Atom _XA_EDE_WM_RESTORE_SIZE;



#define CNT(x) (sizeof(x)/sizeof(x[0]))

static bool atoms_inited = false;
void init_atoms()
{
    if(atoms_inited) return;

    struct {
        Atom *atom;
        const char *name;
    } atom_info[] = {
        { &_XA_UTF8_STRING,        "UTF8_STRING" },
        { &_XA_SM_CLIENT_ID,       "SM_CLIENT_ID" },

        //ICCCM
        { &_XA_WM_PROTOCOLS,      "WM_PROTOCOLS" },
        { &_XA_WM_TAKE_FOCUS,     "WM_TAKE_FOCUS" },
        { &_XA_WM_DELETE_WINDOW,  "WM_DELETE_WINDOW" },
        { &_XA_WM_STATE,            "WM_STATE" },
        { &_XA_WM_CHANGE_STATE,     "WM_CHANGE_STATE" },
        { &_XA_WM_COLORMAP_WINDOWS, "WM_COLORMAP_WINDOWS" },
        { &_XA_WM_CLIENT_LEADER,    "WM_CLIENT_LEADER" },

        //MWM
        { &_XATOM_MWM_HINTS,        _XA_MOTIF_WM_HINTS },
        { &_XATOM_MOTIF_WM_INFO,    _XA_MOTIF_WM_INFO },

        //KDE1
        { &_XA_KWM_WIN_ICON,        "KWM_WIN_ICON" },

        //Extended WM
        { &_XA_NET_SUPPORTED,       "_NET_SUPPORTED" },
        { &_XA_NET_CLIENT_LIST,     "_NET_CLIENT_LIST" },
        { &_XA_NET_CLIENT_LIST_STACKING, "_NET_CLIENT_LIST_STACKING" },
        { &_XA_NET_NUM_DESKTOPS,    "_NET_NUMBER_OF_DESKTOPS" },
        { &_XA_NET_DESKTOP_GEOMETRY,"_NET_DESKTOP_GEOMETRY" },
        { &_XA_NET_DESKTOP_VIEWPORT,"_NET_DESKTOP_VIEWPORT" },
        { &_XA_NET_CURRENT_DESKTOP, "_NET_CURRENT_DESKTOP" },
        { &_XA_NET_DESKTOP_NAMES,   "_NET_DESKTOP_NAMES" },
        { &_XA_NET_ACTIVE_WINDOW,   "_NET_ACTIVE_WINDOW" },
        { &_XA_NET_WORKAREA,        "_NET_WORKAREA" },
        { &_XA_NET_SUPPORTING_WM_CHECK, "_NET_SUPPORTING_WM_CHECK" },
        { &_XA_NET_VIRTUAL_ROOTS,   "_NET_VIRTUAL_ROOTS" },
        { &_XA_NET_DESKTOP_LAYOUT,  "_NET_DESKTOP_LAYOUT" },
        { &_XA_NET_SHOWING_DESKTOP, "_NET_SHOWING_DESKTOP" },
        { &_XA_NET_CLOSE_WINDOW,    "_NET_CLOSE_WINDOW" },
        { &_XA_NET_WM_MOVERESIZE,   "_NET_WM_MOVERESIZE" },
        { &_XA_NET_RESTACK_WINDOW,  "_NET_RESTACK_WINDOW" },
        { &_XA_NET_REQUEST_FRAME_EXTENTS, "_NET_REQUEST_FRAME_EXTENTS" },
        { &_XA_NET_WM_NAME,         "_NET_WM_NAME" },
        { &_XA_NET_WM_VISIBLE_NAME, "_NET_WM_VISIBLE_NAME" },
        { &_XA_NET_WM_ICON_NAME,    "_NET_WM_ICON_NAME" },
        { &_XA_NET_WM_ICON_VISIBLE_NAME, "_NET_WM_ICON_VISIBLE_NAME" },
        { &_XA_NET_WM_DESKTOP,      "_NET_WM_DESKTOP" },
        { &_XA_NET_WM_WINDOW_TYPE,  "_NET_WM_WINDOW_TYPE" },
        { &_XA_NET_WM_WINDOW_TYPE_DESKTOP, "_NET_WM_WINDOW_TYPE_DESKTOP" },
        { &_XA_NET_WM_WINDOW_TYPE_DOCK,    "_NET_WM_WINDOW_TYPE_DOCK" },
        { &_XA_NET_WM_WINDOW_TYPE_TOOLBAR, "_NET_WM_WINDOW_TYPE_TOOLBAR" },
        { &_XA_NET_WM_WINDOW_TYPE_MENU,    "_NET_WM_WINDOW_TYPE_MENU" },
        { &_XA_NET_WM_WINDOW_TYPE_UTIL,    "_NET_WM_WINDOW_TYPE_UTILITY" },
        { &_XA_NET_WM_WINDOW_TYPE_SPLASH,  "_NET_WM_WINDOW_TYPE_SPLASH" },
        { &_XA_NET_WM_WINDOW_TYPE_DIALOG,  "_NET_WM_WINDOW_TYPE_DIALOG" },
        { &_XA_NET_WM_WINDOW_TYPE_NORMAL,  "_NET_WM_WINDOW_TYPE_NORMAL" },
        { &_XA_NET_WM_STATE,        "_NET_WM_STATE" },
        { &_XA_NET_WM_STATE_MODAL,          "_NET_WM_STATE_MODAL" },
        { &_XA_NET_WM_STATE_STICKY,         "_NET_WM_STATE_STICKY" },
        { &_XA_NET_WM_STATE_MAXIMIZED_VERT, "_NET_WM_STATE_MAXIMIZED_VERT" },
        { &_XA_NET_WM_STATE_MAXIMIZED_HORZ, "_NET_WM_STATE_MAXIMIZED_HORZ" },
        { &_XA_NET_WM_STATE_SHADED,         "_NET_WM_STATE_SHADED" },
        { &_XA_NET_WM_STATE_SKIP_TASKBAR,   "_NET_WM_STATE_SKIP_TASKBAR" },
        { &_XA_NET_WM_STATE_SKIP_PAGER,     "_NET_WM_STATE_SKIP_PAGER" },
        { &_XA_NET_WM_STATE_HIDDEN,         "_NET_WM_STATE_HIDDEN" },
        { &_XA_NET_WM_STATE_FULLSCREEN,     "_NET_WM_STATE_FULLSCREEN" },
        { &_XA_NET_WM_STATE_ABOVE,          "_NET_WM_STATE_ABOVE" },
        { &_XA_NET_WM_STATE_BELOW,          "_NET_WM_STATE_BELOW" },
        { &_XA_NET_WM_STATE_DEMANDS_ATTENTION, "_NET_WM_STATE_DEMANDS_ATTENTION" },
        { &_XA_NET_WM_ALLOWED_ACTIONS, "_NET_WM_ALLOWED_ACTIONS" },
        { &_XA_NET_WM_ACTION_MOVE,          "_NET_WM_ACTION_MOVE" },
        { &_XA_NET_WM_ACTION_RESIZE,        "_NET_WM_ACTION_RESIZE" },
        { &_XA_NET_WM_ACTION_MINIMIZE,      "_NET_WM_ACTION_MINIMIZE" },
        { &_XA_NET_WM_ACTION_SHADE,         "_NET_WM_ACTION_SHADE" },
        { &_XA_NET_WM_ACTION_STICK,         "_NET_WM_ACTION_STICK" },
        { &_XA_NET_WM_ACTION_MAXIMIZE_HORZ, "_NET_WM_ACTION_MAXIMIZE_HORZ" },
        { &_XA_NET_WM_ACTION_MAXIMIZE_VERT, "_NET_WM_ACTION_MAXIMIZE_VERT" },
        { &_XA_NET_WM_ACTION_FULLSCREEN,    "_NET_WM_ACTION_FULLSCREEN" },
        { &_XA_NET_WM_ACTION_CHANGE_DESKTOP, "_NET_WM_ACTION_CHANGE_DESKTOP" },
        { &_XA_NET_WM_ACTION_CLOSE,         "_NET_WM_ACTION_CLOSE" },
        { &_XA_NET_WM_STRUT,       "_NET_WM_STRUT" },
        { &_XA_NET_WM_STRUT_PARTIAL, "_NET_WM_STRUT_PARTIAL" },
        { &_XA_NET_WM_ICON_GEOMETRY, "_NET_WM_ICON_GEOMETRY" },
        { &_XA_NET_WM_ICON,        "_NET_WM_ICON" },
        { &_XA_NET_WM_PID,         "_NET_WM_PID" },
        { &_XA_NET_WM_HANDLED_ICONS, "_NET_WM_HANDLED_ICONS" },
        { &_XA_NET_WM_USER_TIME,   "_NET_WM_USER_TIME" },
        { &_XA_NET_FRAME_EXTENTS,  "_NET_FRAME_EXTENTS" },
        { &_XA_NET_WM_PING,        "_NET_WM_PING" },
        { &_XA_NET_WM_SYNC_REQUEST, "_NET_WM_SYNC_REQUEST" },
        /* the next one is not in the spec but KDE use it */
        { &_XA_NET_WM_STATE_STAYS_ON_TOP, "_NET_WM_STATE_STAYS_ON_TOP" },
        /* KDE 1 */
        { &_XA_KWM_WIN_ICON,       "_NET_KWM_WIN_ICON" },
        /* EDE specific message */

		{ &_XA_EDE_WM_ACTION,       "_EDE_WM_ACTION" },
		{ &_XA_EDE_WM_LOGOUT,       "_EDE_WM_LOGOUT" },
		{ &_XA_EDE_WM_RESTORE_SIZE, "_EDE_WM_RESTORE_SIZE" }

        /*
        { &_XA_CLIPBOARD,           "CLIPBOARD" },
        { &_XA_TARGETS,             "TARGETS" },
        { &XA_XdndAware,            "XdndAware" },
        { &XA_XdndEnter,            "XdndEnter" },
        { &XA_XdndLeave,            "XdndLeave" },
        { &XA_XdndPosition,         "XdndPosition" },
        { &XA_XdndStatus,           "XdndStatus" },
        { &XA_XdndDrop,             "XdndDrop" },
        { &XA_XdndFinished,         "XdndFinished" }
        */
    };

    unsigned int i;
    for(i = 0; i < CNT(atom_info); i++) {
        *(atom_info[i].atom) = XInternAtom(fl_display, atom_info[i].name, False);
    }

    atoms_inited = true;
}

void register_protocols(Window root)
{
    static bool protos_registered = false;
    if(protos_registered) return;

    Atom net_proto[] = {
        _XA_NET_SUPPORTED,
        _XA_NET_CLIENT_LIST,
        _XA_NET_CLIENT_LIST_STACKING,
        _XA_NET_NUM_DESKTOPS,
        _XA_NET_DESKTOP_GEOMETRY,
        _XA_NET_DESKTOP_VIEWPORT,
        _XA_NET_SHOWING_DESKTOP,
        _XA_NET_CURRENT_DESKTOP,
        _XA_NET_DESKTOP_NAMES,
        _XA_NET_ACTIVE_WINDOW,
        _XA_NET_WORKAREA,
        _XA_NET_SUPPORTING_WM_CHECK,
        //_XA_NET_VIRTUAL_ROOTS, //hmmm.... I didnt understand this one...
        _XA_NET_CLOSE_WINDOW,
        //_XA_NET_WM_MOVERESIZE,
        _XA_NET_WM_NAME,
        //_XA_NET_WM_VISIBLE_NAME,
        _XA_NET_WM_ICON_NAME,
        //_XA_NET_WM_ICON_VISIBLE_NAME,
        _XA_NET_WM_DESKTOP,
        _XA_NET_WM_WINDOW_TYPE,
        _XA_NET_WM_WINDOW_TYPE_DESKTOP,
        _XA_NET_WM_WINDOW_TYPE_DOCK,
        _XA_NET_WM_WINDOW_TYPE_TOOLBAR,
        _XA_NET_WM_WINDOW_TYPE_MENU,
        _XA_NET_WM_WINDOW_TYPE_UTIL,
        _XA_NET_WM_WINDOW_TYPE_SPLASH,
        _XA_NET_WM_WINDOW_TYPE_DIALOG,
        _XA_NET_WM_WINDOW_TYPE_NORMAL,
        //_XA_NET_WM_STATE,
        //_XA_NET_WM_STATE_MODAL,
        //_XA_NET_WM_STATE_STICKY,
        //_XA_NET_WM_STATE_MAXIMIZED_VERT,
        //_XA_NET_WM_STATE_MAXIMIZED_HORZ,
        //_XA_NET_WM_STATE_SHADED,
        //_XA_NET_WM_STATE_SKIP_TASKBAR,
        //_XA_NET_WM_STATE_SKIP_PAGER,
        // the next one is not in the spec but KDE use it
        //_XA_NET_WM_STATE_STAYS_ON_TOP,
        _XA_NET_WM_STRUT
        //_XA_NET_WM_ICON_GEOMETRY,
        //_XA_NET_WM_ICON,
        //_XA_NET_WM_PID,
        //_XA_NET_WM_HANDLED_ICONS,
        //_XA_NET_WM_PING
    };

    //Set supported NET protos
    XChangeProperty(fl_display, root,
                    _XA_NET_SUPPORTED, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)net_proto, CNT(net_proto));

    Window support_xid = XCreateSimpleWindow(fl_display, root, -200, -200, 5, 5, 0, 0, 0);

    //NET:
    XChangeProperty(fl_display, support_xid,
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&support_xid, 1);
    XChangeProperty(fl_display, root,
                    _XA_NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&support_xid, 1);

    protos_registered = true;
}

void Frame::get_protocols()
{
    unsigned long n;
    Atom* p = (Atom*)getProperty(_XA_WM_PROTOCOLS, XA_ATOM, &n);
    if(p) {
        DBG("get_protocols()");
        clear_flag(DELETE_WIN_PRT|TAKE_FOCUS_PRT);
        for(unsigned int i = 0; i < n; ++i) {
            if(p[i]==_XA_WM_DELETE_WINDOW) {
                set_frame_flag(DELETE_WIN_PRT);
            } else if(p[i]==_XA_WM_TAKE_FOCUS) {
                set_frame_flag(TAKE_FOCUS_PRT);
            }
        }
    }
    XFree((char*)p);
}

void Frame::get_wm_hints()
{
    if(wm_hints) {
        XFree((char *)wm_hints);
        wm_hints = 0;
    }
    wm_hints = XGetWMHints(fl_display, window_);
}

void Frame::update_wm_hints()
{
    if(!wm_hints) return;
    if((wm_hints->flags & InputHint) && !wm_hints->input)
        set_frame_flag(NO_FOCUS);
}
