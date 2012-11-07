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

#ifndef __BUGZILLA_H__
#define __BUGZILLA_H__

struct BugzillaData;
typedef void (*BugzillaErrorCallback)(const char *str, void *data);

BugzillaData  *bugzilla_new(const char *url, BugzillaErrorCallback cb = NULL, void *data = NULL);
void           bugzilla_free(BugzillaData *data);
void           bugzilla_set_error_callback(BugzillaData *data, BugzillaErrorCallback cb, void *cb_data = NULL);

/* return bugzilla version or empty string if fails; returned value must be free()-ed */
char          *bugzilla_get_version(BugzillaData *data);

/* return user id or -1 if fails */
int            bugzilla_login(BugzillaData *data, const char *user, const char *passwd);
void           bugzilla_logout(BugzillaData *data);

/* return bug id or -1 if fails */
int            bugzilla_submit_bug(BugzillaData *data,
								   const char *product,
								   const char *component,
								   const char *summary,
								   const char *version,
								   const char *description,
								   const char *op_sys,
								   const char *platform,
								   const char *priority,
								   const char *severity,
								   const char *cc);
#endif
