#ifndef _MWM_H_
#define _MWM_H_

#include <X11/Xmd.h>
#include <X11/Xlib.h>
/* This file is part of the GNU LessTif Library. */

typedef struct {
    long flags;
    Window wm_window;
} MotifWmInfo;

#define MWM_INFO_STARTUP_STANDARD	(1L<<0)
#define MWM_INFO_STARTUP_CUSTOM		(1L<<1)

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MwmHints;

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

#define MWM_TEAROFF_WINDOW	(1L<<0)

/*
 * atoms
 */
#define _XA_MOTIF_BINDINGS		"_MOTIF_BINDINGS"
#define _XA_MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"
#define _XA_MOTIF_WM_MESSAGES		"_MOTIF_WM_MESSAGES"
#define _XA_MOTIF_WM_OFFSET		"_MOTIF_WM_OFFSET"
#define _XA_MOTIF_WM_MENU		"_MOTIF_WM_MENU"
#define _XA_MOTIF_WM_INFO		"_MOTIF_WM_INFO"
#define _XA_MWM_HINTS			_XA_MOTIF_WM_HINTS
#define _XA_MWM_MESSAGES		_XA_MOTIF_WM_MESSAGES
#define _XA_MWM_MENU			_XA_MOTIF_WM_MENU
#define _XA_MWM_INFO			_XA_MOTIF_WM_INFO

#define PROP_MOTIF_WM_HINTS_ELEMENTS 5
#define PROP_MWM_HINTS_ELEMENTS PROP_MOTIF_WM_HINTS_ELEMENTS

////////////////////////////////////

class Frame;

class MWM
{
public:
    static void get_hints(Frame *f);
    static bool update_hints(Frame *f);

    static void set_motif_info();
};

#endif

