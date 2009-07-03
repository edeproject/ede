/*
 * $Id$
 *
 * ede-bug-report, a tool to report bugs on EDE bugzilla instance
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __PULSEPROGRESS_H__
#define __PULSEPROGRESS_H__

#include <FL/Fl_Widget.H>

class PulseProgress : public Fl_Widget {
private:
	int cprogress;  /* current progress */
	int blen;       /* bar length */
	int slen;       /* step length */

protected:
	void draw(void);

public:
	PulseProgress(int x, int y, int w, int h, const char *l = 0);

	void bar_len(int l) { blen = l; }
	int bar_len(void) { return blen; }

	void step_len(int l) { slen = l; }
	int step_len(void) { return slen; }

	void step(void);

	void restart(void) { cprogress = 0; }
};

#endif

