#include "Applet.h"

#include <FL/Fl_Box.H>
#include <FL/Fl.H>
#include <time.h>

#include <edelib/Debug.h>
#include <edelib/Run.h>

EDELIB_NS_USING(run_async)

static void clock_refresh(void *o);

class Clock : public Fl_Box {
private:
	int  hour;
	char buf[64], tbuf[128];

	time_t     curr_time;
	struct tm *curr_tm;
public:
	Clock() : Fl_Box(450, 0, 80, 25, NULL), hour(0) { 
		box(FL_FLAT_BOX);
	}

	~Clock() {
		Fl::remove_timeout(clock_refresh);
	}

	int handle(int e);
	void update_time(void);
};

static void clock_refresh(void *o) {
	Clock *c = (Clock *)o;
	c->update_time();
	Fl::repeat_timeout(1.0, clock_refresh, o);
}

void Clock::update_time(void) {
	curr_time = time(NULL);
	curr_tm = localtime(&curr_time);
	if(!curr_tm)
		return;

	strftime(buf, sizeof(buf), "%H:%M:%S", curr_tm);
	label(buf);

	/* update tooltip if needed */
	if(curr_tm->tm_hour != hour) {
		hour = curr_tm->tm_hour;
		strftime(tbuf, sizeof(tbuf), "%A, %d %B %Y", curr_tm);
		tooltip(tbuf);
	}
}

int Clock::handle(int e) {
	switch(e) {
		case FL_SHOW: {
			int ret = Fl_Box::handle(e);
			Fl::add_timeout(0, clock_refresh, this);
			return ret;
		}

		case FL_RELEASE:
			run_async("ede-timedate");
			break;

		case FL_HIDE:
			Fl::remove_timeout(clock_refresh);
			/* fallthrough */
	}

	return Fl_Box::handle(e);
}

EDE_PANEL_APPLET_EXPORT (
 Clock, 
 EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT,
 "Clock applet",
 "0.1",
 "empty",
 "Sanel Zukan"
)
