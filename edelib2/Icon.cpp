/*
 * $Id$
 *
 * edelib::Icon - General icon library with support for Icon themes
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include <dirent.h>
#include <sys/stat.h> 
#include <fltk/Group.h>
#include <fltk/filename.h>
 
#include "Icon.h"
#include "Config.h"
#include "../edeconf.h"

using namespace fltk;
using namespace edelib;

// FIXME: use an EDE internal default theme
const char* Icon::iconspath = PREFIX"/share/icons";
const char* Icon::defaulttheme = "crystalsvg";
char* Icon::theme = 0;



// Read configuration to detect currently active theme
void Icon::read_theme_config() 
{
	char temp_value[PATH_MAX];
	if (theme) free(theme);
	
	Config cfg(find_config_file("ede.conf", 0));
	cfg.set_section("Icons");
	if (!cfg.read("IconTheme", temp_value, 0, sizeof(temp_value)))
		theme = strdup(temp_value);

	if (!theme || strlen(theme)<2)
		theme = strdup(defaulttheme);
}


// Change theme in configuration
void Icon::set_theme(const char *t) 
{
	if (theme) free(theme);
	theme = strdup(t);

	Config cfg(find_config_file("ede.conf", true));
	cfg.set_section("Icons");
	cfg.write("IconTheme", theme);
}



// Search theme directory for icon file of given name

// Icon names are unique, regardless of subdirectories. The
// "actions", "apps", "devices" etc. are just an idea that 
// never lived. So we simply flatten the KDE directory
// structure.

// Alternatively, if there are different icons in subdirs with
// same name, you can provide e.g. subdir/filename and it will 
// work.

const char *find_icon(const char *name, const char *directory) 
{
	dirent	**files;
	static char filename[PATH_MAX];

	snprintf (filename, PATH_MAX, "%s/%s", directory, name);
	if (filename_exist(filename)) return filename;
	snprintf (filename, PATH_MAX, "%s/%s.png", directory, name);
	if (filename_exist(filename)) return filename;
	snprintf (filename, PATH_MAX, "%s/%s.xpm", directory, name);
	if (filename_exist(filename)) return filename;

	// Look in subdirectories
	const int num_files = filename_list(directory, &files);
	char dirname[PATH_MAX];
	for (int i=0; i<num_files; i++) {
		if ((strcmp(files[i]->d_name,"./") == 0) || strcmp(files[i]->d_name,"../") == 0) continue;
		snprintf (dirname, PATH_MAX, "%s/%s", directory, files[i]->d_name);
		if (filename_isdir(dirname)) {
			// Cut last slash
			if (dirname[strlen(dirname)-1] == '/')
				dirname[strlen(dirname)-1] = '\0';
			const char *tmp = find_icon(name, dirname);
			if (tmp) return tmp;
		}
	}

	// Not found
	return 0;
}

// Create icon with given "standard" name
Icon* Icon::get(const char *name, int size) 
{
	char directory[PATH_MAX];
	
	// FIXME: rescale icon from other sizes
	if (size!=TINY && size!=SMALL && size!=MEDIUM && size!=LARGE && size!=HUGE)
	{
		fprintf (stderr, "Edelib: ERROR: We don't support icon size %d... using %dx%d\n", size, MEDIUM, MEDIUM);
		size=MEDIUM;
	}

	snprintf (directory, PATH_MAX, "%s/%s/%dx%d", iconspath, get_theme(), size, size);

	const char *icon = find_icon(name, directory);

	if (!icon) // Fallback to icon from default theme, if not found
	{
		snprintf (directory, sizeof(directory), "%s/%s/%dx%d", iconspath, defaulttheme, size, size);
		icon = find_icon(name, directory);
	}
	if (!icon && size!=TINY) // No default icon in this size
	{
		snprintf (directory, sizeof(directory), "%s/%s/%dx%d", iconspath, defaulttheme, TINY, TINY);
		icon = find_icon(name, directory);
		// TODO: scale icon from tiny to requested size
	}
	if (!icon)
		return 0; // sorry
	else
	{
		//fprintf(stderr,"Ikona: %s\n",icon);
		Icon* i = (Icon*)pngImage::get(icon);
		// free(icon); // no need to free icon
		//i->transp_index(255);
		//fprintf (stderr, "Name: %s Transp_index: %d Pixel type: %d\n", name, i->transp_index(), i->pixel_type());
		return i;
	}
}
