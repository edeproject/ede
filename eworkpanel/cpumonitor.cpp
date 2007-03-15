/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * CPU Status
 *
 * For eWorkPanel by Mikko Lahteenmaki 2003
 */
#include "cpumonitor.h"

#include <limits.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(linux)
//#include <linux/kernel.h>
#include <sys/sysinfo.h>
#endif

#ifdef HAVE_KSTAT_H
#include <kstat.h>
#endif

#if (defined(linux) || defined(HAVE_KSTAT_H))

#define UPDATE_INTERVAL .5f

void cpu_timeout_cb(void *d) {
    ((CPUMonitor*)d)->update_status();
    fltk::repeat_timeout(UPDATE_INTERVAL, cpu_timeout_cb, d);
}

CPUMonitor::CPUMonitor()
: fltk::Widget(0,0,30,0)
{
    box(fltk::THIN_DOWN_BOX);
    //box(FL_BORDER_BOX);

    m_draw_label = true;
    m_samples = m_old_samples = -1;
    cpu = 0;

    colors[IWM_USER] = FL_RED;
    colors[IWM_NICE] = FL_GREEN;
    colors[IWM_SYS]  = FL_DARK3;
    colors[IWM_IDLE] = FL_NO_COLOR;

    fltk::add_timeout(UPDATE_INTERVAL, cpu_timeout_cb, this);
}

CPUMonitor::~CPUMonitor() {
    clear();
}

void CPUMonitor::clear()
{
    if(!cpu) return;

    for (int i=0; i < samples(); i++) {
        delete cpu[i]; cpu[i] = 0;
    }
    delete cpu;
    cpu = 0;

    m_old_samples = -1;
}

void CPUMonitor::draw()
{
    if(!cpu) {
        fltk::push_clip(0,0,w(),h());
        parent()->draw_group_box();
        draw_frame();
        fltk::pop_clip();
        return;
    }

    if(colors[IWM_IDLE] == FL_NO_COLOR) {
        fltk::push_clip(0,0,w(),h());
        parent()->draw_group_box();
        fltk::pop_clip();
    }
    draw_frame();

    int n, h = height() - box()->dh();

    int c=0;
    for (int i=box()->dx(); i < samples()+box()->dx(); i++)
    {
        int user  = cpu[c][IWM_USER];
        int nice  = cpu[c][IWM_NICE];
        int sys   = cpu[c][IWM_SYS];
        int idle  = cpu[c][IWM_IDLE];
        int total = user + sys + nice + idle;

        c++;

        int y = height() - 1 - box()->dy();

        if (total > 0)
        {
            if (sys) {
                n = (h * (total - sys)) / total; // check rounding
                if (n >= y) n = y;
                if (n < 1) n = 1;
                fltk::color(colors[IWM_SYS]);
                fltk::line(i, y, i, n);
                y = n - 1;
            }

            if (nice) {
                n = (h * (total - sys - nice))/ total;
                if (n >= y) n = y;
                if (n < 1) n = 1;
                fltk::color(colors[IWM_NICE]);
                fltk::line(i, y, i, n);
                y = n - 1;
            }

            if (user) {
                n = (h * (total - sys - nice - user))/ total;
                if (n >= y) n = y;
                if (n < 1) n = 1;
                fltk::color(colors[IWM_USER]);
                fltk::line(i, y, i, n);
                y = n - 1;
            }
        }

        if (idle) {
            if(colors[IWM_IDLE] != FL_NO_COLOR)
            {
                fltk::color(colors[IWM_IDLE]);
                fltk::line(i, box()->dy(), i, y);
            }
        }
    }

    int cpu_percent = cpu[samples()-1][0]*2;
    if(m_draw_label && cpu_percent<=100) {
        char l[5];
	strcpy(l, itoa(cpu_percent));
        strcat(l, '%');
        label(l);
        draw_inside_label();
    }
}

void CPUMonitor::layout()
{
    label_size(h()/2);

    w(h()*2);
    m_samples = w() - box()->dw();

    if(!cpu || m_old_samples != m_samples) {
        clear();

        cpu = new int*[m_samples];
        for(int i=0; i < m_samples; i++) {
            cpu[i] = new int[IWM_STATES];
            cpu[i][IWM_USER] = cpu[i][IWM_NICE] = cpu[i][IWM_SYS] = 0;
            cpu[i][IWM_IDLE] = 1;
        }
        last_cpu[IWM_USER] = last_cpu[IWM_NICE] = last_cpu[IWM_SYS] = last_cpu[IWM_IDLE] = 0;

        update_status();
        m_old_samples = m_samples;
    }

    fltk::Widget::layout();
}

void CPUMonitor::update_status()
{
    if(!cpu) return;

    for (int i=1; i < samples(); i++) {
        cpu[i - 1][IWM_USER] = cpu[i][IWM_USER];
        cpu[i - 1][IWM_NICE] = cpu[i][IWM_NICE];
        cpu[i - 1][IWM_SYS]  = cpu[i][IWM_SYS];
        cpu[i - 1][IWM_IDLE] = cpu[i][IWM_IDLE];
    }

    get_cpu_info();

    // Update tooltip
    char load[255];
    snprintf(load, sizeof(load)-1,
             _("CPU Load:\n"
             "User: %d%%\n"
             "Nice: %d%%\n"
             "Sys:  %d%%\n"
             "Idle: %d%%"),
             cpu[samples()-1][0]*2, cpu[samples()-1][1]*2,
             cpu[samples()-1][2]*2, cpu[samples()-1][3]*2);
    tooltip(load);

    redraw();
}

void CPUMonitor::get_cpu_info()
{
    if(!cpu) return;

#ifdef linux
    char *p, buf[128];
    long cur[IWM_STATES];
    int len, fd = open("/proc/stat", O_RDONLY);

    cpu[samples()-1][IWM_USER] = 0;
    cpu[samples()-1][IWM_NICE] = 0;
    cpu[samples()-1][IWM_SYS] = 0;
    cpu[samples()-1][IWM_IDLE] = 0;

    if (fd == -1)
        return;
    len = read(fd, buf, sizeof(buf) - 1);
    if (len != sizeof(buf) - 1) {
        close(fd);
        return;
    }
    buf[len] = 0;

    p = buf;
    while (*p && (*p < '0' || *p > '9'))
        p++;

    for (int i = 0; i < 4; i++) {
        cur[i] = strtoul(p, &p, 10);
        cpu[samples()-1][i] = cur[i] - last_cpu[i];
        last_cpu[i] = cur[i];
    }
    close(fd);
#if 0
    fprintf(stderr, "cpu: %d %d %d %d",
            cpu[samples()-1][IWM_USER], cpu[samples()-1][IWM_NICE],
            cpu[samples()-1][IWM_SYS],  cpu[samples()-1][IWM_IDLE]);
#endif

#endif /* linux */

#ifdef HAVE_KSTAT_H
# ifdef HAVE_OLD_KSTAT
# define ui32 ul
#endif

    static kstat_ctl_t  *kc = NULL;
    static kid_t        kcid;
    kid_t               new_kcid;
    kstat_t             *ks = NULL;
    kstat_named_t       *kn = NULL;
    int                 changed,change,total_change;
    unsigned int        thiscpu;
    register int        i,j;
    static unsigned int ncpus;
    static kstat_t      **cpu_ks=NULL;
    static cpu_stat_t   *cpu_stat=NULL;
    static long         cp_old[CPU_STATES];
    long                cp_time[CPU_STATES], cp_pct[CPU_STATES];

    /* Initialize the kstat */
    if (!kc) {
        kc = kstat_open();
        if (!kc) {
            perror("kstat_open ");
            return;/* FIXME : need err handler? */
        }
        changed = 1;
        kcid = kc->kc_chain_id;
        fcntl(kc->kc_kd, F_SETFD, FD_CLOEXEC);
    } else {
        changed = 0;
    }
    /* Fetch the kstat data. Whenever we detect that the kstat has been
     changed by the kernel, we 'continue' and restart this loop.
     Otherwise, we break out at the end. */
    while (1) {
        new_kcid = kstat_chain_update(kc);
        if (new_kcid) {
            changed = 1;
            kcid = new_kcid;
        }
        if (new_kcid < 0) {
            perror("kstat_chain_update ");
            return;/* FIXME : need err handler? */
        }
        if (new_kcid != 0)
            continue; /* kstat changed - start over */
        ks = kstat_lookup(kc, "unix", 0, "system_misc");
        if (kstat_read(kc, ks, 0) == -1) {
            perror("kstat_read ");
            return;/* FIXME : need err handler? */
        }
        if (changed) {
            /* the kstat has changed - reread the data */
            thiscpu = 0; ncpus = 0;
            kn = (kstat_named_t *)kstat_data_lookup(ks, "ncpus");
            if ((kn) && (kn->value.ui32 > ncpus)) {
                /* I guess I should be using 'new' here... FIXME */
                ncpus = kn->value.ui32;
                if ((cpu_ks = (kstat_t **)
                     realloc(cpu_ks, ncpus * sizeof(kstat_t *))) == NULL)
                {
                    perror("realloc: cpu_ks ");
                    return;/* FIXME : need err handler? */
                }
                if ((cpu_stat = (cpu_stat_t *)
                     realloc(cpu_stat, ncpus * sizeof(cpu_stat_t))) == NULL)
                {
                    perror("realloc: cpu_stat ");
                    return;/* FIXME : need err handler? */
                }
            }
            for (ks = kc->kc_chain; ks; ks = ks->ks_next) {
                if (strncmp(ks->ks_name, "cpu_stat", 8) == 0) {
                    new_kcid = kstat_read(kc, ks, NULL);
                    if (new_kcid < 0) {
                        perror("kstat_read ");
                        return;/* FIXME : need err handler? */
                    }
                    if (new_kcid != kcid)
                        break;
                    cpu_ks[thiscpu] = ks;
                    thiscpu++;
                    if (thiscpu > ncpus) {
                        fprintf(stderr, "kstat finds too many cpus: should be %d", ncpus);
                        return;/* FIXME : need err handler? */
                    }
                }
            }
            if (new_kcid != kcid)
                continue;
            ncpus = thiscpu;
            changed = 0;
        }
        for (i = 0; i<(int)ncpus; i++) {
            new_kcid = kstat_read(kc, cpu_ks[i], &cpu_stat[i]);
            if (new_kcid < 0) {
                perror("kstat_read ");
                return;/* FIXME : need err handler? */
            }
        }
        if (new_kcid != kcid)
            continue; /* kstat changed - start over */
        else
            break;
    } /* while (1) */

    /* Initialize the cp_time array */
    for (i = 0; i < CPU_STATES; i++)
        cp_time[i] = 0L;
    for (i = 0; i < (int)ncpus; i++) {
        for (j = 0; j < CPU_STATES; j++)
            cp_time[j] += (long) cpu_stat[i].cpu_sysinfo.cpu[j];
    }
    /* calculate the percent utilization for each category */
    /* cpu_state calculations */
    total_change = 0;
    for (i = 0; i < CPU_STATES; i++) {
        change = cp_time[i] - cp_old[i];
        if (change < 0) /* The counter rolled over */
            change = (int) ((unsigned long)cp_time[i] - (unsigned long)cp_old[i]);
        cp_pct[i] = change;
        total_change += change;
        cp_old[i] = cp_time[i]; /* copy the data for the next run */
    }
    /* this percent calculation isn't really needed, since the repaint
     routine takes care of this... */
    for (i = 0; i < CPU_STATES; i++)
        cp_pct[i] =
            ((total_change > 0) ?
             ((int)(((1000.0 * (float)cp_pct[i]) / total_change) + 0.5)) :
             ((i == CPU_IDLE) ? (1000) : (0)));

    /* OK, we've got the data. Now copy it to cpu[][] */
    cpu[samples()-1][IWM_USER] = cp_pct[CPU_USER];
    cpu[samples()-1][IWM_NICE] = cp_pct[CPU_WAIT];
    cpu[samples()-1][IWM_SYS]  = cp_pct[CPU_KERNEL];
    cpu[samples()-1][IWM_IDLE] = cp_pct[CPU_IDLE];

#endif /* have_kstat_h */
}
#endif

