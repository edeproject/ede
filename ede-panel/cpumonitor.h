#ifndef _CPUMONITOR_H_
#define _CPUMONITOR_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_KSTAT_H
#include <kstat.h>
#include <sys/sysinfo.h>
#endif /* have_kstat_h */

enum {
    IWM_USER = 0,
    IWM_NICE,
    IWM_SYS,
    IWM_IDLE,
    IWM_STATES
};

#include <efltk/Fl_Widget.h>
#include <efltk/Fl_Locale.h>
#include <efltk/fl_draw.h>
#include <efltk/Fl.h>

class CPUMonitor : public Fl_Widget {
public:
    CPUMonitor();
    virtual ~CPUMonitor();

    void clear();

    void update_status();
    void get_cpu_info();

    virtual void draw();
    virtual void layout();
    virtual void preferred_size(int &w, int &h) { w=this->w(); }

    int samples() const { return m_samples; }

private:
    bool m_draw_label;
    int m_old_samples;
    int m_samples;

    int **cpu;
    long last_cpu[IWM_STATES];
    Fl_Color colors[IWM_STATES];
};

#endif
