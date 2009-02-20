#include "Mwm.h"
#include "Frame.h"
#include "Winhints.h"
#include "Windowmanager.h"

void MWM::get_hints(Frame *f)
{
    if(f->mwm_hints) {
        XFree((char *)f->mwm_hints);
        f->mwm_hints = 0;
    }

    unsigned long n=0;
    f->mwm_hints = (MwmHints *)getProperty(f->window(), _XATOM_MWM_HINTS, _XATOM_MWM_HINTS, &n);

    if(n >= PROP_MWM_HINTS_ELEMENTS) {
        // Fill in the default value for missing fields:
        if(!(f->mwm_hints->flags & MWM_HINTS_FUNCTIONS))
            f->mwm_hints->functions = MWM_FUNC_ALL;
        if(!(f->mwm_hints->flags & MWM_HINTS_DECORATIONS))
            f->mwm_hints->decorations = MWM_DECOR_ALL;
    } else {
        XFree((char *)f->mwm_hints);
        f->mwm_hints = 0;
    }
}

static long mwm_functions(MwmHints *hints, XSizeHints *sh)
{
    long functions = ~0U;

    if(hints && (hints->flags & MWM_HINTS_FUNCTIONS))
    {
        if(hints->functions & MWM_FUNC_ALL)
            functions = ~hints->functions;
        else
            functions = hints->functions;

    } else {

        if(sh) {
            bool minmax = false;
            if(sh->min_width == sh->max_width && sh->min_height == sh->max_height) {
                functions &= ~MWM_FUNC_RESIZE;
                minmax = true;
            }
            if((minmax && !(sh->flags & PResizeInc)) ||
               (sh->width_inc == 0 && sh->height_inc == 0))
                functions &= ~MWM_FUNC_MAXIMIZE;
        }
    }
    functions &= (MWM_FUNC_RESIZE | MWM_FUNC_MOVE |
                  MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE |
                  MWM_FUNC_CLOSE);
    return functions;
}

static long mwm_decors(MwmHints *hints, XSizeHints *sh)
{
    long decors = ~0U;
    long func = mwm_functions(hints, sh);

    if(hints && (hints->flags & MWM_HINTS_DECORATIONS)) {
        if(hints->decorations & MWM_DECOR_ALL)
            decors = ~hints->decorations;
        else
            decors = hints->decorations;

    } else {

        if(sh) {
            bool minmax = false;
            if(sh->min_width == sh->max_width && sh->min_height == sh->max_height) {
                decors &= ~MWM_DECOR_RESIZEH;
                minmax = true;
            }
            if((minmax && !(sh->flags & PResizeInc)) ||
               (sh->width_inc == 0 && sh->height_inc == 0))
                decors &= ~MWM_DECOR_MAXIMIZE;
        }
    }
    decors &= (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH |
               MWM_DECOR_TITLE | MWM_DECOR_MENU |
               MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE);

    // Add disabled buttons !!!
    decors &=
        ~(/*((func & MWM_FUNC_RESIZE) ? 0 : MWM_DECOR_RESIZEH) |*/
          ((func & MWM_FUNC_MINIMIZE) ? 0 : MWM_DECOR_MINIMIZE) |
          ((func & MWM_FUNC_MAXIMIZE) ? 0 : MWM_DECOR_MAXIMIZE));

    return decors;
}

bool MWM::update_hints(Frame *f)
{
    MwmHints *mwm_hints = f->mwm_hints;
    XSizeHints *size_hints = f->size_hints;

    long decors = mwm_decors(mwm_hints, size_hints);
    long functions = mwm_functions(mwm_hints, size_hints);

    bool funcs_only = (mwm_hints &&
                       (mwm_hints->flags & (MWM_HINTS_FUNCTIONS) == MWM_HINTS_FUNCTIONS) &&
                       (mwm_hints->flags & (MWM_HINTS_DECORATIONS) != MWM_HINTS_DECORATIONS)
                      );

    // The low bit means "turn the marked items off", invert this.
    // Transient windows already have size & iconize buttons turned off:
    if(functions & MWM_FUNC_ALL)
        functions = ~functions & (f->transient_for_xid ? ~0x58 : -1);
    if(decors & MWM_DECOR_ALL)
        decors = ~decors & (f->transient_for_xid ? ~0x60 : -1);

    int old_decor_flags = f->decor_flags();
    int old_func_flags  = f->func_flags();
    int old_frame_flags = f->frame_flags();

    // Get decorations
    f->clear_decor_flags();
    if(decors & MWM_DECOR_BORDER)
        f->set_decor_flag(BORDER);
    if(decors & MWM_DECOR_TITLE)
        f->set_decor_flag(TITLEBAR);
    if(decors & MWM_DECOR_MENU)
        f->set_decor_flag(SYSMENU);
    if(decors & MWM_DECOR_MINIMIZE)
        f->set_decor_flag(MINIMIZE);
    if(decors & MWM_DECOR_MAXIMIZE)
        f->set_decor_flag(MAXIMIZE);
    if(decors & MWM_DECOR_RESIZEH)
        f->set_decor_flag(RESIZE);

    // Get functions
    f->clear_func_flags();
    if(functions & MWM_FUNC_RESIZE) {
        f->set_func_flag(RESIZE);
        if(funcs_only) f->set_decor_flag(RESIZE|BORDER);
    }
    if(functions & MWM_FUNC_MOVE) {
        f->set_func_flag(MOVE);
        if(funcs_only) f->set_decor_flag(BORDER);
    }
    if(functions & MWM_FUNC_MINIMIZE) {
        f->set_func_flag(MINIMIZE);
        if(funcs_only) f->set_decor_flag(MINIMIZE);
    }
    if(functions & MWM_FUNC_MAXIMIZE) {
        f->set_func_flag(MAXIMIZE);
        if(funcs_only) f->set_decor_flag(MAXIMIZE);
    }
    if(functions & MWM_FUNC_CLOSE) {
        f->set_func_flag(CLOSE);
        f->set_decor_flag(CLOSE);
    }

    // some Motif programs use this to disable resize :-(
    // and some programs change this after the window is shown (*&%$#%)
    if(!(functions & MWM_FUNC_RESIZE) || !(decors & MWM_DECOR_RESIZEH))
        f->clear_decor_flag(RESIZE);
    else
        f->set_decor_flag(RESIZE);

    // and some use this to disable the Close function.  The commented
    // out test is it trying to turn off the mwm menu button: it appears
    // programs that do that still expect Alt+F4 to close them, so I
    // leave the close on then:
    if (!(functions & 0x20) /*|| !(prop[2]&0x10)*/) {
        f->clear_func_flag(CLOSE);
        f->clear_decor_flag(CLOSE);
    } else {
        f->set_func_flag(CLOSE);
        f->set_decor_flag(CLOSE);
    }

    // Set modal
    if(!f->shown() && mwm_hints && (mwm_hints->flags & MWM_HINTS_INPUT_MODE) && (mwm_hints->input_mode != MWM_INPUT_MODELESS))
        f->set_frame_flag(MODAL);

    return ((f->frame_flags() ^ old_frame_flags) || (f->decor_flags() ^ old_decor_flags) || (f->func_flags() ^ old_func_flags));
}

void MWM::set_motif_info()
{
    MotifWmInfo mwminfo;
    mwminfo.flags = MWM_INFO_STARTUP_CUSTOM;
    mwminfo.wm_window = root_win;
    XChangeProperty(fl_display, root_win, _XATOM_MOTIF_WM_INFO, _XATOM_MOTIF_WM_INFO,
                    32, PropModeReplace,
                    (unsigned char *)&mwminfo, 2);
}
