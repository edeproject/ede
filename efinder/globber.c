
/* globber.c */
/* 
   Copyright 2000-2001 Edscott Wilson Garcia

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

#include "globber.h"

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

/** tripas **/
/* private */
#ifndef GLOB_TILDE
#define GLOB_TILDE 0x0
#endif
#ifndef GLOB_ONLYDIR
#define GLOB_ONLYDIR 0x0
#endif	

#define MONTH_T 2628000
#define DAY_T 86400
#define HOUR_T 3600
#define MIN_T 60

typedef struct objeto_globber {
	int options; 
	int type;    
	int user;
	int group;
	long unsigned sizeG;   
	long unsigned sizeL;   
	long unsigned month_t;
	long unsigned day_t;
	long unsigned hour_t;
	long unsigned min_t;
/* private variables, not to be duplicated on recursion: */
	struct stat *stinit;
	struct stat st;
	int pass;
        time_t tiempo;
        time_t actual;
	int dostat;
} objeto_globber;


static int display(char *input){
		printf("%s\n",input); /*fflush(NULL);*/
			return 0;
}

#define DO_CHECK_PARAM if (!address) return 0;	else objeto = (objeto_globber *)address;
/* public */
int glob_clear_options(void *address){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	objeto->stinit=NULL;
	objeto->user=-1, 
	objeto->group=-1, 
	objeto->options=0x0, 
	objeto->sizeG=0x0, 
	objeto->sizeL=0x0, 
	objeto->type=0x0,
	objeto->month_t=0x0, 
	objeto->day_t=0x0, 
	objeto->hour_t=0x0;
	objeto->min_t=0x0;
	objeto->pass=0x0;
	objeto->dostat=0x0;
	return 1;
}

void *globber_create(void){
	objeto_globber *objeto;
	objeto=(objeto_globber *)malloc(sizeof(objeto_globber));
        glob_clear_options((void *)objeto);
	return (void *)objeto;
}
	
void *globber_destroy(void *address){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	if (address) free(address);
	if (objeto->stinit) free(objeto->stinit);
	return NULL;
}
		
int glob_set_options(void *address,int options){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	objeto->options |= options;
	return 1;
}

int glob_set_type(void *address,int type){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	objeto->type=type;
	return 1;
}

int glob_set_sizeG(void *address,long unsigned size){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
        glob_set_options(objeto,GLOBBER_SIZE);	      
	objeto->sizeG=size;
	return 1;
}
int glob_set_sizeL(void *address,long unsigned size){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	objeto->sizeL=size;
	return 1;
}
int glob_set_user(void *address,int user){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
        glob_set_options(objeto,GLOBBER_USER);
	objeto->user=user;
	return 1;
}
int glob_set_group(void *address,int group){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	glob_set_options(objeto,GLOBBER_GROUP);
	objeto->group=group;
	return 1;
}


int glob_set_time(void *address,long unsigned month_t,long unsigned day_t,
		long unsigned hour_t,long unsigned min_t){
	objeto_globber *objeto;
	DO_CHECK_PARAM;
	objeto->month_t=month_t;
	objeto->day_t=day_t;
	objeto->hour_t=hour_t;
	objeto->min_t=min_t;
	return 1;
}


/* if the user defined "operate" function returns TRUE, Globber will exit 
 * and return to calling module with the same return value  */


int globber(void *address,char *path,int (*operate)(char *),char *filter) {
	/* these variables must be kept on the heap */
	glob_t  dirlist;
	int i;
	char *globstring; 
	objeto_globber *object;

	if (!address) 	object= (objeto_globber *)globber_create();
	else object = (objeto_globber *)address;

		
	if (object->options&GLOBBER_VERBOSE) fprintf(stderr,"path= %s\n",path);
	if (object->options&GLOBBER_TIME) {
	  if (object->options&GLOBBER_MTIME) 
	    object->options &=((GLOBBER_CTIME|GLOBBER_ATIME)^0xffffffff);	
	  else if (object->options&GLOBBER_CTIME)
	    object->options &=(GLOBBER_ATIME^0xffffffff); 
	}
	
	dirlist.gl_offs=2;
	if (!operate) operate=display;

	if (filter){
		globstring = (char *)malloc(strlen(path)+strlen(filter)+2);
		strcpy(globstring,path);
		if (path[strlen(path)-1]!='/') strcat(globstring,"/");
		strcat(globstring,filter);
	} else globstring = path;
	
	if (glob(globstring,GLOB_ERR|GLOB_TILDE,NULL,&dirlist) != 0) {
		if (object->options&GLOBBER_VERBOSE) fprintf(stderr,"%s: no match\n",globstring);
	}
	else for (i=0;i<dirlist.gl_pathc;i++) {
	 if (object->options&GLOBBER_STAT) {
	  lstat(dirlist.gl_pathv[i],&(object->st));
	     if (object->options&GLOBBER_USER){
	      if (object->user != object->st.st_uid) 
		      continue;	
	     } 
	     if (object->options&GLOBBER_GROUP){
	      if (object->group != object->st.st_gid) 
		      continue;
	     }
	  if (object->options&GLOBBER_TIME){
	     object->actual=time(NULL);
	     if (object->options&GLOBBER_MTIME) object->tiempo=object->st.st_mtime; 
	     if (object->options&GLOBBER_ATIME) object->tiempo=object->st.st_atime; 
	     if (object->options&GLOBBER_CTIME) object->tiempo=object->st.st_ctime; 
	     if ((object->min_t > 0) && ((object->actual-object->tiempo)/MIN_T > object->min_t)) 
		     continue;
	     if ((object->hour_t > 0) && ((object->actual-object->tiempo)/HOUR_T > object->hour_t)) 
		     continue;
	     if ((object->day_t > 0) && ((object->actual-object->tiempo)/DAY_T > object->day_t)) 
		     continue;
	     if ((object->month_t > 0) && ((object->actual-object->tiempo)/MONTH_T > object->month_t)) 
		     continue;
	  }
	  if (object->options&GLOBBER_SIZE){
 	     if ((object->sizeL > 0)&&(object->st.st_size > object->sizeL)) 
		     continue;
	     if (object->st.st_size < object->sizeG) 
		     continue;
	  } 
	  if (object->options&GLOBBER_PERM){
	     if ((object->st.st_mode & 07777) & (object->type & 07777));
	     else {
	       if ((object->st.st_mode & 07777)==(object->type & 07777)); 
	       else continue;
	     }
	  }
	 
	  if (object->options&GLOBBER_TYPE) {
	     if ((object->st.st_mode & S_IFMT)!=(object->type & S_IFMT)) 
		     continue;
	  }
	 } /* done lstat'ing */
	 
	 if ((object->pass=(*(operate))(dirlist.gl_pathv[i]))!=0) break;
	} 
	if (filter) free(globstring);
	globfree(&dirlist);
	if (object->pass) {
		if (object->stinit) {free(object->stinit); object->stinit=NULL;}
		return (object->pass); /* error returned from function */
	}
	
	if (object->options&GLOBBER_RECURSIVE) {
		globstring = (char *)malloc(strlen(path)+3);
		strcpy(globstring,path);
		strcat(globstring,(globstring[strlen(globstring)-1]=='/')?"*":"/*");
		if (glob(globstring,GLOB_ERR|GLOB_ONLYDIR|GLOB_TILDE,NULL,&dirlist) != 0) {
			if (object->options&GLOBBER_VERBOSE) fprintf(stderr,"%s: no match\n",globstring);
		}
		else for (i=0;i<dirlist.gl_pathc;i++) {
			lstat(dirlist.gl_pathv[i],&(object->st));
			if ((object->st.st_mode & S_IFMT)!=S_IFDIR) continue; /* dont follow non-dirs. */
			if ((object->st.st_mode & S_IFMT)==S_IFLNK) continue; /* dont follow symlinks */
			
			if (object->options&GLOBBER_XDEV){
			       if (object->stinit==NULL) {
			       		object->stinit=(struct stat *) malloc(sizeof (struct stat));
					lstat(dirlist.gl_pathv[i],object->stinit);
			       }
				else {
					if (object->st.st_dev != object->stinit->st_dev) continue; 
					/* dont leave filesystem */
				}
			}	
			if (object->options&GLOBBER_VERBOSE)
				fprintf(stderr,"directory: %s \n",dirlist.gl_pathv[i]); 
			object->pass=globber(address,dirlist.gl_pathv[i],operate,filter);
			if (object->pass) break;
		}
		free(globstring);
	       	globfree(&dirlist);
	}
	
	if (object->stinit) {free(object->stinit);object->stinit=NULL;}
	return (object->pass);
}



