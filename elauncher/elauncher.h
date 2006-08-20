/*
 * $Id$
 *
 * Program and URL opener
 * Provides startup notification, crash handler and other features
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


#ifndef _ELAUNCHER_H_
#define _ELAUNCHER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include <fltk/Window.h>
#include <fltk/ReturnButton.h>
#include <fltk/TextDisplay.h>
#include <fltk/ask.h>
// for cursors:
//#include <fltk/x.h>
//#include <fltk/Enumerations.h>
// run dialog
#include <fltk/Image.h>
#include <fltk/xpmImage.h>
#include <fltk/Input.h>
#include <fltk/file_chooser.h>
#include <fltk/InvisibleBox.h>
#include <fltk/CheckButton.h>
#include <fltk/ReturnButton.h>
#include <fltk/run.h>
#include <fltk/events.h>
// other stuff
#include <fltk/filename.h>
#include <fltk/x.h>
//#include <fltk/Cursor.h>
#include <X11/cursorfont.h>

// This need to be last for Xlib.h errors
#include "../edelib2/NLS.h"
#include "../edelib2/Config.h"


// TODO: replace with edelib::Icon
#include "icons/run.xpm"
#include "icons/error.xpm"
#include "icons/crash.xpm"


#endif
