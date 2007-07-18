//  eglob.cpp
//
//  glob for xfce Copyright 2000-2001 Edscott Wilson Garcia
//  Copyright (C) 2001-2002 Martin Pekar
//
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

#include <efltk/Fl.h>
#include <efltk/fl_ask.h>
#include <efltk/Fl_Item.h>
#include <efltk/Fl_Locale.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_ListView_Item.h>

#include <edeconf.h>

#include "efinder.h"
#include "eglob.h"

#include "icons/block_dev.xpm"
#include "icons/char_dev.xpm"
#include "icons/dir_close.xpm"
#include "icons/exe.xpm"
#include "icons/sexe.xpm"
#include "icons/fifo.xpm"
#include "icons/page.xpm"
#include "icons/page_lnk.xpm"
#include "icons/socket.xpm"

#define GLOB "glob"
#define TRUE 1
#define FALSE 0
#define MAX_ARG 50

static int considerTime = FALSE, considerSize = FALSE,
considerUser = FALSE, considerPerm = FALSE,  cancelled = FALSE;
static int pfd[2];               /* the pipe */
static pid_t Gpid;               /* glob pid, to be able to cancel search */
static short int findCount;      /* how many files found */
static short int fileLimit = 64;
static int type=0x0;

static Fl_Image block_dev_pix = *Fl_Image::read_xpm(0, (const char **)block_dev_xpm);
static Fl_Image char_dev_pix  = *Fl_Image::read_xpm(0, (const char **)char_dev_xpm);
static Fl_Image dir_close_pix = *Fl_Image::read_xpm(0, (const char **)dir_close_xpm);
static Fl_Image exe_pix       = *Fl_Image::read_xpm(0, (const char **)exe_xpm);
static Fl_Image sexe_pix      = *Fl_Image::read_xpm(0, (const char **)sexe_xpm);
static Fl_Image fifo_pix      = *Fl_Image::read_xpm(0, (const char **)fifo_xpm);
static Fl_Image page_pix      = *Fl_Image::read_xpm(0, (const char **)page_xpm);
static Fl_Image page_lnk_pix  = *Fl_Image::read_xpm(0, (const char **)page_lnk_xpm);
static Fl_Image socket_pix    = *Fl_Image::read_xpm(0, (const char **)socket_xpm);

static char *ftypes[9] =
{
    "Any kind",
    "Regular",
    "Directory",
    "Symlink",
    "Socket",
    "Block device",
    "Character device",
    "FIFO",
    NULL
};

static char *ft[] =
{
    "any",
    "reg",
    "dir",
    "sym",
    "sock",
    "blk",
    "chr",
    "fifo",
    NULL
};

void
jam(char *file, Fl_Menu_ *optmenu)
{
    FILE *archie;
    char line[256];
    char *s,*r,*t = "Anyone";

    archie=fopen(file,"r");
    if (archie==NULL) return;

    optmenu->add("Anyone");

    while (!feof(archie) && (fgets(line,255,archie)))
    {
        if (feof(archie)) break;
        line[255]=0;
        if ((line[0]=='#')||(strchr(line,':')==NULL)) continue;
        r=strtok(line,":"); if (!r)  continue;
        s=strchr(r+strlen(r)+1,':')+1;if (!s)  continue;
        s=strtok(s,":");if (!s)  continue;
        t=(char *)malloc(strlen(s)+1);
        strcpy(t,s);
        optmenu->add(r);
    }
    fclose(archie);
    return;
}


void
toggle_permission(long data)
{
    int flag;
    flag = (int ) ((long)data);
    type ^= (flag&07777);
}


static void
abort_glob()
{
    if (Gpid)
    {
        kill (Gpid, SIGKILL);    //agressive
    }
}


static void
abort_glob1()
{
    if (Gpid)
    {
        kill (Gpid, SIGTERM);    // nonagressive
    }
}


void
GlobWait(void *data)
{
    int status;
    int childPID;
    childPID = (int) ((long)data);
    //fprintf(stderr,"waiting\n");
    waitpid (childPID, &status, WNOHANG);
    if (WIFEXITED (status))
    {
        //fprintf(stderr,"waiting done\n");
        return;
    }
    Fl::add_timeout(2, GlobWait, (void*)childPID);
    return;
}


void
findCB()
{
    char *argument[MAX_ARG];
    char sizeG_s[64], sizeM_s[64], hours_s[64], permS[64];
    char *path, *filter, *token, *s;
    int i, j, sizeG, sizeM, hours;
    int childPID;

    cancelled = FALSE;

    if (Gpid)
    {
        kill (Gpid, SIGHUP);
        Gpid = 0;
    }

    searchList->clear();

    findCount = 0;
    fileLimit = (int) fileLimitValue->value();
    path = (char*) pathInput->value();

    if (strlen(path)==0)
        path = "/";
    if (path[strlen(path)-1]=='~')
        path = "~/";             //tilde expansion

    if (path[0]=='$')            //environment variables
    {
        path=getenv(path+1);
        if (path==NULL)
            path="/";
    }

    filter = (char*) filterInput->value();
    token = (char*) containsInput->value();
    considerTime = considerTimeValue->value();
    considerSize = considerSizeValue->value();
    considerUser = considerUserValue->value();
    considerPerm = considerPermValue->value();

    if (considerSize)
    {
        sizeG = (int)sizeGValue->value();
        sizeM = (int)sizeMValue->value();
        if ((sizeM <= sizeG)&&(sizeM > 0))
        {
            fl_alert("Incoherent size considerations!");
            return;
        }
    }
    else
        sizeG = sizeM = 0;

    if (considerTime)
    {
        hours = (int)timeValue->value();
    }
    else
        hours = 0;

    //s = (char*) fileTypeBrowser->text(fileTypeBrowser->value());
    s = (char*) fileTypeBrowser->value();

    for (j = -1, i = 0; ftypes[i] != NULL; i++)
    {
        if (strcmp (s, ftypes[i]) == 0)
        {
            j = i;
            break;
        }
    }

    if (j < 0)
        s = ftypes[0];
    i = 0;
    argument[i++] = GLOB;

    //argument[i++] = "-v"; (verbose output from glob for debugging)
    argument[i++] = "-P";

    if (doNotLookIntoBinaryCheck->value())
        argument[i++] = "-I";

    if (recursiveCheck->value())
        argument[i++] = "-r";

    if (considerPerm)
    {
        argument[i++] = "-o";
        snprintf(permS, sizeof(permS)-1, "0%o",type&07777);
        argument[i++] = permS;
    }

    if (caseSensitiveCheck->value())
        argument[i++] = "-i";

    if (outputCountCheck->value())
        argument[i++] = "-c";

    if (invertMatchRadio->value())
        argument[i++] = "-L";

    if (matchWordsRadio->value())
        argument[i++] = "-w";
    else
    {
        if (matchLinesRadio->value())
            argument[i++] = "-x";
    }
    if (j > 0)
    {
        argument[i++] = "-t";
        argument[i++] = ft[j];
    }

    if (considerTime)
    {
        if (modifiedRadio->value()) argument[i++] = "-M";
        if (accessedRadio->value()) argument[i++] = "-A";
        if (changedRadio->value()) argument[i++] = "-C";
        if (hours > 0)
        {
            if (minutesRadio->value()) argument[i++] = "-k";
            if (hoursRadio->value()) argument[i++] = "-h";
            if (daysRadio->value()) argument[i++] = "-d";
            if (mounthsRadio->value()) argument[i++] = "-m";

            snprintf (hours_s, sizeof(hours_s)-1, "%d", hours);
            argument[i++] = hours_s;
        }
    }

    if (considerSize)
    {
        if (sizeG > 0)
        {
            argument[i++] = "-s";
            snprintf (sizeG_s, sizeof(sizeG_s)-1, "+%d", sizeG);
            argument[i++] = sizeG_s;
        }
        if (sizeM > 0)
        {
            argument[i++] = "-s";
            snprintf (sizeM_s, sizeof(sizeM_s)-1,  "-%d", sizeM);
            argument[i++] = sizeM_s;
        }
    }

    if (stayOnSingleCheck->value())
        argument[i++] = "-a";

    if (considerUser)
    {
        if (userIdChoice->value())
        {
            argument[i++] = "-u";
            //argument[i++] = (char*)userIdChoice->text(userIdChoice->value());
            argument[i++] = (char*)userIdChoice->value();
        }
        if (groupIdChoice->value())
        {
            argument[i++] = "-g";
            //argument[i++] = (char*)groupIdChoice->text(groupIdChoice->value());
            argument[i++] = (char*)groupIdChoice->value();
        }
    }

    if (strlen(filter) > 0)      //don't apply filter if not specified and path is absolute!!
    {
        argument[i++] = "-f";
        argument[i++] = filter;
    }
    else
    {
        if (path[strlen (path) - 1] == '/')
        {
            argument[i++] = "-f";
            argument[i++] = "*";
        }
        else
        {
            struct stat st;
            if (stat (path, &st) == 0)
            {
                if (S_ISDIR (st.st_mode))
                {
                    argument[i++] = "-f";
                    argument[i++] = "*";
                }
            }
        }
    }

    if (strlen(token) > 0)       //search token in files
    {
        if (useRegexpCheck->value())
            argument[i++] = "-E";
        else
            argument[i++] = "-e";
        argument[i++] = token;
    }

    argument[i++] = path;        // last argument must be the path
    argument[i] = (char *) 0;
    //for (j=0;j<i;j++) printf ("%s ",argument[j]);printf ("\n");

    Gpid = 0;
    childPID=fork ();

    if (!childPID)
    {
        dup2 (pfd[1], 1);        /* assign child stdout to pipe */
        close (pfd[0]);          /* not used by child */
        execvp (GLOB, argument);
        perror ("exec");
        _exit (127);             /* child never get here */
    }
    Fl::add_timeout(2, GlobWait, (void*)childPID);

    char command[128];
    char *textos[6];
    strcpy (command, argument[0]);
    for (j = 1; j < i; j++)
    {
        strcat (command, " ");
        strcat (command, argument[j]);
    }

    if (strlen(token)) textos[0] = token;  else textos[0] = "";
    if (strlen(filter)) textos[1] = filter;  else textos[1] = "";
    if (strlen(path)) textos[2] = path;  else textos[2] = "";
    textos[3] = textos[4] =  textos[5] = "";

    int *data;
    data=(int *)malloc(3*sizeof(int));
    data[0]=data[1]=data[2]=0;
}


void
stopSearch()
{
    cancelled = TRUE;
    abort_glob();
}


void
process_find_messages(int, void*)
{
    static char *buffer, line[256];
    static int nothing_found;
    char *filename;

    buffer = line;

    while (1)
    {
        if (!read (pfd[0], buffer, 1))
            return;
        if (buffer[0] == '\n')
        {
            buffer[1] = (char) 0;
            if (strncmp(line, "GLOB DONE=", strlen ("GLOB DONE=")) == 0)
            {
                fl_message(_("Search finished."));
                Gpid = 0;
                if (nothing_found)
                    fl_message(_("Nothing found."));
                if (findCount)
                {
                    char mess[128];
                    snprintf(mess, 127, _("Found %d files."), findCount);
                    if (findCount >= fileLimit)
                        fl_message(_("Interrupted because maximum limit exceded."));
                    fl_alert(mess);
                }
                return;
            }

            if ((strncmp (line, "PID=", 4) == 0))
            {
                Gpid = atoi (line + 4);
                //printf("Glob PID=%d\n",Gpid);
                //	      fflush(NULL);
                nothing_found = TRUE;
                return;
            }
            if (cancelled)
                return;

            if (line[0] == '/')  /* strstr for : and strtok and send to cuenta */
            {
                if (findCount >= fileLimit)
                    abort_glob1();
                else
                {
                    char *path, *linecount = NULL, *textos[6], cuenta[32],
                        sizeF[64], permF[16];
                    struct stat st;
                    int *data;

                    path = line;
                    char *ptr = path;
                    while(*ptr) { if(*ptr=='\n') *ptr='\0'; ptr++; }
                    statusLine->copy_label(fl_trim(path));
                    statusLine->redraw();

                    if (strstr(path, ":"))
                    {
                        path = strtok(path, ":");
                        linecount = strtok (NULL, ":");
                        if (strcmp(linecount, "0") == 0)
                        {
                            linecount = NULL;
                            return;
                        }
                    }

                    findCount++;
                    data=(int *)malloc(3*sizeof(int));
                    data[0]=findCount;
                    data[1]=data[2]=0;

                    if (linecount)
                        snprintf(cuenta, sizeof(cuenta)-1, "%d (%s %s)", findCount, linecount, "lines");
                    else
                        snprintf (cuenta, sizeof(cuenta)-1, "%d", findCount);

                    textos[0] = cuenta;
                    textos[1] = filename = (char*)fl_file_filename(path);
                    textos[2] = path;

                    Fl_Image *resultImage=0;

                    if (lstat (path, &st) == 0)
                    {
                        data[1]=st.st_size;
                        data[2]=st.st_ctime;

                        snprintf (sizeF, sizeof(sizeF)-1,"%ld", st.st_size);
                        snprintf (permF, sizeof(permF)-1,"0%o", st.st_mode & 07777);
                        textos[3] = sizeF;
                        textos[4] = ctime (&(st.st_ctime));
                        textos[5] = permF;

                        if (S_ISREG (st.st_mode))
                        {
                            resultImage = &page_pix;
                        }
                        if ((st.st_mode & 0100) || (st.st_mode & 010)
                            || (st.st_mode & 01))
                        {
                            resultImage = &exe_pix;
                        }
                        if (st.st_mode & 04000)
                        {
                            resultImage = &sexe_pix;
                        }
                        if (S_ISDIR (st.st_mode))
                        {
                            resultImage = &dir_close_pix;
                        }
                        if (S_ISCHR (st.st_mode))
                        {
                            resultImage = &char_dev_pix;
                        }
                        if (S_ISBLK (st.st_mode))
                        {
                            resultImage = &block_dev_pix;
                        }
                        if (S_ISFIFO (st.st_mode))
                        {
                            resultImage = &fifo_pix;
                        }
                        if (S_ISLNK (st.st_mode))
                        {
                            resultImage = &page_lnk_pix;
                        }
                        if (S_ISSOCK (st.st_mode))
                        {
                            resultImage = &socket_pix;
                        }
                    }
                    else
                    {
                        textos[2] = textos[3] = textos[4] = "-";
                    }
                    {
                                 // leave just directory
                        *(strrchr(path,'/'))=0;
                        if (!strlen(path))
                            textos[2]="/";
                        char output[FL_PATH_MAX];
                        snprintf(output, sizeof(output)-1, "%s/%s", textos[2], textos[1]);
                        searchList->begin();
                        Fl_ListView_Item *resultItem = new Fl_ListView_Item();

                        // Copy labels, so item destructor knows to de-allocate them
                        resultItem->copy_label(0, output);
                        resultItem->copy_label(1, textos[3]);
                        resultItem->copy_label(2, textos[4]);
                        resultItem->copy_label(3, textos[5]);

                        resultItem->image(resultImage);
                        searchList->end();
                        searchList->relayout();
                        searchList->redraw();
                    }
                }
            }
            //else {}
            nothing_found = FALSE;
            buffer = line;
            return;  ;           /* continue here causes main loop blocking */
        }
        buffer++;
    }
    return;
}

int main (int argc, char **argv)
{
    fl_init_locale_support("efinder", PREFIX"/share/locale");

    if (pipe (pfd) < 0)
    {
        perror ("pipe");
        return 1;
    }

    createFindWindow();

    Fl::add_fd(pfd[0], FL_READ, process_find_messages, (void*)pfd[0]);

    Fl::run();

    close(pfd[0]);
    close(pfd[1]);

    return 0;
}
