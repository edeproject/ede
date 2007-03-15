//  eglob.h
//
//  Copyright 2000-2001 Edscott Wilson Garcia
//  Copyright (C) 2001-2002 Martin Pekar
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef EGLOB_H_
#define EGLOB_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <efltk/Fl_Locale.h>
#include <efltk/Fl_Util.h>

int  process_find_messages();
void jam(char *file, Fl_Menu_ *);
void findCB();
void stopSearch();
void toggle_permission(long);

#endif

