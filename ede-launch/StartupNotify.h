/*
 * $Id: ede-launch.cpp 3112 2011-10-24 09:02:36Z karijes $
 *
 * ede-launch, launch external application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2011 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __STARTUPNOTIFY_H__
#define __STARTUPNOTIFY_H__

struct StartupNotify;

StartupNotify *startup_notify_start(const char *progam, const char *icon);
void           startup_notify_end(StartupNotify *s);

#endif
