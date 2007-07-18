/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

void edewm_log(const char* str, ...);
void edewm_printf(const char* str, ...);
void edewm_warning(const char* str, ...);
void edewm_fatal(const char* str, ...);

#define ELOG     edewm_log
#define EWARNING edewm_warning
#define EFATAL   edewm_fatal
#define EPRINTF  edewm_printf

#endif
