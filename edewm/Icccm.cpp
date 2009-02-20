#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Icccm.h"
#include "Frame.h"
#include "Winhints.h"
#include "Windowmanager.h"

static void icccm_send_state(Window wid, long state)
{
    long data[2];
    data[0] = state;
    data[1] = None;
    XChangeProperty(fl_display, wid, _XA_WM_STATE, _XA_WM_STATE,
                    32, PropModeReplace, (unsigned char *)data, 2);
}

void ICCCM::state_iconic(Frame *f)
{
    icccm_send_state(f->window(), IconicState);
}

void ICCCM::state_normal(Frame *f)
{
    icccm_send_state(f->window(), NormalState);
}

void ICCCM::state_withdrawn(Frame *f)
{
    icccm_send_state(f->window(), WithdrawnState);
}

bool ICCCM::get_size(Frame *f, int &w, int &h)
{
    bool ret=false;
    int i, j;
    double aspect;

    XSizeHints *sh = f->size_hints;

    int real_minw = (f->decor_flag(BORDER) || f->decor_flag(THIN_BORDER)) ? 100 : 0;
    int minw = max(sh->min_width + f->offset_w, real_minw);
    int minh = sh->min_height+ f->offset_h;

    if(w > sh->max_width) { w = sh->max_width; ret = true; }
    if(h > sh->max_height) { h = sh->max_height; ret = true; }
    if(w < minw) { w = minw; ret = true; }
    if(h < minh) { h = minh; ret = true; }

    if( w>0 && h>0 )
    {
        w -= sh->base_width;
        h -= sh->base_height;
        if( w>0 && h>0 )
        {
            if(f->frame_flag(KEEP_ASPECT)) {
                aspect = ((double)w) / ((double)h);
                if (aspect < f->aspect_min)
                    w = (int)((double)h * f->aspect_min);
                if (aspect > f->aspect_max)
                    h = (int)((double)w / f->aspect_max);
            }
            i = w / sh->width_inc;
            j = h / sh->height_inc;
            w = i * sh->width_inc;
            h = j * sh->height_inc;
        }
        w += sh->base_width;
        h += sh->base_height;
    }
    return ret;
}

void ICCCM::configure(Frame *f)
{
    XConfigureEvent ce;
    ce.send_event = True;
    ce.display = fl_display;
    ce.type   = ConfigureNotify;
    ce.event  = f->window();
    ce.window = f->window();
    ce.x = f->x()+f->offset_x;
    ce.y = f->y()+f->offset_y;
    ce.width  = f->w()-f->offset_w;
    ce.height = f->h()-f->offset_h;
    ce.border_width = f->box()->dx();
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(fl_display, f->window(), False, StructureNotifyMask, (XEvent*)&ce);
}

void ICCCM::set_iconsizes()
{
    XIconSize *is = XAllocIconSize();

    is->min_width = 8;
    is->min_height = 8;
    is->max_width = 48;
    is->max_height = 48;
    is->width_inc = 1;
    is->height_inc = 1;
    XSetIconSizes(fl_display, root_win, is, 1);

    XFree(is);
}

char *ICCCM::get_title(Frame *f)
{
    XTextProperty xtp;
    char *title = 0;

    if(XGetWMName(fl_display, f->window(), &xtp))
    {
        if(xtp.encoding == XA_STRING) {
            title = strdup((const char*)xtp.value);
        } else {
#if HAVE_X11_UTF_TEXT_PROP
            int items;
            char **list=0;
            Status s;
            s = Xutf8TextPropertyToTextList(fl_display, &xtp, &list, &items);
            if((s == Success) && (items > 0)) {
                title = strdup((const char *)*list);
            } else
                title = strdup((const char *)xtp.value);
            if(list) XFreeStringList(list);
#else
            title = strdup((const char*)xtp.value);
#endif
        }
        XFree(xtp.value);
    }
    return title;
}

// Read the sizeHints to size_hints object
// Returns true if autoplace should be done.
bool ICCCM::size_hints(Frame *f)
{
    long supplied;

    XSizeHints *size_hints = f->size_hints;

    if(!XGetWMNormalHints(fl_display, f->window(), size_hints, &supplied))
        size_hints->flags = 0;

    if(size_hints->flags & PResizeInc) {
        if(size_hints->width_inc < 1)  size_hints->width_inc = 1;
        if(size_hints->height_inc < 1) size_hints->height_inc = 1;
    } else
        size_hints->width_inc = size_hints->height_inc = 1;

    if(!(size_hints->flags & PBaseSize)) {
        if (size_hints->flags & PMinSize) {
            size_hints->base_width  = size_hints->min_width;
            size_hints->base_height = size_hints->min_height;
        } else
            size_hints->base_width = size_hints->base_height = 0;
    }

    if(!(size_hints->flags & PMinSize)) {
        size_hints->min_width  = size_hints->base_width;
        size_hints->min_height = size_hints->base_height;
    }

    if(!(size_hints->flags & PMaxSize)) {
        size_hints->max_width = 32767;
        size_hints->max_height = 32767;
    }

    if (size_hints->min_height <= 0)
        size_hints->min_height = 1;
    if (size_hints->min_width <= 0)
        size_hints->min_width = 1;

    if(size_hints->max_width<size_hints->min_width || size_hints->max_width<=0)
        size_hints->max_width = 32767;
    if(size_hints->max_height<size_hints->min_height || size_hints->max_height<=0)
        size_hints->max_height = 32767;

    if(!(size_hints->flags & PWinGravity)) {
        size_hints->win_gravity = NorthWestGravity;
        size_hints->flags |= PWinGravity;
    }

    if(size_hints->flags & PAspect) {
        f->set_frame_flag(KEEP_ASPECT);
        if((size_hints->min_aspect.y > 0.0) && (size_hints->min_aspect.x > 0.0)) {
            f->aspect_min = ((double)size_hints->min_aspect.x) / ((double)size_hints->min_aspect.y);
        } else {
            f->aspect_min = 0.0;
        }
        if((size_hints->max_aspect.y > 0.0) && (size_hints->max_aspect.x > 0.0)) {
            f->aspect_max = ((double)size_hints->max_aspect.x) / ((double)size_hints->max_aspect.y);
        } else {
            f->aspect_max = 32767.0;
        }
    } else {
        f->aspect_min = 0.0;
        f->aspect_max = 32767.0;
    }

    // fix for old gimp, which sets PPosition to 0,0:
    // if(f->x() <= 0 && f->y() <= 0) size_hints->flags &= ~PPosition;

    return !(size_hints->flags & (USPosition|PPosition));
}

