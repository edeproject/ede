/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <FL/x.H>
#include <X11/Xresource.h>

#include <edelib/Debug.h>
#include <edelib/TiXml.h>
#include <edelib/Color.h>
#include <edelib/String.h>
#include <edelib/Util.h>
#include <edelib/Directory.h>
#include <edelib/XSettingsCommon.h>
#include <edelib/File.h>
#include <edelib/Resource.h>
#include <edelib/Nls.h>

#ifdef EDELIB_HAVE_DBUS
# include <edelib/EdbusList.h>
#endif

#include "Xsm.h"

#define USER_XRESOURCE       ".Xdefaults"
#define USER_XRESOURCE_TMP   ".Xdefaults-tmp"
#define USER_XRESOURCE_SAVED ".Xdefaults-ede-saved"

#define SETTINGS_FILENAME    "ede-settings"

EDELIB_NS_USING(String)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(XSettingsSetting)
EDELIB_NS_USING(XSettingsList)

#ifdef EDELIB_HAVE_DBUS
EDELIB_NS_USING(EdbusMessage)
EDELIB_NS_USING(EdbusData)
EDELIB_NS_USING(EdbusList)
EDELIB_NS_USING(EDBUS_SESSION)
#endif

EDELIB_NS_USING(dir_home)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(file_rename)
EDELIB_NS_USING(build_filename)
EDELIB_NS_USING(user_config_dir)
EDELIB_NS_USING(xsettings_list_find)
EDELIB_NS_USING(xsettings_list_free)
EDELIB_NS_USING(xsettings_decode)
EDELIB_NS_USING(color_rgb_to_fltk)
EDELIB_NS_USING(color_fltk_to_html)
EDELIB_NS_USING(XSETTINGS_TYPE_COLOR)
EDELIB_NS_USING(XSETTINGS_TYPE_INT)
EDELIB_NS_USING(XSETTINGS_TYPE_STRING)

#define STR_CMP(s1, s2) (strcmp((s1), (s2)) == 0)

struct ResourceMap {
	const char* name;
	const char* xresource_key;
	const char* xresource_klass;
};

/*
 * Make sure xresource_klass with '*' is listed last since it have
 * highest priority and will override all previous classes (X Resource class, not C++ one :P)
 * with the same xresource_key.
 */
static ResourceMap resource_map [] = {
	{ "Fltk/Background2", "background", "*Text" },
	{ "Fltk/Background",  "background", "*" },
	{ "Fltk/Foreground",  "foreground", "*" }
};

#define RESOURCE_MAP_SIZE(x) (sizeof(x)/sizeof(x[0]))

static int ignore_xerrors(Display* display, XErrorEvent* xev) {
	return True;
}

#ifdef EDELIB_HAVE_DBUS
static void handle_get_type(XSettingsData* mdata, const EdbusMessage* orig, EdbusMessage& reply) {
	if(orig->size() != 1) {
		reply.create_error_reply(*orig, _("This function accepts only one parameter"));
		return;
	} 

	EdbusMessage::const_iterator it = orig->begin();
	if(!(*it).is_string()) {
		reply.create_error_reply(*orig, _("Parameter must be a string"));
		return;
	}

	XSettingsSetting* s = xsettings_list_find(mdata->settings, (*it).to_string());
	if(!s) {
		reply.create_error_reply(*orig, _("Requested setting wasn't found"));
		return;
	}

	reply.create_reply(*orig);
	switch(s->type) {
		case XSETTINGS_TYPE_STRING:
			reply << EdbusData::from_string("string");
			break;
		case XSETTINGS_TYPE_INT:
			reply << EdbusData::from_string("int");
			break;
		case XSETTINGS_TYPE_COLOR:
			reply << EdbusData::from_string("color");
			break;
		default:
			E_FATAL("Received unknown XSETTINGS type!\n");
	}
}

static void handle_get_all(XSettingsData* mdata, const EdbusMessage* orig, EdbusMessage& reply) {
	reply.create_reply(*orig);

	EdbusList array = EdbusList::create_array();
	XSettingsList* iter = mdata->settings;

	while(iter) {
		array << EdbusData::from_string(iter->setting->name);
		iter = iter->next;
	}

	reply << EdbusData::from_array(array);
}

static void handle_get_value(XSettingsData* mdata, const EdbusMessage* orig, EdbusMessage& reply) {
	if(orig->size() != 1) {
		reply.create_error_reply(*orig, _("This function accepts only one parameter"));
		return;
	} 

	EdbusMessage::const_iterator it = orig->begin();
	if(!(*it).is_string()) {
		reply.create_error_reply(*orig, _("Parameter must be a string"));
		return;
	}

	XSettingsSetting* s = xsettings_list_find(mdata->settings, (*it).to_string());
	if(!s) {
		reply.create_error_reply(*orig, _("Requested setting wasn't found"));
		return;
	}

	reply.create_reply(*orig);
	switch(s->type) {
		case XSETTINGS_TYPE_STRING:
			reply << EdbusData::from_string(s->data.v_string);
			break;
		case XSETTINGS_TYPE_INT:
			reply << EdbusData::from_int32(s->data.v_int);
			break;
		case XSETTINGS_TYPE_COLOR: {
			EdbusList rgb_array = EdbusList::create_array();
			rgb_array << EdbusData::from_int32(s->data.v_color.red);
			rgb_array << EdbusData::from_int32(s->data.v_color.green);
			rgb_array << EdbusData::from_int32(s->data.v_color.blue);
			rgb_array << EdbusData::from_int32(s->data.v_color.alpha);

			reply << EdbusData::from_array(rgb_array);
			break;
		}
		default:
			E_FATAL("Received unknown XSETTINGS type!\n");
	}
}

static void handle_remove(Xsm* xsm, XSettingsData* mdata, const EdbusMessage* msg) {
	if(msg->size() != 1)
		return;

	EdbusMessage::const_iterator it = msg->begin();
	if(!(*it).is_string())
		return;

	/* 
	 * just remove it, without reporting the status 
	 * TODO: lock this
	 */
	xsettings_list_remove(&(mdata->settings), (*it).to_string());
	xsm->notify();
}

static void handle_set(Xsm* xsm, XSettingsData* mdata, const EdbusMessage* orig, EdbusMessage& reply) {
	if(orig->size() != 2) {
		reply.create_error_reply(*orig, _("This function accepts two parameters"));
		return;
	} 

	EdbusMessage::const_iterator it = orig->begin();
	if(!(*it).is_string()) {
		reply.create_error_reply(*orig, _("First parameter must be a string"));
		return;
	}

	const char* name = (*it).to_string();

	/* figure out the type of second parameter and add it to the list */
	++it;
	if((*it).is_string()) {
		xsm->set(name, (*it).to_string());
		xsm->notify();

		reply.create_reply(*orig);
		reply << EdbusData::from_bool(true);
	} else if((*it).is_int32()) {
		xsm->set(name, (*it).to_int32());
		xsm->notify();

		reply.create_reply(*orig);
		reply << EdbusData::from_bool(true);
	} else if((*it).is_array()) {
		EdbusList rgb_array = (*it).to_array();

		/* RGBA array has 4 elements */
		if(rgb_array.size() != 4) {
			reply.create_error_reply(*orig, _("Color array must have 4 parameters"));
			return;
		}

		EdbusList::const_iterator arr_it = rgb_array.begin();
		unsigned short r, g, b, a;
		r = g = b = a = 0;

		r = (*arr_it).to_int32();
		++arr_it;
		g = (*arr_it).to_int32();
		++arr_it;
		b = (*arr_it).to_int32();
		++arr_it;
		a = (*arr_it).to_int32();

		xsm->set(name, r, g, b, a);
		xsm->notify();

		reply.create_reply(*orig);
		reply << EdbusData::from_bool(true);
	} else {
		reply.create_error_reply(*orig, _("Unknown value type. Only 3 types are allowed: string, int32 and array[int32]"));
	}
}

#define XSM_OBJECT_PATH "/org/equinoxproject/Xsettings"

#define XSM_INTROSPECTION_XML \
"<node>\n"\
" <interface name=\"org.equinoxproject.Xsettings\">\n" \
"  <method name=\"GetType\">\n"	\
"    <arg name=\"name\" type=\"s\" direction=\"in\"/>\n" \
"    <arg name=\"type\" type=\"s\" direction=\"out\"/>\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Returns type for given name.\" />\n" \
"  </method>\n"	\
"  <method name=\"GetAll\">\n" \
"    <arg name=\"all\" type=\"as\" direction=\"out\"/>\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Get all settings currently registered.\" />\n" \
"  </method>\n" \
"  <method name=\"GetValue\">\n" \
"    <arg name=\"name\" type=\"s\" direction=\"in\"/>\n" \
"    <arg name=\"value\" type=\"v\" direction=\"out\"/>\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Returns value for named setting.\" />\n" \
"  </method>\n" \
"  <method name=\"Remove\">\n" \
"    <arg name=\"name\" type=\"s\" direction=\"in\"/>\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Removes named setting. If not exists, does nothing.\" />\n" \
"  </method>\n" \
"  <method name=\"Flush\">\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Flushes all settings to disk.\" />\n" \
"  </method>\n" \
"  <method name=\"Set\">\n" \
"    <arg name=\"status\" type=\"b\" direction=\"out\"/>\n" \
"    <arg name=\"name\" type=\"s\" direction=\"in\"/>\n" \
"    <arg name=\"value\" type=\"v\" direction=\"in\"/>\n" \
"    <annotation name=\"org.equinoxproject.DBus.DocString\" value=\"Set named setting to given value. Returns true if set or false if failed.\" />\n" \
"  </method>\n" \
" </interface>\n" \
"</node>\n";

static int xsettings_dbus_cb(const EdbusMessage* m, void* data) {
	Xsm* x = (Xsm*)data;
	XSettingsData* md = x->get_manager_data();

	/* introspection */
	if(STR_CMP(m->member(), "Introspect") &&
	   STR_CMP(m->interface(), "org.freedesktop.DBus.Introspectable") &&
	   STR_CMP(m->destination(), "org.equinoxproject.Xsettings"))
	{
		String ret = EDBUS_INTROSPECTION_DTD;
		
		if(STR_CMP(m->path(), XSM_OBJECT_PATH)) {
			ret += XSM_INTROSPECTION_XML;
		} else {
			ret += "<node>\n <node ";
			if(STR_CMP(m->path(), "/"))
				ret += "name=\"org\"";
			else if(STR_CMP(m->path(), "/org"))
				ret += "name=\"equinoxproject\"";
			else if(STR_CMP(m->path(), "/org/equinoxproject"))
				ret += "name=\"Xsettings\"";

			ret += " />\n</node>\n";
		}

		EdbusMessage reply;
		reply.create_reply(*m);
		reply << EdbusData::from_string(ret.c_str());
		x->get_dbus_connection()->send(reply);
		return 1;
	}

	/* string GetType(string name) */
	if(STR_CMP(m->member(), "GetType")) {
		EdbusMessage reply;
		handle_get_type(md, m, reply);

		x->get_dbus_connection()->send(reply);
		return 1;
	}

	/* string-array GetAll(void) */
	if(STR_CMP(m->member(), "GetAll")) {
		EdbusMessage reply;
		handle_get_all(md, m, reply);

		x->get_dbus_connection()->send(reply);
		return 1;
	}

	/* [string|array|int32] GetValue(string name) */
	if(STR_CMP(m->member(), "GetValue")) {
		EdbusMessage reply;
		handle_get_value(md, m, reply);

		x->get_dbus_connection()->send(reply);
		return 1;
	}

	/* void Remove(string name) */
	if(STR_CMP(m->member(), "Remove")) {
		handle_remove(x, md, m);
		return 1;
	}

	/* void Flush(void) */
	if(STR_CMP(m->member(), "Flush")) {
		x->save_serialized();
		return 1;
	}

	/* bool Set(string name, [string|array|int32] value) */
	if(STR_CMP(m->member(), "Set")) {
		EdbusMessage reply;
		handle_set(x, md, m, reply);

		x->get_dbus_connection()->send(reply);
		return 1;
	}

	return 0;
}

#endif /* EDELIB_HAVE_DBUS */

/*
 * This is a short explaination how evoke's XSETTINGS part is combined
 * with X Resource database (xrdb). First of all, why mess with this? Almost
 * all pure X apps (xterm, xedit, rxvt) reads color (and more) from xrdb, not to say
 * that FLTK apps do that too, at least those not linked with edelib::Window. On other
 * hand since edelib::Window already have builtin XSETTINGS and FLTK backend, you will
 * that colors for edelib::Window will be specified twice, but this is not a big deal
 * since painting is done only once.
 *
 * Here, in the code, we look for XSETTINGS names listed in resource_map[] and they should 
 * be colors only; when they are found, their equivalents will be created in xrdb as class/key 
 * (see X Resource manual about these).
 *
 * Values picked up from XSETTINGS color items will be converted to html since because clients 
 * who reads xrdb expects html colors (or X11 ones, like red/green/blue names, but let we not 
 * complicate). After conversion, everything is stored in ~/.Xdefaults file. If this file
 * already exists (someone else made it), it will be first merged, picking old values and backed up.
 *
 * After evoke quits, file is restored, if existed before or deleted if not. This is also a
 * workaround for missing functions to delete key/value pairs from xrdb (what was they thinking for??).
 */
void Xsm::xresource_replace(void) {
	/* with inheritance we got manager_data */
	if(!manager_data->settings)
		return;

	String home = dir_home();

	/* try to open ~/.Xdefaults; if failed, X Resource will not complain */
	String db_file = build_filename(home.c_str(), USER_XRESOURCE);

	/* initialize XResource manager */
	XrmInitialize();

	/* load XResource database */
	XrmDatabase db = XrmGetFileDatabase(db_file.c_str());

	XSettingsSetting* s;
	int status;
	XrmValue xrmv;
	char color_val[8], *type;

	String tmp;

	/* 
	 * XSETTINGS does not contains duplicate entries so there is no need to 
	 * check for them. We only scan ResourceMap table for XSETTINGS name and 
	 * its X Resource equivalent.
	 */
	for(unsigned int i = 0; i < RESOURCE_MAP_SIZE(resource_map); i++) {
		s = xsettings_list_find(manager_data->settings, resource_map[i].name);
		if(!s)
			continue;

		/* assure that XSETTINGS key is color type */
		if(s->type != XSETTINGS_TYPE_COLOR) {
			E_WARNING(E_STRLOC ": Expected color type in %s, but it is not, skipping...\n", s->name);
			continue;
		}

		/* check if resource is present */
		status = XrmGetResource(db, resource_map[i].xresource_key, resource_map[i].xresource_klass, &type, &xrmv);

		if(status && STR_CMP(type, "String")) {
			E_DEBUG(E_STRLOC ": %s.%s found in database\n", 
					resource_map[i].xresource_klass, resource_map[i].xresource_key);
		}

		/*
		 * Now convert color from XSETTINGS to html value. First convert to fltk color.
		 * TODO: Strange, didn't implemented something like color_rgb_to_html in edelib ?
		 */
		int fltk_color = color_rgb_to_fltk(s->data.v_color.red, s->data.v_color.green, s->data.v_color.blue);
		color_fltk_to_html(fltk_color, color_val);

		/* and save it */
		tmp.clear();
		tmp.printf("%s.%s: %s", resource_map[i].xresource_klass, resource_map[i].xresource_key, color_val);
		XrmPutLineResource(&db, tmp.c_str());
	}

	String tmp_db_file = build_filename(home.c_str(), USER_XRESOURCE_TMP);

	/*
	 * Try to merge existing ~/.Xdefaults (if present) with our changes. If there is existing
	 * key/values, they will be replaced with our. If XrmCombineFileDatabase() fails, this means
	 * there is no ~/.Xdefaults, so we don't need to backup it; opposite, backup it before we do
	 * final rename.
	 */
	status = XrmCombineFileDatabase(db_file.c_str(), &db, 0);
	XrmPutFileDatabase(db, tmp_db_file.c_str());
	XrmDestroyDatabase(db);

	if(status) {
		String db_backup = build_filename(home.c_str(), USER_XRESOURCE_SAVED);
		file_rename(db_file.c_str(), db_backup.c_str());
	} 

	file_rename(tmp_db_file.c_str(), db_file.c_str());
}

void Xsm::xresource_undo(void) {
	String home, db_file_backup, db_file;

	home = dir_home();
	db_file_backup = build_filename(home.c_str(), USER_XRESOURCE_SAVED);
	db_file = build_filename(home.c_str(), USER_XRESOURCE);

	/*
	 * If we have backup, restore it; otherwise delete ~/.Xdefaults.
	 * TODO: what if user something write in it? Changes will be lost...
	 */
	if(!file_rename(db_file_backup.c_str(), db_file.c_str()))
		file_remove(db_file.c_str());
}

void Xsm::xsettings_dbus_serve(void) {
#ifdef EDELIB_HAVE_DBUS
	E_RETURN_IF_FAIL(!dbus_conn);

	EdbusConnection* d = new EdbusConnection;

	if(!d->connect(EDBUS_SESSION)) {
		E_WARNING(E_STRLOC ": Unable to connecto to session bus. XSETTINGS will not be served via D-Bus\n");
		delete d;
		return;
	}

	if(!d->request_name("org.equinoxproject.Xsettings")) {
		E_WARNING(E_STRLOC ": Unable to request 'org.equinoxproject.Xsettings' name\n");
		delete d;
		return;
	}

	d->register_object(XSM_OBJECT_PATH);
	d->method_callback(xsettings_dbus_cb, this);

	d->setup_listener_with_fltk();
	dbus_conn = d;
#endif /* EDELIB_HAVE_DBUS */
}

bool Xsm::load_serialized(void) {
#ifdef USE_LOCAL_CONFIG
	/* 
	 * this will load SETTINGS_FILENAME only from local directory;
	 * intended for development and testing only
	 */
	String file = SETTINGS_FILENAME".conf";
#else
	/* try to find it in home directory, then will scan for the system one */
	String file = Resource::find_config(SETTINGS_FILENAME);
	if(file.empty()) {
		E_WARNING(E_STRLOC ": Unable to load XSETTINGS data from '%s'\n", file.c_str());
		return false;
	}
#endif

	TiXmlDocument doc(file.c_str());
	if(!doc.LoadFile())
		return false;

	const char* name = NULL, *type = NULL;
	const char* v_string = NULL;
	int v_int = 0;
	int v_red = 0, v_green = 0, v_blue = 0, v_alpha = 0;
	int cmp = 0;

	TiXmlNode* elem = doc.FirstChild("ede-settings");
	if(!elem)
		return false;

	for(elem = elem->FirstChildElement(); elem; elem = elem->NextSibling()) {
		if(!STR_CMP(elem->Value(), "setting")) {
			E_WARNING(E_STRLOC ": Got unknown child in 'ede-setting' %s\n", elem->Value());
			continue;
		}

		name = elem->ToElement()->Attribute("name");
		if(!name) {
			E_WARNING(E_STRLOC ": Missing name key\n");
			continue;
		}

		type = elem->ToElement()->Attribute("type");
		if(!type) {
			E_WARNING(E_STRLOC ": Missing type key\n");
			continue;
		}

		if(STR_CMP(type, "int"))
			cmp = 1;
		else if(STR_CMP(type, "string"))
			cmp = 2;
		else if(STR_CMP(type, "color"))
			cmp = 3;
		else {
			E_WARNING(E_STRLOC ": Unknown type %s\n", type);
			continue;
		}

		switch(cmp) {
			case 1:
				if(elem->ToElement()->QueryIntAttribute("value", &v_int) == TIXML_SUCCESS)
					set(name, v_int);
				else
					E_WARNING(E_STRLOC ": Unable to query integer value\n");
				break;
			case 2:
				v_string = elem->ToElement()->Attribute("value");
				if(v_string)
					set(name, v_string);
				break;
			case 3:
				if((elem->ToElement()->QueryIntAttribute("red", &v_red) == TIXML_SUCCESS) && 
					(elem->ToElement()->QueryIntAttribute("green", &v_green) == TIXML_SUCCESS) &&
					(elem->ToElement()->QueryIntAttribute("blue", &v_blue) == TIXML_SUCCESS) &&
					(elem->ToElement()->QueryIntAttribute("alpha", &v_alpha) == TIXML_SUCCESS)) {
					set(name, v_red, v_green, v_blue, v_alpha);
				}
				break;
			default:
				break;
		}
	}

	xresource_replace();
	xsettings_dbus_serve();
	return true;
}

bool Xsm::save_serialized(void) {
	Atom type;
	int format;
	unsigned long n_items, bytes_after;
	unsigned char* data;
	int result;
	XSettingsList* settings = NULL, *iter = NULL;

	int (*old_handler)(Display*, XErrorEvent*);

	/* possible ? */
	E_RETURN_VAL_IF_FAIL(manager_data->manager_win, false);

	/* manually fetch XSETTINGS encoded data, so we can pick whatever was externally set */
	old_handler = XSetErrorHandler(ignore_xerrors);
	result = XGetWindowProperty(manager_data->display, 
								manager_data->manager_win, 
								manager_data->xsettings_atom, 
								0, LONG_MAX, False, 
								manager_data->xsettings_atom, 
								&type, &format, &n_items, &bytes_after, 
								(unsigned char**)&data);

	XSetErrorHandler(old_handler);
	if(result == Success && type != None) {
		if(type != manager_data->xsettings_atom)
			E_WARNING(E_STRLOC ": Invalid type for XSETTINGS property\n");
		else if(format != 8)
			E_WARNING(E_STRLOC ": Invalid format for XSETTINGS property\n");
		else
			settings = xsettings_decode(data, n_items, NULL);
		XFree(data);
	}

	E_RETURN_VAL_IF_FAIL(settings, false);

#ifdef USE_LOCAL_CONFIG
	/* 
	 * this will load SETTINGS_FILENAME only from local directory;
	 * intended for development and testing only
	 */
	String file = SETTINGS_FILENAME".conf";
#else
	String file = user_config_dir();
	file += "/ede/"SETTINGS_FILENAME".conf";
#endif

	FILE* setting_file = fopen(file.c_str(), "w");
	if(!setting_file) {
		E_WARNING(E_STRLOC ": Unable to write to %s\n", file.c_str());
		xsettings_list_free(settings);
		return false;
	}

	fprintf(setting_file, "<? xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	fprintf(setting_file, "<ede-settings>\n");

	iter = settings;
	while(iter) {
		fprintf(setting_file, " <setting name=\"%s\" ", iter->setting->name);
		switch(iter->setting->type) {
			case XSETTINGS_TYPE_INT:
				fprintf(setting_file, "type=\"int\" value=\"%i\" />\n", iter->setting->data.v_int);
				break;
			case XSETTINGS_TYPE_STRING:
				fprintf(setting_file, "type=\"string\" value=\"%s\" />\n", iter->setting->data.v_string);
				break;
			case XSETTINGS_TYPE_COLOR:
				fprintf(setting_file, "type=\"color\" red=\"%i\" green=\"%i\" blue=\"%i\" alpha=\"%i\" />\n",
						iter->setting->data.v_color.red,
						iter->setting->data.v_color.green,
						iter->setting->data.v_color.blue,
						iter->setting->data.v_color.alpha);
				break;
		}

		iter = iter->next;
	}

	fprintf(setting_file, "</ede-settings>\n");

	fclose(setting_file);
	xsettings_list_free(settings);
	xresource_undo();

	return true;
}
