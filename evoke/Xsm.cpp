/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Xsm.h"
#include <edelib/Debug.h>
#include <edelib/TiXml.h>
#include <edelib/File.h>
#include <edelib/Color.h>
#include <edelib/String.h>
#include <edelib/Util.h>
#include <edelib/Directory.h>
#include <edelib/XSettingsCommon.h>
#include <string.h>
#include <limits.h>

#include <FL/x.h>

#include <X11/Xresource.h>

#define USER_XRESOURCE       ".Xdefaults"
#define USER_XRESOURCE_TMP   ".Xdefaults-tmp"
#define USER_XRESOURCE_SAVED ".Xdefaults-ede-saved"

struct ResourceMap {
	char* name;
	char* xresource_key;
	char* xresource_klass;
};

/*
 * Make sure xresource_klass with '*' is listed last since it have
 * highest priority and will override all previous classes (X Resource class, not C++ one :P)
 * with the same xresource_key.
 */
ResourceMap resource_map [] = {
	{ "Fltk/Background2", "background", "*Text" },
	{ "Fltk/Background",  "background", "*" },
	{ "Fltk/Foreground",  "foreground", "*" }
};

#define RESOURCE_MAP_SIZE(x) (sizeof(x)/sizeof(x[0]))

int ignore_xerrors(Display* display, XErrorEvent* xev) {
	return True;
}

Xsm::Xsm() {
}

Xsm::~Xsm() { 
	EDEBUG("Xsm::~Xsm()\n"); 
}

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
	// with inheritance we got manager_data
	if(!manager_data->settings)
		return;

	edelib::String home = edelib::dir_home();

	// try to open ~/.Xdefaults; if failed, X Resource will not complain
	edelib::String db_file = edelib::build_filename("/", home.c_str(), USER_XRESOURCE);

	// initialize XResource manager
	XrmInitialize();

	// load XResource database
	XrmDatabase db = XrmGetFileDatabase(db_file.c_str());

	edelib::XSettingsSetting* s;
	int status;
	char* type, *value;
	XrmValue xrmv;
	char color_val[8];

	edelib::String tmp;

	/* 
	 * XSETTINGS does not contains duplicate entries so there is no need to 
	 * check for them. We only scan ResourceMap table for XSETTINGS name and 
	 * its X Resource equivalent.
	 */
	for(unsigned int i = 0; i < RESOURCE_MAP_SIZE(resource_map); i++) {
		s = edelib::xsettings_list_find(manager_data->settings, resource_map[i].name);
		if(!s)
			continue;

		// assure that XSETTINGS key is color type
		if(s->type != edelib::XSETTINGS_TYPE_COLOR) {
			EWARNING(ESTRLOC ": Expected color type in %s, but it is not, skipping...\n", s->name);
			continue;
		}

		// check if resource is present
		status = XrmGetResource(db, resource_map[i].xresource_key, resource_map[i].xresource_klass, &type, &xrmv);

		if(status && strcmp(type, "String") == 0) {
			EDEBUG(ESTRLOC ": %s.%s found in database\n", 
					resource_map[i].xresource_klass, resource_map[i].xresource_key);

			value = xrmv.addr;
		}

		/*
		 * Now convert color from XSETTINGS to html value. First convert to fltk color.
		 * TODO: Strange, didn't implemented something like color_rgb_to_html in edelib ?
		 */
		int fltk_color = edelib::color_rgb_to_fltk(s->data.v_color.red, s->data.v_color.green, s->data.v_color.blue);
		edelib::color_fltk_to_html(fltk_color, color_val);

		// and save it
		tmp.clear();
		tmp.printf("%s.%s: %s", resource_map[i].xresource_klass, resource_map[i].xresource_key, color_val);
		XrmPutLineResource(&db, tmp.c_str());
	}

	edelib::String tmp_db_file = edelib::build_filename("/", home.c_str(), USER_XRESOURCE_TMP);

	/*
	 * Try to merge existing ~/.Xdefaults (if present) with our changes. If there is existing
	 * key/values, they will be replaced with our. If XrmCombineFileDatabase() fails, this means
	 * there is no ~/.Xdefaults, so we don't need to backup it; opposite, backup it before we do
	 * final rename.
	 */
	status = XrmCombineFileDatabase(db_file.c_str(), &db, 0);
	XrmPutFileDatabase(db, tmp_db_file.c_str());
	//XrmSetDatabase(fl_display, db);
	XrmDestroyDatabase(db);
	if(status) {
		edelib::String db_backup = edelib::build_filename("/", home.c_str(), USER_XRESOURCE_SAVED);
		edelib::file_rename(db_file.c_str(), db_backup.c_str());
	} 

	edelib::file_rename(tmp_db_file.c_str(), db_file.c_str());
}

void Xsm::xresource_undo(void) {
	edelib::String home, db_file_backup, db_file;

	home = edelib::dir_home();
	db_file_backup = edelib::build_filename("/", home.c_str(), USER_XRESOURCE_SAVED);
	db_file = edelib::build_filename("/", home.c_str(), USER_XRESOURCE);

	/*
	 * If we have backup, restore it; otherwise delete ~/.Xdefaults.
	 * TODO: what if user something write in it? Changes will be lost...
	 */
	if(edelib::file_exists(db_file_backup.c_str()))
		edelib::file_rename(db_file_backup.c_str(), db_file.c_str());
	else
		edelib::file_remove(db_file.c_str());
}

bool Xsm::load_serialized(const char* file) {
	TiXmlDocument doc(file);
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
		if(strcmp(elem->Value(), "setting") != 0) {
			EWARNING(ESTRLOC ": Got unknown child in 'ede-setting' %s\n", elem->Value());
			continue;
		}

		name = elem->ToElement()->Attribute("name");
		if(!name) {
			EWARNING(ESTRLOC ": Missing name key\n");
			continue;
		}

		type = elem->ToElement()->Attribute("type");
		if(!type) {
			EWARNING(ESTRLOC ": Missing type key\n");
			continue;
		}

		if(strcmp(type, "int") == 0)
			cmp = 1;
		else if(strcmp(type, "string") == 0)
			cmp = 2;
		else if(strcmp(type, "color") == 0)
			cmp = 3;
		else {
			EWARNING(ESTRLOC ": Unknown type %s\n", type);
			continue;
		}

		switch(cmp) {
			case 1:
				if(elem->ToElement()->QueryIntAttribute("value", &v_int) == TIXML_SUCCESS)
					set(name, v_int);
				else
					EWARNING(ESTRLOC ": Unable to query integer value\n");
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
	return true;
}

bool Xsm::save_serialized(const char* file) {
	// FIXME: a lot of this code could be in edelib
	Atom type;
	int format;
	unsigned long n_items, bytes_after;
	unsigned char* data;
	int result;
	edelib::XSettingsList* settings = NULL, *iter = NULL;

	int (*old_handler)(Display*, XErrorEvent*);

	// possible ?
	if(!manager_data->manager_win)
		return false;

	old_handler = XSetErrorHandler(ignore_xerrors);
	result = XGetWindowProperty(manager_data->display, manager_data->manager_win, manager_data->xsettings_atom,
			0, LONG_MAX, False, manager_data->xsettings_atom,
			&type, &format, &n_items, &bytes_after, (unsigned char**)&data);

	XSetErrorHandler(old_handler);
	if(result == Success && type != None) {
		if(type != manager_data->xsettings_atom)
			EWARNING(ESTRLOC ": Invalid type for XSETTINGS property\n");
		else if(format != 8)
			EWARNING(ESTRLOC ": Invalid format for XSETTINGS property\n");
		else
			settings = edelib::xsettings_decode(data, n_items, NULL);
		XFree(data);
	}

	if(!settings)
		return false;

	edelib::File setting_file;
	if(!setting_file.open(file, edelib::FIO_WRITE)) {
		EWARNING(ESTRLOC ": Unable to write to %s\n", file);
		edelib::xsettings_list_free(settings);
		return false;
	}

	setting_file.printf("<? xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	setting_file.printf("<ede-settings>\n");

	iter = settings;
	while(iter) {
		setting_file.printf("\t<setting name=\"%s\" ", iter->setting->name);
		switch(iter->setting->type) {
			case edelib::XSETTINGS_TYPE_INT:
				setting_file.printf("type=\"int\" value=\"%i\" />\n", iter->setting->data.v_int);
				break;
			case edelib::XSETTINGS_TYPE_STRING:
				setting_file.printf("type=\"string\" value=\"%s\" />\n", iter->setting->data.v_string);
				break;
			case edelib::XSETTINGS_TYPE_COLOR:
				setting_file.printf("type=\"color\" red=\"%i\" green=\"%i\" blue=\"%i\" alpha=\"%i\" />\n",
						iter->setting->data.v_color.red,
						iter->setting->data.v_color.green,
						iter->setting->data.v_color.blue,
						iter->setting->data.v_color.alpha);
				break;
		}

		iter = iter->next;
	}
	setting_file.printf("</ede-settings>\n");

	setting_file.close();
	edelib::xsettings_list_free(settings);

	xresource_undo();

	return true;
}
