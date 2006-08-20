
/* glob.c file filter for grep.*/
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

#include "globber.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
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

/** tripas */
#define VERSION_NAME "\nglob 0.5.0\n\nCopyright 2000-2001 Edscott Wilson Garcia\n\
This is free software; see the source for copying conditions. There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"

#define GREP "grep"

static void *object=NULL;
static int initial;
static int terminated = 0;
static char *token;
static int options=0,type=0;
static long size=0;
static long month_t=0;
static long unsigned day_t=0;
static long unsigned hour_t=0;
static long unsigned min_t=0;

#define GLOBRUN_PID     	0x01
#define GLOBRUN_COUNT   	0x02
#define GLOBRUN_FILTERED   	0x04
#define GLOBRUN_IGNORE_CASE	0x08
#define GLOBRUN_REG_EXP  	0x10
#define GLOBRUN_INVERT       	0x20
/*#define GLOBRUN_WHATEVER        	0x40*/
#define GLOBRUN_WORDS_ONLY   	0x80
#define GLOBRUN_LINES_ONLY   	0x100
#define GLOBRUN_ZERO_BYTE    	0x200
#define GLOBRUN_NOBINARIES   	0x400
#define GLOBRUN_RECURSIVE    	0x800
#define GLOBRUN_VERBOSE     	0x1000
#define GLOBRUN_XDEV         	0x2000

#define MAX_ARG 25

static int display (char *input)
{
  if (terminated) return terminated; /* die quietly and quickly */
  printf ("%s\n", input);
  if (time (NULL) - initial > 3) {
      fflush (NULL);
      initial = time (NULL);
  }
  return terminated;
}

static int grep (char *file)
{
  static char *arguments[MAX_ARG];
  int status = 0;
  if (terminated) return terminated; /* die quietly and quickly */

  arguments[status++] = "grep";
  arguments[status++] = "-d";
  arguments[status++] = "skip";
  arguments[status++] = "-H";
  if (options & GLOBRUN_NOBINARIES)
    arguments[status++] = "-I";
  if (options & GLOBRUN_IGNORE_CASE)
    arguments[status++] = "-i";
  if (options & GLOBRUN_WORDS_ONLY)
    arguments[status++] = "-w";
  if (options & GLOBRUN_LINES_ONLY)
    arguments[status++] = "-x";
  if (options & GLOBRUN_ZERO_BYTE)
    arguments[status++] = "-Z";

  if ((options & GLOBRUN_COUNT) && (options & GLOBRUN_INVERT))
    {
      arguments[status++] = "-c";
      arguments[status++] = "-v";
    }
  if ((options & GLOBRUN_COUNT) && !(options & GLOBRUN_INVERT))
    {
      arguments[status++] = "-c";
    }
  if (!(options & GLOBRUN_COUNT) && (options & GLOBRUN_INVERT))
    {
      arguments[status++] = "-L";
    }
  if (!(options & GLOBRUN_COUNT) && !(options & GLOBRUN_INVERT))
    {
      arguments[status++] = "-l";
    }

  if (options & GLOBRUN_REG_EXP)
    arguments[status++] = "-E";
  else
    arguments[status++] = "-e";
  arguments[status++] = token;

  arguments[status++] = file;
  arguments[status++] = (char *) 0;
  if (options & GLOBRUN_VERBOSE)
    {
      int i;
      for (i = 0; i < status; i++)
	printf ("%s ", arguments[i]);
      printf ("\n");
    }

  if (fork () == 0){
    execvp (GREP, arguments);
    fprintf(stderr,"%s not found in path!\n",GREP);
    exit(1);
  }
  wait (&status);

  /*fflush(NULL); */
  return terminated;
}


static char *message[] = {
  " [-vVPrMACaiIyLcwxZ] [-fpotkhsmudgeE (option)] path \n\n",
  "options: \n"
    "          [-r] [-v] [-d ddd] [-m mmm] [-f filter] [-s (+/-)size]\n",
  "          [-t type] [-p perm] [grep options...] \n",
  "-v            = verbose\n",
  "-V            = print version number information\n",
  "-a            = stay on a single filesystem.\n",
  "-P            = print process id (capital P)\n",
  "-f filter     = file filter (enclosed in quotes if regexp *,? or\n",
  "                [] is used)\n",
  "-r            = recursive\n",
  "-s +kbytes    = size greater than kbytes KBYTES\n",
  "-s -kbytes    = size less than kbytes KBYTES\n",
  "-p perm       = perm is either suid | exe\n",
  "-o octal_mode = octal mode is the file mode in octal notation\n",
  "-t type       = any | reg | dir | sym | sock | blk | chr | fifo\n",
  "                (any, regular, directory, symlink, socket, blk_dev,\n",
  "                 chr_dev, fifo: any is the default.)\n",
  "  * Time options must be used with either  -M, -C, or -A.\n"
  "-k min        = file time in the previous (int) min minutes (either -M -C -A)\n",
  "-h hhh        = file time in the previous (int) hh hours (either -M -C -A)\n",
  "-d ddd        = file time in the previous (int) dd days (either -M -C -A)\n",
  "-m mmm        = file time in the previous (int) mm months (either -M -C -A)\n",
  "-M            = use mtime for file (modification time: mknod, truncate,\n",
  "                utime,write  \n",
  "-A            = use atime for file (access time: exec, mknod, pipe,\n",
  "                utime, read)  \n",
  "-C            = use ctime for file (change time: setting inode information\n",
  "                i.e., owner, group, link count, mode, etc.) \n",
  "-u user-id    = only files matching numeric user-id\n",
  "-g group-id   = only files matching numeric group-id\n",
  "-Z            = Output  a  zero  byte  (the  ASCII  NUL  character)\n",
  "                instead of the character that  normally  follows  a\n",
  "                file  name (never tested option, if you do, email me)\n",
    "\n",
  "**specifying these option will be used in content search (grep):\n",
  "-e string     = containing string (if *,? or [], use quotes)\n",
  "-E regexp     = containing regexp: (use quotes amigo). \n",
  "-i            = ignore case (for search string -c)\n",
  "-I            = do not search into binary files\n",
  "-y            = same as -i (obsolete)\n",
  "-L            = print the  name  of each input file from which *no*\n",
  "                output would normally have been printed.\n",
  "-c            = only print a count of matching lines for each input\n",
  "                file.\n",
  "-w            = Select  only  those  lines  containing matches that\n",
  "               form whole words. Word-constituent  characters  are\n",
  "               letters, digits, and the underscore.\n",
  "-x            = Select only those matches that  exactly  match  the\n",
  "                whole line.\n",
  "\n",
  NULL
};

void
finish (int sig)
{
  /*printf("\n****\nglob terminated by signal\n****\n"); */
  terminated = 1;
  fflush (NULL);
}

void
halt (int sig)
{
  fflush (NULL);
  globber_destroy(object);
  exit (1);
}


#define CHECK_ARG if (argc <= i) goto error;
int
main (int argc, char **argv)
{
  int i,timetype=0;
  char *filter = NULL, globbered = 0;
  int (*operate) (char *) = display;
  initial = time (NULL);

  /* initializations  */
  signal (SIGHUP, halt);
  signal (SIGSEGV, finish);
  signal (SIGKILL, finish);
  signal (SIGTERM, finish);


  if (argc < 2)
    {
    error:
      fprintf (stdout, "use:  %s ", argv[0]);
      i = 0;
      while (message[i])
	fprintf (stdout,"%s", message[i++]);
      exit (1);
    }
  object=globber_create();
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
	{
		/* options for the globber : *****************/
	  if (strstr (argv[i], "M") != NULL)
	    {
	      timetype=1;
	      glob_set_options(object,GLOBBER_MTIME);
	      continue;
	    }
	  if (strstr (argv[i], "A") != NULL)
	    {
	      timetype=1;
	      glob_set_options(object,GLOBBER_ATIME);
	      continue;
	    }
	  if (strstr (argv[i], "C") != NULL)
	    {
	      timetype=1;
	      glob_set_options(object,GLOBBER_CTIME);
	      continue;
	    }
	  if (strstr (argv[i], "a") != NULL)
	    {
	      glob_set_options(object,GLOBBER_XDEV);
	      options |= GLOBRUN_XDEV;
	      continue;
	    }
	  if (strstr (argv[i], "v") != NULL)
	    {
	      glob_set_options(object,GLOBBER_VERBOSE);
	      options |= GLOBRUN_VERBOSE;
	      continue;
	    }
	  if (strstr (argv[i], "r") != NULL)
	    {
	      glob_set_options(object,GLOBBER_RECURSIVE);
	      options |= GLOBRUN_RECURSIVE;
	      continue;
	    }
	  if (strstr (argv[i], "u") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      glob_set_user(object,atol(argv[i]));
	      continue;
	    }
	  if (strstr (argv[i], "g") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      glob_set_group(object,atol(argv[i]));
	      continue;
	    }

	  
	  if (strstr (argv[i], "t") != NULL)
	    {
	      i++;
	      type &= 07777; 
	      CHECK_ARG;
	      /*if (strcmp (argv[i], "any") == 0) type &= 07777;*/
	      if (strcmp (argv[i], "reg") == 0) type |= S_IFREG;	
	      if (strcmp (argv[i], "dir") == 0)	type |= S_IFDIR;
	      if (strcmp (argv[i], "sym") == 0)	type |= S_IFLNK;
	      if (strcmp (argv[i], "sock") == 0)type |= S_IFSOCK;
	      if (strcmp (argv[i], "blk") == 0)	type |= S_IFBLK;
	      if (strcmp (argv[i], "chr") == 0)	type |= S_IFCHR;
	      if (strcmp (argv[i], "fifo") == 0)type |= S_IFIFO;
	      if (strcmp (argv[i], "any") != 0) {
		glob_set_options(object,GLOBBER_TYPE);
 	        glob_set_type(object,type);
	      }
	      continue;
	    }
	  if (strstr (argv[i], "p") != NULL) 
	    {
	      i++;
	      /*type &= S_IFMT;*/
	      CHECK_ARG;
	      if (strcmp (argv[i], "suid") == 0)
		type |= S_ISUID;
	      if (strcmp (argv[i], "exe") == 0)
		type |= S_IXUSR;
              glob_set_options(object,GLOBBER_PERM);
 	      glob_set_type(object,type);
	      continue;
	    }
	  if (strstr (argv[i], "o") != NULL)
	   {
	      int valor;	    
	      i++;
	      type &= S_IFMT;
	      CHECK_ARG;
	      sscanf(argv[i],"%o",&valor);
	      type |= (07777&valor);
              glob_set_options(object,GLOBBER_PERM);
 	      glob_set_type(object,type);
	      continue;
	    }
	  
	  if (strstr (argv[i], "s") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      size = atol (argv[i]);
	      if (size < 0) glob_set_sizeL(object,-size*1024);
	      else glob_set_sizeG(object,size*1024);
	      continue;
	    }

	  if (strstr (argv[i], "k") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      min_t = atol (argv[i]);
              glob_set_time(object,month_t,day_t,hour_t,min_t);
	      continue;
	    }
	  if (strstr (argv[i], "h") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      hour_t = atol (argv[i]);
              glob_set_time(object,month_t,day_t,hour_t,min_t);
	      continue;
	    }
	  if (strstr (argv[i], "d") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      day_t = atol (argv[i]);
              glob_set_time(object,month_t,day_t,hour_t,min_t);
	      continue;
	    }
	  if (strstr (argv[i], "m") != NULL)
	    {
	      CHECK_ARG;
	      month_t = atol (argv[i]);
              glob_set_time(object,month_t,day_t,hour_t,min_t);
	      continue;
	    }
	  

	  if (strstr (argv[i], "f") != NULL)
	    {
	      options |= GLOBRUN_FILTERED;
	      i++;
	      CHECK_ARG;
	      filter = argv[i];
	      if (options & GLOBRUN_VERBOSE)
		fprintf (stderr, "filtering %s\n", filter);
	      continue;
	    }
	      /* options for grep : *******************/
	  if (strstr (argv[i], "I") != NULL)
	    {
	      options |= GLOBRUN_NOBINARIES;
	      continue;
	    }
	  if ((strstr (argv[i], "i") != NULL)||(strstr (argv[i], "y") != NULL))
	    {
	      options |= GLOBRUN_IGNORE_CASE;
	      continue;
	    }
	  if (strstr (argv[i], "L") != NULL)
	    {
	      options |= GLOBRUN_INVERT;
	      continue;
	    }
	  if (strstr (argv[i], "c") != NULL)
	    {
	      options |= GLOBRUN_COUNT;
	      continue;
	    }
	  if (strstr (argv[i], "w") != NULL)
	    {
	      options |= GLOBRUN_WORDS_ONLY;
	      continue;
	    }
	  if (strstr (argv[i], "x") != NULL)
	    {
	      options |= GLOBRUN_LINES_ONLY;
	      continue;
	    }
	  if (strstr (argv[i], "Z") != NULL)
	    {
	      options |= GLOBRUN_ZERO_BYTE;
	      continue;
	    }
	  if (strstr (argv[i], "P") != NULL)
	    {
	      options |= GLOBRUN_PID;
	      printf ("PID=%d\n", (int) getpid ());
	      fflush (NULL);
	      continue;
	    }
	  if (strstr (argv[i], "E") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      token = argv[i];
	      operate = grep;
	      options |= GLOBRUN_REG_EXP;
	      continue;
	    }
	  if (strstr (argv[i], "e") != NULL)
	    {
	      i++;
	      CHECK_ARG;
	      token = argv[i];
	      operate = grep;
	      options |= GLOBRUN_REG_EXP;
	      options ^= GLOBRUN_REG_EXP; /* turn off extended regexp */
	      continue;
	    }

	  if (strstr (argv[i], "V") != NULL)
	    {
	      printf ("%s", VERSION_NAME);
	      return 0;
	    }
	  fprintf(stdout,"unknown argument: %s\nuse -h for help.\n",argv[i]);
	  exit(1);
	}
      if (((min_t)||(hour_t)||(day_t)||(month_t))&& !timetype) 
	      glob_set_options(object,GLOBBER_MTIME);
      terminated = globber (object,argv[i], operate, filter);
      globbered = 1;
    } /* end of argument processing */
  
  
  if (!globbered)
    {
      fprintf (stderr, "must specify path\n");
      goto error;
    }
/*	if (terminated) printf("glob run was terminated.\n");*/
  if (!terminated)
    {				/* die quietly and quickly */
      if (options & GLOBRUN_PID)
	printf ("GLOB DONE=%d\n", (int) getpid ());
    }
  fflush (NULL);
  globber_destroy(object);
  exit (0);
}
