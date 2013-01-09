/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 *
 * CPU Status
 *
 * For eWorkPanel by Mikko Lahteenmaki 2003
 * For ede-panel by Sanel Zukan 2009
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include <edelib/Nls.h>

#if defined(linux)
# include <sys/sysinfo.h>
#endif

#ifdef HAVE_KSTAT
# include <kstat.h>
# include <sys/sysinfo.h>
# include <string.h> /* strncmp */
#endif

#ifdef __FreeBSD__
# include <sys/param.h>
# include <sys/sysctl.h>
# if __FreeBSD_version < 500101
#  include <sys/dkstat.h>
# else
#  include <sys/resource.h>
# endif
# include <sys/stat.h>
#endif

#include "CpuMonitor.h"

#ifdef __FreeBSD__

/* The part ripped from top... */
/*
 *	Top users/processes display for Unix
 *	Version 3
 *
 *	This program may be freely redistributed,
 *	but this entire comment MUST remain intact.
 *
 *	Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *	Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

/*
 *	percentages(cnt, out, new, old, diffs) - calculate percentage change
 *	between array "old" and "new", putting the percentages i "out".
 *	"cnt" is size of each array and "diffs" is used for scratch space.
 *	The array "old" is updated on each call.
 *	The routine assumes modulo arithmetic.	This function is especially
 *	useful on BSD mchines for calculating cpu state percentages.
 */

static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];
static int cpu_states[CPUSTATES];

long percentages(int cnt, int *out, long *my_new, long *old, long *diffs)
{
	register int i;
	register long change;
	register long total_change;
	register long *dp;
	long half_total;

	/* initialization */
	total_change = 0;
	dp = diffs;

	/* calculate changes for each state and the overall change */
	for (i = 0; i < cnt; i++)
	{
		if ((change = *my_new - *old) < 0)
		{
			/* this only happens when the counter wraps */
			change = (int)
			((unsigned long)*my_new-(unsigned long)*old);
		}
		total_change += (*dp++ = change);
		*old++ = *my_new++;
	}

	/* avoid divide by zero potential */
	if (total_change == 0)
	{
		total_change = 1;
	}

	/* calculate percentages based on overall change, rounding up */
	half_total = total_change / 2l;

	/* Do not divide by 0. Causes Floating point exception */
	if(total_change) {
		for (i = 0; i < cnt; i++)
		{
		  *out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
		}
	}

	/* return the total in case the caller wants to use it */
	return total_change;
}

#endif /* freebsd */


#define UPDATE_INTERVAL .5f

void cpu_timeout_cb(void *d) {
	((CPUMonitor*)d)->update_status();
	Fl::repeat_timeout(UPDATE_INTERVAL, cpu_timeout_cb, d);
}

CPUMonitor::CPUMonitor() : Fl_Box(0, 0, 45, 25)
{
	box(FL_THIN_DOWN_BOX);

	m_draw_label = true;
	m_samples = m_old_samples = -1;
	cpu = 0;

	colors[IWM_USER] = FL_RED;
	colors[IWM_NICE] = FL_GREEN;
	colors[IWM_SYS]  = FL_DARK3;
	colors[IWM_IDLE] = FL_BACKGROUND_COLOR;

	layout();
}

void CPUMonitor::clear()
{
	if(!cpu) return;

	for (int i = 0; i < samples(); i++)
		delete [] cpu[i]; 

	delete [] cpu;
	cpu = 0;

	m_old_samples = -1;
}

void CPUMonitor::draw()
{
	draw_box();

	if(!cpu && label()) {
		draw_label();
		return;
	}

	int n, c, user, nice, sys, idle, total;
	int W = w() - Fl::box_dw(box());
	int H = h() - Fl::box_dh(box());
	int X = x() + Fl::box_dx(box());
	int Y = y() + Fl::box_dy(box());

	fl_push_clip(X, Y, W, H);

	c = 0;
	for (int i = X; i < samples() + X; i++) {
		user  = cpu[c][IWM_USER];
		nice  = cpu[c][IWM_NICE];
		sys   = cpu[c][IWM_SYS];
		idle  = cpu[c][IWM_IDLE];

		total = user + sys + nice + idle;
		c++;

		int y = Y + H;

		if (total > 0) {
			if (sys) {
				n = (H * (total - sys)) / total; // check rounding
				if (n >= y) n = y;
				if (n < 1) n = 1;
				fl_color(colors[IWM_SYS]);
				fl_line(i, y, i, n);
				y = n - 1;
			}

			if (nice) {
				n = (H * (total - sys - nice)) / total;
				if (n >= y) n = y;
				if (n < 1) n = 1;
				fl_color(colors[IWM_NICE]);
				fl_line(i, y, i, n);
				y = n - 1;
			}

			if (user) {
				n = (H * (total - sys - nice - user)) / total;
				if (n >= y) n = y;
				if (n < 1) n = 1;
				fl_color(colors[IWM_USER]);
				fl_line(i, y, i, n);
				y = n - 1;
			}
		}

		if (idle) {
			if(colors[IWM_IDLE] != FL_BACKGROUND_COLOR) {
				fl_color(colors[IWM_IDLE]);
				fl_line(i, Fl::box_dy(box()), i, y);
			}
		}
	}

	draw_label();
	fl_pop_clip();
}

void CPUMonitor::layout() {
	m_samples = w() - Fl::box_dw(box());

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
}

int CPUMonitor::handle(int e) {
	switch(e) {
		case FL_SHOW: {
			int ret = Fl_Box::handle(e);
			Fl::add_timeout(UPDATE_INTERVAL, cpu_timeout_cb, this);
			return ret;
		}

		case FL_HIDE:
			Fl::remove_timeout(cpu_timeout_cb);
			/* fallthrough */
	}

	return Fl_Box::handle(e);
}

void CPUMonitor::update_status() {
	if(!cpu) return;

	for (int i=1; i < samples(); i++) {
		cpu[i - 1][IWM_USER] = cpu[i][IWM_USER];
		cpu[i - 1][IWM_NICE] = cpu[i][IWM_NICE];
		cpu[i - 1][IWM_SYS]  = cpu[i][IWM_SYS];
		cpu[i - 1][IWM_IDLE] = cpu[i][IWM_IDLE];
	}

	get_cpu_info();

	// Update tooltip
	static char load[255];
	snprintf(load, sizeof(load)-1,
			 _("CPU Load\n"
			 "User: %d%%\n"
			 "Nice: %d%%\n"
			 "Sys:   %d%%\n"
			 "Idle: %d%%"),
			 cpu[samples()-1][0]*2, cpu[samples()-1][1]*2,
			 cpu[samples()-1][2]*2, cpu[samples()-1][3]*2);

	// Update label
	int cpu_percent = cpu[samples()-1][0]*2;
	if(m_draw_label && cpu_percent<=100) {
		static char buf[16];
		snprintf(buf, sizeof(buf), "%i%%", cpu_percent);
		label(buf);
	}

	tooltip(load);
}

void CPUMonitor::get_cpu_info() {
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
	fprintf(stderr, "cpu: %d %d %d %d\n",
			cpu[samples()-1][IWM_USER], cpu[samples()-1][IWM_NICE],
			cpu[samples()-1][IWM_SYS],	cpu[samples()-1][IWM_IDLE]);
#endif

#endif /* linux */

#ifdef HAVE_KSTAT
# ifdef HAVE_OLD_KSTAT
# define ui32 ul
#endif
	static kstat_ctl_t	*kc = NULL;
	static kid_t		kcid;
	kid_t				new_kcid;
	kstat_t				*ks = NULL;
	kstat_named_t		*kn = NULL;
	int					changed,change,total_change;
	unsigned int		thiscpu;
	register int		i,j;
	static unsigned int ncpus;
	static kstat_t		**cpu_ks=NULL;
	static cpu_stat_t	*cpu_stat=NULL;
	static long			cp_old[CPU_STATES];
	long				cp_time[CPU_STATES], cp_pct[CPU_STATES];

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

#ifdef __FreeBSD__
	size_t len = sizeof(cp_time);

	cpu[samples()-1][IWM_USER] = 0;
	cpu[samples()-1][IWM_NICE] = 0;
	cpu[samples()-1][IWM_SYS] = 0;
	cpu[samples()-1][IWM_IDLE] = 0;

	if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) == -1)
		return; /* FIXME : need err handler? */

	percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

	// Translate FreeBSD stuff into ours (probably the same thing anyway)
	cpu[samples()-1][IWM_USER] = cp_diff[CP_USER];
	cpu[samples()-1][IWM_NICE] = cp_diff[CP_NICE];
	cpu[samples()-1][IWM_SYS] = cp_diff[CP_SYS];
	cpu[samples()-1][IWM_IDLE] = cp_diff[CP_IDLE];

#if 0
	fprintf(stderr, "cpu: %d %d %d %d\n",
			cpu[samples()-1][IWM_USER], cpu[samples()-1][IWM_NICE],
			cpu[samples()-1][IWM_SYS],	cpu[samples()-1][IWM_IDLE]);
#endif

#endif /* freebsd */
}

EDE_PANEL_APPLET_EXPORT (
 CPUMonitor, 
 EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT,
 "CPU monitor",
 "0.1",
 "empty",
 "various"
)
