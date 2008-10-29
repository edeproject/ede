/*
 * $Id$
 *
 * Screensaver configuration
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../edeconf.h"
// blah

/*#include <efltk/fl_ask.h>
#include <efltk/Fl_Util.h>
#include <efltk/Fl_Input_Browser.h>
#include <efltk/Fl_Config.h>*/

#include <fltk/ask.h>
//#include <efltk/Fl_Util.h>
#include <fltk/InputBrowser.h>
#include <fltk/Window.h>
#include <fltk/x.h>
#include "../edelib2/Config.h"

#include "escreensaver.h"
#include "escrsaverconf.h"
#include "ehacklist.h"


using namespace fltk;
using namespace edelib;





int running_saver = 0;
pid_t running_preview_pid = 0;
int default_saver = 0;

typedef struct screenhack screenhack;
struct screenhack
{
    bool enabled_p;
    char *visual;
    char *name;
    char *command;
};

struct saver_preferences
{
    screenhack **screenhacks;    /* the scrsavers to run */
    int screenhacks_count;
};

char* format_command(const char *cmd, bool wrap_p);

/* Returns a new string describing the shell command.
   This may be just the name of the program, capitalized.
   It also may be something from the resource database (gotten
   by looking for "hacks.XYZ.name", where XYZ is the program.)
 */
char* make_hack_name(const char *shell_command)
{
    char *s = strdup (shell_command);
    char *s2;

    for (s2 = s; *s2; s2++)      /* truncate at first whitespace */
        if (isspace (*s2))
    {
        *s2 = 0;
        break;
    }

    s2 = strrchr (s, '/');       /* if pathname, take last component */
    if (s2)
    {
        s2 = strdup (s2+1);
        free (s);
        s = s2;
    }

    if (strlen (s) > 50)         /* 51 is hereby defined as "unreasonable" */
        s[50] = 0;

    for (s2 = s; *s2; s2++)      /* if it has any capitals, return it */
        if (*s2 >= 'A' && *s2 <= 'Z')
            return s;

                                 /* else cap it */
    if (s[0] >= 'a' && s[0] <= 'z')
        s[0] -= 'a'-'A';
                                 /* (magic leading X) */
    if (s[0] == 'X' && s[1] >= 'a' && s[1] <= 'z')
        s[1] -= 'a'-'A';
    return s;
}


//Parsing scrsavers

screenhack* parse_screenhack(const char *line)
{
    screenhack *h = (screenhack *) calloc (1, sizeof(*h));
    const char *s;

    h->enabled_p = true;

                                 /* skip whitespace */
    while (isspace(*line)) line++;
    if (*line == '-')            /* handle "-" */
    {
        h->enabled_p = false;
        line++;
                                 /* skip whitespace */
        while (isspace(*line)) line++;
    }

    s = line;                    /* handle "visual:" */
    while (*line && *line != ':' && *line != '"' && !isspace(*line))
        line++;
    if (*line != ':')
        line = s;
    else
    {
        h->visual = (char *) malloc (line-s+1);
        strncpy (h->visual, s, line-s);
        h->visual[line-s] = 0;
        if (*line == ':') line++;/* skip ":" */
                                 /* skip whitespace */
        while (isspace(*line)) line++;
    }

    if (*line == '"')            /* handle "name" */
    {
        line++;
        s = line;
        while (*line && *line != '"')
            line++;
        h->name = (char *) malloc (line-s+1);
        strncpy(h->name, s, line-s);
        h->name[line-s] = 0;
        if (*line == '"') line++;/* skip "\"" */
                                 /* skip whitespace */
        while (isspace(*line)) line++;
    }

     /* handle command */
    h->command = format_command(line, false);
    return h;
}


char* format_command(const char *cmd, bool wrap_p)
{
    int tab = 30;
    int col = tab;
    char *cmd2 = (char *) calloc (1, 2 * (strlen (cmd) + 1));
    const char *in = cmd;
    char *out = cmd2;
    while (*in)
    {
        /* shrink all whitespace to one space, for the benefit of the "demo"
        mode display.  We only do this when we can easily tell that the
        whitespace is not significant (no shell metachars).
         */
        switch (*in)
        {
            case '\'': case '"': case '`': case '\\':
                /* Metachars are scary.  Copy the rest of the line unchanged. */
                while (*in)
                    *out++ = *in++, col++;
                break;

            case ' ': case '\t':
                /* Squeeze all other whitespace down to one space. */
                while (*in == ' ' || *in == '\t')
                    in++;
                *out++ = ' ', col++;
                break;

            default:
                /* Copy other chars unchanged. */
                *out++ = *in++, col++;
                break;
        }
    }

    *out = 0;

    /* Strip trailing whitespace */
    while (out > cmd2 && isspace (out[-1]))
        *(--out) = 0;

    return cmd2;
}


static struct saver_preferences p;

void getScreenhacks()
{
    int i, j;
    int start = 0;
    int end = 0;
    int size;
    char *d = hacks;

    p.screenhacks = 0;
    p.screenhacks_count = 0;

    if (!d || !*d)
        return;

    size = strlen (d);

    /* Count up the number of newlines (which will be equal to or larger than
       one less than the number of hacks.)
     */

    for (i = j = 0; d[i]; i++)
    {
        if (d[i] == '\n') j++;
    }

    j++;

    p.screenhacks = (screenhack **) calloc (j + 1, sizeof (screenhack *));

    /* Iterate over the lines in `d' (the string with newlines)
       and make new strings to stuff into the `screenhacks' array.
     */
    p.screenhacks_count = 0;
    while (start < size)
    {
        // skip forward over whitespace. 
        while (d[start] == ' ' || d[start] == '\t' || d[start] == '\n')
            start++;

        // skip forward to newline or end of string. 
        end = start;
        while (d[end] != 0 && d[end] != '\n')
            end++;

        // null terminate
        d[end] = '\0';

        p.screenhacks[p.screenhacks_count++] = parse_screenhack (d + start);
        if (p.screenhacks_count >= i)
            abort();

        start = end+1;
    }

    if (p.screenhacks_count == 0)
    {
        free(p.screenhacks);
        p.screenhacks = 0;
    }
}


void free_screenhack(screenhack *hack)
{
    if (hack->visual) free (hack->visual);
    if (hack->name) free (hack->name);
    free(hack->command);
    memset(hack, 0, sizeof(*hack));
    free (hack);
}

static void free_screenhack_list(screenhack **list, int count)
{
    int i;
    if (!list) return;
    for (i = 0; i < count; i++)
	if (list[i]) free_screenhack (list[i]);
    free (list);
}

Config config("EDE Team", "escrsaver");

void readConfiguration()
{
    int cdefserver = 0;
    config.get("Saver", "Active", cdefserver);
    
    if (!config.error()) 
    {
	default_saver = cdefserver;  
	saversList->text(saversList->child(default_saver)->label()); 
    }

    int ctimeout = 0;
    config.get("Saver", "Timeout", ctimeout, 1);
    if (!config.error()) timeoutSlider->value(ctimeout);
    
    int cdpmsOn = 0;
    config.get("DPMS", "On", cdpmsOn);
    if (!config.error()) {enableDPMSCheck->value(cdpmsOn); enableDPMSCheck->do_callback();}
    
    int cdpmsStandby = 0; config.get("DPMS", "Standby", cdpmsStandby);
    if (!config.error()) standbySlider->value(cdpmsStandby);
    
    int cdpmsSuspend = 0;
    config.get("DPMS", "Suspend", cdpmsSuspend);
    if (!config.error()) suspendSlider->value(cdpmsSuspend);
    
    int cdpmsOff = 0;
    config.get("DPMS", "Off", cdpmsOff);
    if (!config.error()) offSlider->value(cdpmsOff);
}


void writeConfiguration()
{
//    FLE_Config config(fle_find_config_file("apps/escrsaver.conf", 1));
    config.create_section("Saver");
    config.set_section("Saver");
    
    config.write("Active", saversList->item() ? (int)saversList->item()->user_data() : default_saver);

    config.write("Timeout", timeoutSlider->value());
    config.create_section("DPMS");
    config.set_section("DPMS");
    config.write("On", enableDPMSCheck->value());
    config.write("Standby", standbySlider->value());
    config.write("Suspend", suspendSlider->value());
    config.write("Off", offSlider->value());
    
    writeConfigurationSaver();
}


void writeConfigurationSaver()
{
    char *home = getenv("HOME");
    char *path = (char*)malloc(200);
    snprintf(path,200,"%s/.xscreensaver", home);  
    delete [] home;

    FILE *config = fopen(path, "w+"); 
    
    delete [] path;
    
    fprintf(config, "timeout:		0:%d:00\n", (int) timeoutSlider->value());
    fprintf(config, "dpmsEnabled:	%s\n", enableDPMSCheck->value() ? "True" : "False");

    fprintf(config, "dpmsStandby:	0:%d:00\n", (int) standbySlider->value());
    fprintf(config, "dpmsSuspend:	0:%d:00\n", (int) suspendSlider->value());
    fprintf(config, "dpmsOff:		0:%d:00\n", (int) offSlider->value());

    fprintf(config, "mode:		one\n");


    fprintf(config, "selected:		%d\n", 0);    
    
    fprintf(config, "programs:	\"%s\"	%s\n", saversList->item() ?
        (char*)p.screenhacks[(int)saversList->item()->user_data()]->name :
	    (char*)p.screenhacks[default_saver]->name,
	saversList->item() ?    
	(char*)p.screenhacks[(int)saversList->item()->user_data()]->command :
	    (char*)p.screenhacks[default_saver]->command
    );    
    
    fclose(config);

}


void clearOnExit()
{
    free_screenhack_list(p.screenhacks, p.screenhacks_count);
    exit(0);
}


static void killPreviewSubproc()
{
    if (running_preview_pid)
    {
        kill(running_preview_pid, SIGTERM);
        running_preview_pid = 0;
    }
    running_saver = 0;
}


void launchPreviewSubprocess(char *new_cmd)
{
    extern char **environ;
    pid_t forked;

    if (running_preview_pid)
    {
        killPreviewSubproc();
    }

    switch ((int) (forked = fork ()))
    {
        case -1:
        {
            alert(_("Couldn't fork screensaver subprocess."));
            return;
        }
        case 0:
        {
            usleep(250000);
            char *argv[4];
            argv[0] = "sh";
            argv[1] = "-c";
            argv[2] = new_cmd;
            argv[3] = NULL;
            if (execve("/bin/sh", argv, environ) == -1)
                alert(_("Couldn't fork shell subprocess."));
            exit(1);
            break;
        }
        default:
            running_preview_pid = forked;
            break;
    }
}

// fill combo box with scrsavers names
// TODO: read screensavers actually installed on the system
void fillSaversList(InputBrowser *browser)
{
    for (int i = 0; i < p.screenhacks_count; i++)
    {
        browser->begin();
        Item *item = 0;
        if (!(p.screenhacks[i]->name)) // if scrsaver do not have the name
        {
            item = new Item(make_hack_name(p.screenhacks[i]->command));
        }
        else item = new Item(p.screenhacks[i]->name);

        item->user_data((void*) i);
	item->activate(p.screenhacks[i]->enabled_p);
        browser->end();
    }
    browser->text(browser->child(0)->label());
}

// preview 
void startSaverPreview()
{
    static XWindow id;
    static int wid = 0;
    static bool warned = false;

    if (!wid)
    {
        id = xid(saverWindow);
        wid=1;
    }
    saverWindow->show();

    if (id==0)
        return;
    killPreviewSubproc();

    char *location = 0;
    if (access(PREFIX"/X11R6/lib/xscreensaver/", F_OK)==0)
	location = PREFIX"/X11R6/lib/xscreensaver/";
    else if (access(PREFIX"/lib/xscreensaver/", F_OK)==0) 	
	location = PREFIX"/lib/xscreensaver/";
    else if (access("/usr/X11R6/lib/xscreensaver/", F_OK)==0) 	
	location = "/usr/X11R6/lib/xscreensaver/";
    else if (access("/lib/xscreensaver/",F_OK)==0) 	
	location = "/lib/xscreensaver/";
    else {
	location = "";
	if (!warned) {
		alert(_("xscreensaver wasn't found on your system.\n\nEquinox Desktop Environment uses xscreensaver to display screen savers. Please install this package if you wish to use graphical screen savers."));
		warned=true;
	}
    }
    
    // no need to continue now...
    if (strlen(location)<1) return;


    char cmd[4096] = {0};
    char* savercmd;
    // all sorts of stuff can be undefined (zero), so we need to check thoroughly
    if (saversList->item()) {
	int saverno = (int)saversList->item()->user_data();
	fprintf (stderr, " ++ screenhack %d\n", saverno);
    	if (p.screenhacks && p.screenhacks[saverno] && p.screenhacks[saverno]->command) {
		savercmd = (char*)p.screenhacks[(int)saversList->item()->user_data()]->command;
	} else {
		// no preview
		return;
	}
    } else
	savercmd = (char*)p.screenhacks[default_saver]->command;
    snprintf(cmd, 4096, "%s%s -window-id 0x%X", location, savercmd, (int)id);
    launchPreviewSubprocess(cmd);
}
