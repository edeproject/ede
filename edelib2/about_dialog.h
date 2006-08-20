/*
 * $Id$
 *
 * About dialog
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef _edelib_aboutdialog_h_
#define _edelib_aboutdialog_h_

#include <fltk/Window.h>
#include <fltk/InvisibleBox.h>
#include <fltk/Button.h>


namespace edelib {

void about_dialog(const char *progname, const char *progversion, const char *addcomment = 0);

}

#endif
