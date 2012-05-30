#ifndef __CPUMONITOR_H__
#define __CPUMONITOR_H__

#include <FL/Fl_Box.H>
#include "Applet.h"

/*
#ifdef HAVE_KSTAT_H
# include <kstat.h>
# include <sys/sysinfo.h>
#endif
*/

enum {
    IWM_USER = 0,
    IWM_NICE,
    IWM_SYS,
    IWM_IDLE,
    IWM_STATES
};

class CPUMonitor : public Fl_Box {
private:
    bool m_draw_label;
    int  m_old_samples;
    int  m_samples;

    int **cpu;
    long last_cpu[IWM_STATES];
    Fl_Color colors[IWM_STATES];

public:
    CPUMonitor();
    ~CPUMonitor() { clear(); }

    void clear();

    void update_status();
    void get_cpu_info();

    void draw();
    void layout();
	int  handle(int e);

    int samples() const { return m_samples; }
};

#endif
