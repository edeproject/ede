/*
 * $Id$
 *
 * edelib::PTY - A class for handling pseudoterminals (PTYs)
 * Adapted from KDE (kdelibs/kdesu/kdesu_pty.h) - original copyright message below
 *
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


/* vi: ts=8 sts=4 sw=4
 *
 * Id: kdesu_pty.h 439322 2005-07-27 18:49:23Z coolo 
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * This is free software; you can use this library under the GNU Library
 * General Public License, version 2. See the file "COPYING.LIB" for the
 * exact licensing terms.
 */


/**
 * PTY compatibility routines. This class tries to emulate a UNIX98 PTY API
 * on various platforms.
 */
#ifndef _edelib_PTY_h_
#define _edelib_PTY_h_

//#include <qbytearray.h>

//#include <kdelibs_export.h>

namespace edelib {

class PTY {

public:
    /**
     * Construct a PTY object.
     */
    PTY();

    /**
     * Destructs the object. The PTY is closed if it is still open.
     */
    ~PTY();

    /**
     * Allocate a pty.
     * @return A filedescriptor to the master side.
     */
    int getpt();

    /**
     * Grant access to the slave side.
     * @return Zero if succesfull, < 0 otherwise.
     */
    int grantpt();

    /**
     * Unlock the slave side.
     * @return Zero if successful, < 0 otherwise.
     */
    int unlockpt();

    /**
     * Get the slave name.
     * @return The slave name.
     */
    const char *ptsname();

private:

    int ptyfd;
    char *ptyname, *ttyname;

    class PTYPrivate;
    PTYPrivate *d;
};

}

#endif  // _edelib_PTY_h_
