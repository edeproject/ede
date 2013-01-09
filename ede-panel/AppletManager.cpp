/*
 * $Id$
 *
 * Copyright (C) 2012 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <dlfcn.h>
#include <string.h>
#include <edelib/Debug.h>

#include "AppletManager.h"
#include "Panel.h"

EDELIB_NS_USING(String)
EDELIB_NS_USING(list)

struct AppletData {
	void                  *dl;
	Fl_Widget             *awidget; /* widget from the applet */
	AppletInfo            *ainfo;   /* applet informations */

	applet_create_t       create_func;
	applet_destroy_t      destroy_func;

	applet_destroy_info_t destroy_info_func;
};

static void clear_applet(AppletData *a) {
	E_RETURN_IF_FAIL(a != NULL);

	/* clear applet information first */
	if(a->ainfo) {
		E_DEBUG(E_STRLOC ": Cleaning class %s\n", a->ainfo->klass_name);
		(a->destroy_info_func)(a->ainfo);
	}

	/* clear applet specific suff */
	(a->destroy_func)(a->awidget);

	dlclose(a->dl);
	delete a;
}

AppletManager::~AppletManager() {
	clear();
}

bool AppletManager::load(const char *path) {
	dlerror();
	const char *dl_err = NULL;

	void *a = dlopen(path, RTLD_LAZY);
	if(!a) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Unable to load '%s' : '%s'\n", path, dl_err);
		return false;
	}

	/* first check if we have valid plugin by requesting version function */
	void *av = (applet_version_t*)dlsym(a, "ede_panel_applet_get_iface_version");
	if(!av) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Invalid ede-panel plugin. The plugin will not be loaded\n");
		return false;
	}

	applet_version_t avf = (applet_version_t)av;
	if((avf)() > EDE_PANEL_APPLET_INTERFACE_VERSION) {
		E_WARNING(E_STRLOC ": This plugin requries newer ede-panel version\n");
		return false;
	}

	void *ac = (applet_create_t*)dlsym(a, "ede_panel_applet_create");
	if(!ac) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Unable to find 'create' function in ede-panel plugin: '%s'\n", dl_err);
		return false;
	}

	void *ad = (applet_destroy_t*)dlsym(a, "ede_panel_applet_destroy");
	if(!ad) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Unable to find 'destroy' function in ede-plugin plugin: '%s'\n", dl_err);
		return false;
	}

	void *agi = (applet_get_info_t*)dlsym(a, "ede_panel_applet_get_info");
	if(!agi) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Don't know how to fetch applet information: '%s'\n", dl_err);
		return false;
	}

	void *adi = (applet_destroy_info_t*)dlsym(a, "ede_panel_applet_destroy_info");
	if(!adi) {
		dl_err = dlerror();
		E_WARNING(E_STRLOC ": Don't know how to clean applet information: '%s'\n", dl_err);
		return false;
	}

	AppletData *data = new AppletData;
	data->dl = a;
	data->awidget = NULL;

	data->create_func       = (applet_create_t)ac;
	data->destroy_func      = (applet_destroy_t)ad;
	data->destroy_info_func = (applet_destroy_info_t)adi;

	applet_get_info_t get_info_func = (applet_get_info_t)agi;

	/* load applet info first, so we can even use it when widget wasn't created yet */
	data->ainfo = (get_info_func)();
	E_DEBUG(E_STRLOC ": Loading class %s\n", data->ainfo->klass_name);

	applet_list.push_back(data);
	return true;
}

void AppletManager::clear(void) {
	E_RETURN_IF_FAIL(applet_list.size() > 0);

	AListIter it = applet_list.begin(), ite = applet_list.end();
	while(it != ite) {
		clear_applet(*it);
		it = applet_list.erase(it);
	}
}

/*
 * Must be called so widget can actually be added to FLTK parent. Widgets will be created and
 * added to the group.
 */
void AppletManager::fill_group(Panel *p) {
	AListIter it = applet_list.begin(), ite = applet_list.end();
	AppletData *applet;

	for(; it != ite; ++it) {
		applet = *it;

		/* allocate memory for widget and append it to the group */
		applet->awidget = applet->create_func();
		p->add(applet->awidget);
	}
}

void AppletManager::unfill_group(Panel *p) {
	AListIter it = applet_list.begin(), ite = applet_list.end();

	for(; it != ite; ++it)
		p->remove((*it)->awidget);
}

bool AppletManager::get_applet_options(Fl_Widget *o, unsigned long &opts) {
	AListIter it = applet_list.begin(), ite = applet_list.end();

	for(; it != ite; ++it) {
		if(o == (*it)->awidget) {
			opts = (*it)->ainfo->options;
			return true;
		}
	}

	return false;
}
