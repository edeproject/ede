#include "Netwm.h"
#include "Frame.h"
#include "Winhints.h"
#include "Windowmanager.h"
#include "debug.h"

void NETWM::get_strut(Frame *f)
{
    unsigned long size = 0;
    CARD32 *val=0;

    val = (CARD32 *)getProperty(f->window(), _XA_NET_WM_STRUT, XA_CARDINAL, &size);
    if(!val) return;

    if ((size / (sizeof(CARD32))) != 4) {
        DBG("Window 0x%lx has wrong STRUT value (%d)\n", f->window(), size / (sizeof(CARD32)));
        XFree((char*)val);
        return;
    }

    if(!f->strut_) f->strut_ = new Fl_Rect();

    int l=val[0];
    int r=val[1];
    int t=val[2];
    int b=val[3];
    f->strut_->set(l, t, l+r, t+b);

    XFree((char*)val);
}

bool NETWM::get_window_type(Frame *f)
{
    unsigned long size = 0;
    Atom *val=0;
    int ret=0;
    int wintype = TYPE_NORMAL;

    val = (Atom *)getProperty(f->window(), _XA_NET_WM_WINDOW_TYPE, XA_ATOM, &size, &ret);
    if(!val || ret!=Success) {
        f->window_type(TYPE_NORMAL);
        return false;
    }

    for(uint i = 0; i < (size / (sizeof(Atom))); i++)
    {
        if (val[i] == _XA_NET_WM_WINDOW_TYPE_DOCK)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_DOCK\n");
            wintype = TYPE_DOCK;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_TOOLBAR)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_TOOLBAR\n");
            wintype = TYPE_TOOLBAR;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_MENU)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_MENU\n");
            wintype = TYPE_MENU;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_UTIL)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_UTIL\n");
            wintype = TYPE_UTIL;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_DIALOG)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_DIALOG\n");
            wintype = TYPE_DIALOG;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_NORMAL)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_NORMAL\n");
            wintype = TYPE_NORMAL;
            break;
        }
        else if (val[i] == _XA_NET_WM_WINDOW_TYPE_DESKTOP)
        {
            DBG("_XA_NET_WM_WINDOW_TYPE_DESKTOP\n");
            wintype = TYPE_DESKTOP;
            break;
        } else {
            DBG("Unknown NETWM window type 0x%08lx\n", val[i]);
        }

    } /* for */

    XFree((char*)val);

    f->window_type(wintype);
    return true;
}

void NETWM::set_active_window(Window win)
{
    //Set NET-WM active window
    XChangeProperty(fl_display, root_win, _XA_NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&win, 1);
}

char *NETWM::get_title(Frame *f)
{
    int ret=0;
    char *title = (char*)getProperty(f->window(), _XA_NET_WM_NAME, _XA_UTF8_STRING, 0, &ret);
    if(!title || ret!=Success) {
        return 0;
    }
    return title;
}

