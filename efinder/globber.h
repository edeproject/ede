
/* globber.h */
/* 
   Copyright 2000 Edscott Wilson Garcia

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/*****************************************************************/

/* version 0.5.0 is object oriented and eliminates sharing of
*  global variables with other modules. */

/* globber in its own .o file and link it in later:*/

#define GLOBBER_VERSION 0.5.0

int globber(void *address,char *input,int (*operate)(char *),char *filter);
int glob_clear_options(void *address);
void *globber_create(void);
void *globber_destroy(void *);
int glob_set_options(void *address,int options);
int glob_set_type(void *address,int type);
int glob_set_sizeL(void *address,long unsigned size);
int glob_set_sizeG(void *address,long unsigned size);
int glob_set_user(void *address,int user);
int glob_set_group(void *address,int group);
int glob_set_time(void *address,long unsigned month_t,long unsigned day_t,
		long unsigned hour_t,long unsigned min_t);

#ifdef __GLOBBER_INCLUDES__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef HAVE_SNPRINTF
#  include "snprintf.h"
#endif
#ifdef DMALLOC
#  include "dmalloc.h"
#endif
#endif /* __GLOBBER_C__ */



#define GLOBBER_RECURSIVE    0x01
#define GLOBBER_VERBOSE      0x02
#define GLOBBER_XDEV         0x04
#define GLOBBER_SIZE         0x08

#define GLOBBER_MTIME        0x10
#define GLOBBER_ATIME        0x20
#define GLOBBER_CTIME        0x40
/*GLOBBER_MTIME|GLOBBER_ATIME|GLOBBER_CTIME :*/
#define GLOBBER_TIME         0x70 
#define GLOBBER_PERM         0x80
#define GLOBBER_TYPE         0x100
#define GLOBBER_USER         0x200
#define GLOBBER_GROUP        0x400
/*  GLOBBER_XDEV | GLOBBER_SIZE | GLOBBER_TIME | GLOBBER_PERM |  
*   GLOBBER_TYPE | GLOBBER_USER | GLOBBER_GROUP : */
#define GLOBBER_STAT         0xffc




