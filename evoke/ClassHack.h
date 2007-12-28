/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __CLASSHACK_H__
#define __CLASSHACK_H__

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A stupid hack to extract value from 'class' member in XWindowAttributes structure.
 * Calling it in regular C++ translation unit will yield compilation error (sic)
 * so we must force it to be seen as regular C source.
 *
 * Does X developers ever heard for C++ language ?@#?#!@
 */

int get_attributes_class_hack(XWindowAttributes* attr);

#ifdef __cplusplus
}
#endif

#endif
