/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 *
 * Based on SLiM (Simple Login Manager) numlock code by:
 * Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
 * Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
 *
 * which in turn they ripped from KDE and nicely provide authors
 * so I'm doing that too:
 *
 * Copyright (C) 2000-2001 Lubos Lunak        <l.lunak@kde.org>
 * Copyright (C) 2001      Oswald Buddenhagen <ossi@kde.org>
 *
 * And for else, blame me (Sanel).
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "NumLock.h"
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <string.h>

static unsigned int numlock_mask(Display* dpy) {
	XkbDescPtr xkb = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
	if(xkb == NULL)
		return 0;

	char* modstr;
	unsigned int mask = 0;

	for(int i = 0; i < XkbNumVirtualMods; i++) {
		modstr = XGetAtomName(xkb->dpy, xkb->names->vmods[i]);
		if(modstr != NULL && strcmp(modstr, "NumLock") == 0) {
			XkbVirtualModsToReal(xkb, 1 << i, &mask);
			break;
		}
	}

	XkbFreeKeyboard(xkb, 0, True);
	return mask;
}

bool numlock_xkb_init(Display* dpy) {
	int opcode, event, error;
	int maj, min;

	return XkbLibraryVersion(&maj, &min) && XkbQueryExtension(dpy, &opcode, &event, &error, &maj, &min);
}

void numlock_on(Display* dpy) {
	unsigned int mask = numlock_mask(dpy);
	if(!mask)
		return;
	XkbLockModifiers(dpy, XkbUseCoreKbd, mask, mask);
}

void numlock_off(Display* dpy) {
	unsigned int mask = numlock_mask(dpy);
	if(!mask)
		return;
	XkbLockModifiers(dpy, XkbUseCoreKbd, mask, 0);
}
