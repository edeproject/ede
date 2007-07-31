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

#ifndef __LOGOUT_H__
#define __LOGOUT_H__

#define LOGOUT_CANCEL   0
#define LOGOUT_LOGOUT   1
#define LOGOUT_RESTART  2
#define LOGOUT_SHUTDOWN 3

int logout_dialog(int screen_w, int screen_h, bool disable_restart = 0, bool disable_shutdown = 0);

#endif
