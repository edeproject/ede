#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Frame.h"
#include "Desktop.h"
#include "Windowmanager.h"

#include <efltk/filename.h>
#include <efltk/fl_draw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "debug.h"

#include <signal.h>

bool wm_shutdown = false;

void exit_signal(int signum)
{
    printf("EDEWM: Exiting (got signal %d)\n", signum);
    wm_shutdown = true;
}

int main(int argc, char ** argv) 
{
    signal(SIGTERM, exit_signal);
    signal(SIGKILL, exit_signal);
    signal(SIGINT, exit_signal);

    Fl::args(argc, argv);

    fl_init_locale_support("edewm", PREFIX"/share/locale");

    WindowManager wm(argc, argv);
    root = &wm;

    Fl_Style::load_theme();

    while(!wm_shutdown) {
        Fl::wait();
        wm.idle();
    }

    Frame_List l(map_order);
    for(uint n=0; n<l.size(); n++) {
        Frame *f = l[n];
        delete f;
    }
	DBG("Going down");
    return 0;
}

void DBG(const char *str, ...)
{
#ifdef _DEBUG
    fprintf(stderr, "EDEWM DEBUG: ");

    va_list args;
    va_start(args, str);
    vfprintf(stderr, str, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
#endif
}
