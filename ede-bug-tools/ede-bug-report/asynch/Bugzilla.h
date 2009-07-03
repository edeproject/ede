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

/*
 * This is asynchronous Bugzilla interface (xmlrpc calls to Bugzilla installation). This means
 * that all calls will be processed without blocking and all results will be reported via callbacks.
 *
 * To keep things simple, we have two type of callbacks, IdResponseCallback and VersionResponseCallback.
 * VersionResponseCallback will be called when request is sent via bugzilla_get_version() and
 * IdResponseCallback when bugzilla_login() and bugzilla_submit_bug(). In first case, we will get
 * user ID or -1 (and set errnum > 0 and err != NULL) if user does not exists or something was wrong
 * with the connection. bugzilla_submit_bug() behaves the same, except it returns bug ID if bug was
 * created successfully.
 *
 * Callbacks are sev bia bugzilla_set_id_response_callback() and bugzilla_set_version_response_callback()
 * and data passed to the callback is done via bugzilla_set_callback_data().
 *
 * The reason why IdResponseCallback was used for both bugzilla_login() and bugzilla_submit_bug()
 * is because both mark returned values as 'id' and to not clutter things with callbacks types such
 * as BugIdResponseCallback or LoginIdResponse-bla-bla... you get it :)
 */

struct BugzillaData;

typedef void (*IdResponseCallback)(int, void *data, int errnum, const char *err);
typedef void (*VersionResponseCallback)(const char *, void *data, int errnum, const char *err);

BugzillaData  *bugzilla_new(const char *url);
void           bugzilla_free(BugzillaData *data);

/* callbacks when response arrives */
void           bugzilla_set_id_response_callback(BugzillaData *data, IdResponseCallback cb);
void           bugzilla_set_version_response_callback(BugzillaData *data, VersionResponseCallback cb);

/* data sent to callbacks */
void           bugzilla_set_callback_data(BugzillaData *data, void *param);

void           bugzilla_event_loop_finish_timeout(BugzillaData *data, unsigned long ms);
void           bugzilla_event_loop_finish(BugzillaData *data);

/* get bugzilla version; VersionResponseCallback will be called */
void           bugzilla_get_version(BugzillaData *data);

/* login to bugzilla instance; IdResponseCallback will be called */
void           bugzilla_login(BugzillaData *data, const char *user, const char *passwd);
void           bugzilla_logout(BugzillaData *data);

/* submit bug (must be logged); retuned bug id will be reported via IdResponseCallback */
void           bugzilla_submit_bug(BugzillaData *data,  const char *product,
														const char *component,
														const char *summary,
														const char *version,
														const char *description,
														const char *op_sys,
														const char *platform,
														const char *priority,
														const char *severity);
#endif
