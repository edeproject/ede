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

#include <stdlib.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <edelib/Debug.h>
#include <edelib/String.h>

#include "Bugzilla.h"

EDELIB_NS_USING(String)

#define CLIENT_NAME    "ede-bug-report"
#define CLIENT_VERSION "0.1"

/* must be static; don't ask me why */
static struct xmlrpc_clientparms gparms;

struct BugzillaData {
	xmlrpc_env      xenv;
	xmlrpc_client  *xcli;
	String          url;

	IdResponseCallback    id_response_cb;
	VersionResponseCallback version_response_cb;
	void *cb_data;
};

static void _logout_response_handler(const char *url, const char *method, 
		xmlrpc_value *val, void *user_data, xmlrpc_env *faultp, xmlrpc_value *result)
{
	/* do nothing */
}

static void _version_response_handler(const char *url, const char *method, 
		xmlrpc_value *val, void *user_data, xmlrpc_env *faultp, xmlrpc_value *result)
{
	BugzillaData *data = (BugzillaData *)user_data;

	if(faultp->fault_occurred) {
		if(data->version_response_cb) {
			/* do callback with error */
			(*data->version_response_cb)(NULL, data->cb_data, faultp->fault_occurred, faultp->fault_string);
		}
		return;
	}

	char  *tmp;
	xmlrpc_decompose_value(&data->xenv, result, "{s:s,*}", "version", &tmp);

	if(data->version_response_cb) {
		/* do callback with values */
		(*data->version_response_cb)(tmp, data->cb_data, faultp->fault_occurred, faultp->fault_string);
	}

	/* xmlrpc_decompose_value() allocates returned string value */
	free(tmp);
}

static void _id_response_handler(const char *url, const char *method, 
		xmlrpc_value *val, void *user_data, xmlrpc_env *faultp, xmlrpc_value *result)
{
	BugzillaData *data = (BugzillaData *)user_data;

	if(faultp->fault_occurred) {
		if(data->id_response_cb) {
			/* do callback with error */
			(*data->id_response_cb)(-1, data->cb_data, faultp->fault_occurred, faultp->fault_string);
		}
		return;
	}

	int id;
	xmlrpc_decompose_value(&data->xenv, result, "{s:i,*}", "id", &id);

	if(data->id_response_cb) {
		/* do callback with values */
		(*data->id_response_cb)(id, data->cb_data, faultp->fault_occurred, faultp->fault_string);
	}
}

BugzillaData *bugzilla_new(const char *u) {
	BugzillaData *data = new BugzillaData;

	xmlrpc_env_init(&data->xenv);
	xmlrpc_client_setup_global_const(&data->xenv);
	data->url = u;

	/* only cURL transport is supported */
	gparms.transport = "curl";

	xmlrpc_client_create(&data->xenv, XMLRPC_CLIENT_NO_FLAGS, CLIENT_NAME, CLIENT_VERSION, 
			&gparms, sizeof(gparms), &data->xcli);

	if(data->xenv.fault_occurred) {
		E_WARNING(E_STRLOC ": Unable to init xmlrpc client data (%s)\n", data->xenv.fault_string);
		delete data;
		data = NULL;
	}

	/* nullify callback stuff */
	data->id_response_cb = NULL;
	data->version_response_cb = NULL;
	data->cb_data = NULL;

	return data;
}

void bugzilla_free(BugzillaData *data) {
	if(data == NULL)
		return;

	xmlrpc_client_destroy(data->xcli);
	xmlrpc_env_clean(&data->xenv);

	delete data;
}

void bugzilla_set_id_response_callback(BugzillaData *data, IdResponseCallback cb) {
	E_ASSERT(data != NULL);
	data->id_response_cb = cb;
}

void bugzilla_set_version_response_callback(BugzillaData *data, VersionResponseCallback cb) {
	E_ASSERT(data != NULL);
	data->version_response_cb = cb;
}

void bugzilla_set_callback_data(BugzillaData *data, void *param) {
	E_ASSERT(data != NULL);
	data->cb_data = param;
}

void bugzilla_event_loop_finish_timeout(BugzillaData *data, unsigned long ms) {
	E_ASSERT(data != NULL);
	xmlrpc_client_event_loop_finish_timeout(data->xcli, ms);
}

void bugzilla_event_loop_finish(BugzillaData *data) {
	E_ASSERT(data != NULL);
	xmlrpc_client_event_loop_finish(data->xcli);
}

void bugzilla_get_version(BugzillaData *data) {
	E_ASSERT(data != NULL);

	xmlrpc_client_start_rpcf(&data->xenv, data->xcli, data->url.c_str(), 
			"Bugzilla.version", 
			_version_response_handler, data, "()");
}

void bugzilla_login(BugzillaData *data, const char *user, const char *passwd) {
	E_ASSERT(data != NULL);

	const int remember = 0;  /* remember login data */

	xmlrpc_client_start_rpcf(&data->xenv, data->xcli, data->url.c_str(),
			"User.login",
			_id_response_handler, data,
			"({s:s,s:s,s:b})",
			"login", user,
			"password", passwd,
			"remember", remember);
}

void bugzilla_logout(BugzillaData *data) {
	E_ASSERT(data != NULL);

	xmlrpc_client_start_rpcf(&data->xenv, data->xcli, data->url.c_str(), 
			"User.logout", _logout_response_handler, NULL, "()");
}

void bugzilla_submit_bug(BugzillaData *data, const char *product,
											const char *component,
											const char *summary,
											const char *version,
											const char *description,
											const char *op_sys,
											const char *platform,
											const char *priority,
											const char *severity)
{
	E_ASSERT(data != NULL);

	xmlrpc_client_start_rpcf(&data->xenv, data->xcli, data->url.c_str(), 
			"Bug.create",
			_id_response_handler, data,
			"({s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s})",
			"product", product,
			"component", component,
			"summary", summary,
			"version", version,
			"description", description,
			"op_sys", op_sys,
			"platform", platform,
			"priority", priority,
			"severity", severity);
}
