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

// TODO: add support for xpm, use functions from fltk/filename.h

char *find_icon(const char *name, const char *directory) 
{
	DIR *my_dir; 
	struct dirent *my_dirent;
	char filename[PATH_MAX];
	
	// Buffer for stat()
	struct stat *buf = (struct stat*)malloc(sizeof(struct stat));

	if (!(my_dir = opendir(directory))) 
	{
		// Directory doesn't exist!
		fprintf (stderr, "Edelib: ERROR: Theme %s is no more on disk :(\n", directory);
		free(buf);
		return 0;
	}
	while ((my_dirent = readdir(my_dir)) != NULL) 
	{
		// filename is a fully qualified path
		snprintf (filename, sizeof(filename), "%s/%s", directory, my_dirent->d_name);

		// strip .png part
		char *work = strdup(my_dirent->d_name);
		char *d = strrchr(work,'.');
		if (d && (strcmp(d, ".png") == 0))
		{
			*d = '\0';
			if (strcmp(work, name)==0) 
			{
				//free(work); // this cause stack corruption sometimes!!
				closedir(my_dir);
				free(buf);
				return strdup(filename); // found!
			}
		}
		//free(work); // this cause stack corruption sometimes!!
		// FIXME

		// Let's try subdirectories
		stat (filename, buf);
		if (strcmp(my_dirent->d_name,".")!=0 && strcmp(my_dirent->d_name,"..")!=0 && S_ISDIR(buf->st_mode)) 
		{
			// filename is a directory, look inside:
			char *tmp = find_icon(name, filename);
			if (tmp) { // found
				closedir(my_dir);
				free(buf);
				return tmp;
			}
		}
	}
	closedir(my_dir);
	free(buf);

	// not found
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

	snprintf (directory, sizeof(directory), "%s/%s/%dx%d", iconspath, get_theme(), size, size);

	char *icon = find_icon(name, directory);

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
		free(icon);
		//i->transp_index(255);
		//fprintf (stderr, "Name: %s Transp_index: %d Pixel type: %d\n", name, i->transp_index(), i->pixel_type());
		return i;
	}
}
