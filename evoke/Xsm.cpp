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
#include <string.h>
#include <limits.h>

int ignore_xerrors(Display* display, XErrorEvent* xev) {
	return True;
}

Xsm::Xsm() { }

Xsm::~Xsm() { 
	EDEBUG("Xsm::~Xsm()\n"); 
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
	return true;
}
