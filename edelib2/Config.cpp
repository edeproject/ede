/*
 * $Id$
 *
 * edelib::Config - library for configuration management
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

/*#include "fl_internal.h"

#include <efltk/vsnprintf.h>
#include <efltk/Fl_String_List.h>
#include <efltk/Fl_Exception.h>
#include <efltk/Fl_Config.h>
#include <efltk/filename.h>
#include <efltk/fl_utf8.h>*/

#include "Config.h"
#include <fltk/filename.h>

#include "NLS.h"
#include "../edeconf.h"
#include "Util.h"

/*#include <ctype.h>
#include <locale.h>*/

#include <errno.h>
/*#include <stdlib.h>
#include <stdio.h>*/
#include <sys/stat.h>

/*#ifdef _WIN32_WCE
#include <stdlibx.h>
#endif
//#include <config.h>

#ifdef _WIN32

# include <io.h>
# include <direct.h>
# include <windows.h>
# define access(a,b) _access(a,b)
# define mkdir(a,b) _mkdir(a)
# define R_OK 4

#else

# include <unistd.h>

#endif *//* _WIN32 */


#define LOCALE_TO_C() \
    char *locale = setlocale(LC_ALL, ""); \
    char *restore_locale = locale ? strdup(locale) : strdup("C"); \
    setlocale(LC_ALL, "C")

#define RESTORE_LOCALE() \
    setlocale(LC_ALL, restore_locale); \
    free(restore_locale)

// From Enumerations.h
#ifdef _WIN32
# undef slash
# define slash '\\'
#else
# undef slash
# define slash '/'
#endif
// End Enumerations.h

using namespace fltk;
using namespace edelib;



// GLOBAL NOTE: asprintf() is a GNU extension - if it's unsupported on some
// platforms, use our tasprintf() instead (in Util.h)






// vec_from_string() - similar to explode() in PHP or split() in Perl
// adapted from Fl_String_List to use vector
std::vector<char*> vec_from_string(const char *str, const char *separator)
{
	if(!str) return std::vector<char*> ();

	uint separator_len = strlen(separator);
	const char *ptr = str;
	const char *s = strstr(ptr, separator);
	std::vector<char*> retval;
	if(s) {
		do {
			uint len = s - ptr;
			if (len) {
				retval.push_back(strndup(ptr,len));
			} else {
				retval.push_back(NULL);
			}
		
			ptr = s + separator_len;
			s = strstr(ptr, separator);
		}
		while(s);
		
		if(*ptr) {
			retval.push_back(strdup(ptr));
		}
	} else {
		retval.push_back(strdup(ptr));
	}
	return retval;
}


// Get filename with full path of config file
// TODO: mergeing of system and user config
char* Config::find_file(const char *filename, bool create, int mode)
{
	static char path[PATH_MAX];

	if(is_path_rooted(filename)) {
		strncpy(path, filename, PATH_MAX);
		return (create || !access(path, R_OK)) ? path : 0;
	}
	if(mode==USER) {
		const char *cptr = getenv("HOME");
		char *ret=0;
		if(cptr) {
			snprintf(path, PATH_MAX-1, "%s%c%s%c%s", cptr, slash, ".ede", slash, filename);
			if(create || !access(path, R_OK)) {
				ret = path;
			}
			return ret;
		}
		return 0;
	} else {
		snprintf(path, sizeof(path)-1, "%s%c%s", get_sys_dir(), slash, filename);
		return (create || !access(path, R_OK)) ? path : 0;
	}
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define S(item) ((Config_Section*)item)

Config::Config(const char *vendor, const char *application, int mode)
: Config_Section("","",0)
{
	m_vendor=m_app=m_filename=NULL;
	m_cur_sec = 0;
	m_changed=false;
	m_error = 0;
	
	if(vendor) m_vendor = strdup(vendor);
	if(application) m_app = strdup(application);
	
	if(strlen(m_app) > 0) 
	{
		const char *file=0;
		char tmp[PATH_MAX];
#ifdef _WIN32
		if(mode==SYSTEM)
			snprintf(tmp, sizeof(tmp)-1, "%s%c%s.conf", m_app, slash, m_app);
		else
#endif
			snprintf(tmp, sizeof(tmp)-1, "apps%c%s%c%s.conf", slash, m_app, slash, m_app);
		file = find_file(tmp, true, mode);
		if(file) 
		{
			bool ret = make_path_for_file(file);
			if(ret) 
			{
				m_filename = strdup(file);
				read_file(true);
			} else
				m_error = CONF_ERR_FILE;
		} else
			m_error = CONF_ERR_FILE;
	} else
		m_error = CONF_ERR_FILE;
}

Config::Config(const char *filename, bool read, bool create)
: Config_Section("","",0)
{
	m_vendor=m_app=m_filename=NULL;

	if(filename) m_filename = strdup(filename); else m_filename = strdup("");
// TODO: shouldn't we just return false if there's no filename??
// use case: creating a new file (nonexistant)
    
	m_error = 0;
	m_cur_sec = 0;
	m_changed=false;
	
	if(create && strlen(m_filename)>0) {
		make_path_for_file(m_filename);
	}
	
	if(read) read_file(create);
}

Config::~Config()
{
	flush();
	clear();
	if(m_filename) free(m_filename);
	if(m_vendor) free(m_vendor);
	if(m_app) free(m_app);
}

/* get error string associated with error number */
const char *Config::strerror(int error)
{
	switch(error)
	{
	case CONF_SUCCESS: 	return _("Successful operation");
	case CONF_ERR_FILE: 	return _("Could not access config file");
	case CONF_ERR_SECTION: 	return _("Config file section not found");
	case CONF_ERR_KEY: 	return _("Key not found in section");
	case CONF_ERR_MEMORY: 	return _("Could not allocate memory");
	case CONF_ERR_NOVALUE: 	return _("Invalid value associated with key");
	default: 		return _("Unknown error");
	}
}

bool Config::read_file(bool create)
{
	bool error = false;
	if(m_filename && strlen(m_filename)<1) {
		m_error = CONF_ERR_FILE;
		return false;
	}
	
	if(create && !(access(m_filename, F_OK)==0)) {
		FILE *f = fopen(m_filename, "w+");
		if(f) {
			fputs(" ", f);
			fclose(f);
		} else error=true;
	}
	
	if(error) {
		m_error = CONF_ERR_FILE;
		return false;
	}

	// If somebody calls this function two times, we
	// need to clean earlier section list...
	clear();

	/////
	struct stat fileStat;
	stat(m_filename, &fileStat);
	uint size = fileStat.st_size;
	if(size == 0) {
		m_error = 0;
		return true;
	}
	
	FILE *fp = fopen(m_filename, "r");
	if(!fp) {
		//fprintf(stderr, "fp == 0: %s\n", m_filename);
		m_error = CONF_ERR_FILE;
		return false;
	}
	
	uint bsize = size*sizeof(char);
	char *buffer = (char*)malloc(bsize+1);
	buffer[bsize] = 0;
	if(!buffer) {
		m_error = CONF_ERR_MEMORY;
		return false;
	}
	
	uint readed = fread(buffer, 1, size, fp);
	if(readed <= 0) {
		free((char*)buffer);
		fclose(fp);
		m_error = CONF_ERR_FILE;
		return false;
	}
	fclose(fp);

    /* old parser
    String_List strings(buffer, "\n");

    free((char*)buffer);

    Config_Section *section = this;
    for(uint n=0; n<strings.size(); n++)
    {
        String line;
        int comment_pos = strings[n].rpos('#');
        if(comment_pos>=0) {
            line = strings[n].sub_str(comment_pos, strings[n].length()-comment_pos).trim();
        } else {
            line = strings[n].trim();
        }

        if(line[0] == '[')
        {
            int pos = line.pos(']');
            if(pos>=0)
            {
                String sec(line.sub_str(1, pos-1));
                section = create_section(sec);
            }
        }
        else if(line[0] != '#')
        {
            int pos = line.pos('=');
            if(pos==-1) pos = line.pos(':');
            if(pos>=0) {
                String key(line.sub_str(0, pos));
                pos++;
                String value(line.sub_str(pos, line.length()-pos));
                section->add_entry(key, value);
            }
        }
    }
    */
    
	// new parser by Vedran
	// I like writing parsers
	// too bad others don't like me writing parsers...
	// TODO: i did some stupid things here for debugging, need to check
    
	int pos=0;
	bool comment, iskey, issection;
	const uint maxlen=4096;
	char key[maxlen], value[maxlen], sectionname[maxlen];
	key[0]='\0'; value[0]='\0'; sectionname[0]='\0';
	Config_Section *section = this;
	
	do {
		int c=buffer[pos];
		if ((c == '\n') || (c == '\0')) {
			comment=false; iskey=true; issection=false;
			wstrim(sectionname);
			wstrim(key);
			wstrim(value);
			if (strlen(sectionname) > 0)
				section = create_section(sectionname);
			if (strlen(key) > 0)
				section->add_entry(key,value);
			key[0]='\0'; value[0]='\0'; sectionname[0]='\0';
		}
		else if (c == '#')
			comment = true;
		else if (comment == false) {
			if (c == '[')
				issection = true;
			else if (c == ']')
				issection = false;
			else if ((c == '=') || (c == ':')) 
				iskey = false;
			else {
				if (issection) {
					if (maxlen>strlen(sectionname)+1)
						strcat(sectionname, (const char*) &c);
				} else if (iskey) {
					if (maxlen>strlen(key)+1)
						strcat(key, (const char*) &c);
				} else
					if (maxlen>strlen(value)+1)
						strcat(value, (const char*) &c);
			}
		}
		pos++;
	} while (buffer[pos] != '\0');
    
	m_error = 0;
	m_changed=false;
	return true;
}

bool Config::flush()
{
	if(!m_changed) return true;
	if(strlen(m_filename) < 1) return false;
	
	FILE *file = fopen(m_filename, "w+");
//    if(!file)
//        fl_throw(::strerror(errno));

	LOCALE_TO_C();

	fprintf(file, "# EDE INI Version %s\n", PACKAGE_VERSION);
	if(m_vendor && strlen(m_vendor)>0) fprintf(file, "# Vendor: %s\n", m_vendor);
	if(m_app && strlen(m_app)>0)    fprintf(file, "# Application: %s\n", m_app);

	// Flush sections
	write_section(0, file);

	RESTORE_LOCALE();

	fclose(file);

	m_error = 0;
	m_changed=false;
	return true;
}


Config_Section *Config::create_section(const char* name)
{
	if(strlen(name)<1) return 0;
	
	Config_Section *section = find_section(name, true);
	if(section) return section;
	
	char *lastptr = strrchr(name,'/'); // int pos = name.rpos('/')+1;
	int pos;
	if(lastptr) {
		pos = lastptr-name + 1;
	} else {
		section = new Config_Section(name, "", 0);
		sections().push_back(section);
		return section;
	}
	
	//char* sec_name(name.sub_str(pos, name.length()-pos));
	char *sec_name = strndup(name+pos, strlen(name)-pos);
	//char* sec_path(name.sub_str(0, pos));
	char *sec_path = strndup(name, pos);
	
	Config_Section *parent = find_section(sec_path, false);
	Config_Sections *list = &sections();
	
	if(!parent) {
		// Fl_String_List sections;
		std::vector<char*> sections = vec_from_string(sec_path, "/");
		
		char path[PATH_MAX];
		path[0]='\0';
		for(uint n=0; n<sections.size(); n++) {
			if(parent) list = &parent->sections();
		
			parent = new Config_Section(sections.at(n), path, parent);
			list->push_back(parent);
		
			if (PATH_MAX>strlen(path)+strlen(sections.at(n))+1) {
				strcat(path, sections.at(n));
				strcat(path, "/");
			}
		}
	}

	if(parent) list = &parent->sections();
	
	section = new Config_Section(sec_name, sec_path, parent);
	list->push_back(section);
	
	free(sec_name); free(sec_path);
	m_error = 0;
	return section;
}

Config_Section *Config::find_section(const char *path, bool perfect_match) const
{
	if(!path || !*path) return 0;
	
	std::vector<char*> sections = vec_from_string(path, "/");
	
	if(sections.size()==0)
		return find(path, false);
	
	Config_Section *section = (Config_Section *)this;
	for(uint n=0; n<sections.size(); n++) {
		Config_Section *tmp = section->find(sections.at(n), false);
		if(!tmp) {
			if(perfect_match)
			return 0;
			else
			break;
		}
		section = tmp;
	}
	return section;
}

void Config::remove_key(const char *section, const char *key)
{
	if(key) {
		Config_Section *sect = find_section(section, true);
		if(sect->remove_entry(key)) {
			m_error = 0;
			m_changed = true;
			return;
		}
	}
	m_error = CONF_ERR_KEY;
}

// finding and removing stuff from deque
void sectremove(Config_Sections sects, Config_Section *sect) 
{
	for (uint n=0; n<sects.size(); n++) {
		Config_Section *current = (Config_Section *)sects.at(n);
		if (current == sect)
			sects.erase(sects.begin()+n);
	}
	return;
}

void Config::remove_sec(const char *section)
{
	if(!section) return;
	
	Config_Section *sect;
	if((sect = find_section(section, true)) != 0) {
		if(sect->parent()) {
			sectremove(sect->parent()->sections(),sect);
		} else {
			sectremove(sections(),sect);
		}
		delete sect;
		m_error = 0;
		m_changed = true;
		return;
	}
	m_error = CONF_ERR_SECTION;
}

/*
 *  Read functions
 */

int Config::_read_string(Config_Section *s, const char *key, char *ret, const char *def_value, int size)
{
	if(!key || !s) {
		if(def_value) strncpy(ret, def_value, size);
		else ret[0] = '\0';
		m_error = (!key ? CONF_ERR_KEY : CONF_ERR_SECTION);
		return m_error;
	}

	char *val = s->find_entry(key);
	if(val) {
		int len = strlen(val); // convert from unsigned... and now:
		len = (len<size) ? len+1 : size;
		memcpy(ret, val, len);
		return (m_error = CONF_SUCCESS);
	}
	free(val); 
	
	if(def_value) strncpy(ret, def_value, size);
	else ret[0] = '\0';
	
	m_error = CONF_ERR_KEY;
	return m_error;
}

int Config::_read_string(Config_Section *s, const char *key, char *&ret, const char *def_value)
{
	if(!key || !s) {
		if (def_value) ret=strdup(def_value); else ret=0;
		if (!key) m_error = CONF_ERR_KEY; else m_error = CONF_ERR_SECTION;
		return m_error;
	}
	
	char *val = s->find_entry(key);
	if(val && strlen(val)>0)
	{
		ret = strdup(val);
		return (m_error = CONF_SUCCESS);
	}
	free(val);
	
	if (def_value) ret = strdup(def_value); else ret=0;
	m_error = CONF_ERR_KEY;
	return m_error;
}

/*int Config::_read_string(Config_Section *s, const char *key, Fl_String &ret, const char *def_value)
{
    if(!key || !s) {
        ret = def_value;
        m_error = !key ? CONF_ERR_KEY : CONF_ERR_SECTION;
        return m_error;
    }

    Fl_String *val = s->find_entry(key);
    if(val) {
        ret = (*val);
        return (m_error = CONF_SUCCESS);
    }

    ret = def_value;
    return (m_error = CONF_ERR_KEY);
}*/

int Config::_read_long(Config_Section *s, const char *key, long &ret, long def_value)
{
	char* tmp;
	if(!_read_string(s, key, tmp, 0))
		if (tmp[0]) ret=strtol(tmp, NULL, 10); else ret=def_value;
	else
		ret = def_value;
	return m_error;
}

int Config::_read_int(Config_Section *s, const char *key, int &ret, int def_value)
{
	char* tmp;
	if(!_read_string(s, key, tmp, 0)) {
		ret = atoi(tmp);
		if ((errno == ERANGE) || (ret == 0 && strcmp(tmp,"0") != 0)) ret = def_value;
	} else
		ret = def_value;
	return m_error;
}

int Config::_read_float (Config_Section *s, const char *key, float &ret, float def_value)
{
	char* tmp;
	if(!_read_string(s, key, tmp, 0)) {
		LOCALE_TO_C();
		ret = (float)strtod(tmp, 0);
		RESTORE_LOCALE();
	} else
		ret = def_value;
	return m_error;
}

int Config::_read_double(Config_Section *s, const char *key, double &ret, double def_value)
{
	char* tmp;
	if(!_read_string(s, key, tmp, 0)) {
		LOCALE_TO_C();
		ret = strtod(tmp, 0);
		RESTORE_LOCALE();
	} else
		ret = def_value;
	return m_error;
}

int Config::_read_bool(Config_Section *s, const char *key, bool &ret, bool def_value)
{
	char* tmp;
	if(_read_string(s, key, tmp, 0)) {
		ret = def_value;
		return m_error;
	}
	if ((strncasecmp(tmp,"true",4)==0) 
	|| (strncasecmp(tmp,"yes",3)==0) 
	|| (strncasecmp(tmp,"on",2)==0) 
	|| (strcasecmp(tmp,"1")==0)) {
		ret = true;
	} else if((strncasecmp(tmp,"false",5)==0) 
	|| (strncasecmp(tmp,"no",2)==0) 
	|| (strncasecmp(tmp,"off",3)==0) 
	|| (strcasecmp(tmp,"0")==0)) {
		ret = false;
	} else {
		m_error = CONF_ERR_NOVALUE;
		ret = def_value;
	}
	return m_error;
}

int Config::_read_color(Config_Section *s, const char *key, Color &ret, Color def_value)
{
	char* tmp;
	if(_read_string(s, key, tmp, 0)) {
		ret = def_value;
		return m_error;
	}
	
	int r=0,g=0,b=0;
	if(sscanf(tmp, "RGB(%d,%d,%d)", &r, &g, &b)!=3) {
		ret = def_value;
		return (m_error = CONF_ERR_NOVALUE);
	}
	ret = color(r,g,b);
	return m_error;
}

/*
 *  Write functions
 */

/*int Config::_write_string(Config_Section *s, const char *key, const char *value)
{
    char* val(value);
    return _write_string(s, key, val);
}*/

int Config::_write_string(Config_Section *s, const char *key, const char* value)
{
	if(!s) return (m_error = CONF_ERR_SECTION);
	if(!key) return (m_error = CONF_ERR_KEY);

/*  This logic is now in add_entry, cause we can't pass around pointers into structure

	char *val = s->find_entry(key);
	if(val) {
		strncpy(val, value, strlen(value));
	} else */
	if (value) s->add_entry(key, value); else s->add_entry(key, "");
	
	m_changed=true;
	return (m_error=CONF_SUCCESS);
}

int Config::_write_long(Config_Section *s, const char *key, const long value)
{
	return _write_string(s, key, tsprintf("%ld", value));
}

int Config::_write_int(Config_Section *s, const char *key, const int value)
{
	return _write_string(s, key, tsprintf("%d", value));
}

int Config::_write_float(Config_Section *s, const char *key, const float value)
{
	LOCALE_TO_C();
	char tmp[32]; snprintf(tmp, sizeof(tmp)-1, "%g", value);
	RESTORE_LOCALE();
	return _write_string(s, key, tmp);
}

int Config::_write_double(Config_Section *s, const char *key, const double value)
{
	LOCALE_TO_C();
	char tmp[32]; snprintf(tmp, sizeof(tmp)-1, "%g", value);
	RESTORE_LOCALE();
	return _write_string(s, key, tmp);
}

int Config::_write_bool(Config_Section *s, const char *key, const bool value)
{
	if(value) return _write_string(s, key, "1");
	return _write_string(s, key, "0");
}

int Config::_write_color(Config_Section *s, const char *key, const Color value)
{
	unsigned char r,g,b;
	split_color(value, r,g,b);
	return _write_string(s, key, tsprintf("RGB(%d,%d,%d)", r,g,b));
}

//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

Config_Section::Config_Section(const char* name, const char* path, Config_Section *par)
: m_parent(par)
{
	m_name=strdup(name);
	m_path=strdup(path);
}

Config_Section::~Config_Section()
{
	free(m_name);
	free(m_path);
	clear();
}

void Config_Section::clear()
{
	for(uint n=0; n<sections().size(); n++) {
		Config_Section *s = (Config_Section *)sections()[n];
		delete s;
	}
	m_lines.clear();
	m_sections.clear();
}

void Config_Section::write_section(int indent, FILE *fp) const
{
	for(int a=0; a<indent; a++) fprintf(fp, " ");

	if(strlen(name())>0)
		fprintf(fp, "[%s%s]\n", path(), name());

	for(uint n=0; n<m_lines.size(); n++) {
		if(strlen(m_lines.at(n)) > 0) {
			for(int a=0; a<indent; a++) fprintf(fp, " ");
			// fprintf(fp, "  %s=%s\n", it.id().c_str(), it.value().c_str());
			fprintf(fp, "  %s\n", m_lines.at(n));
		}
	}
	
	fprintf(fp, "\n");
	
	for(uint n=0; n<sections().size(); n++) {
		Config_Section *child = S(sections()[n]);
		child->write_section(indent+2, fp);
	}
}

void Config_Section::add_entry(const char* key, const char* value)
{
	if(!key || strlen(key)<1) return;
	if(!value) return;

	char *kvpair;
	char *m_key = wstrim(strdup(key));
	char *m_value = wstrim(strdup(value));
	asprintf(&kvpair,"%s=%s",m_key,m_value);
	free (m_key); free (m_value);

	// if key already exists, delete
	bool found = false;
	for (uint i=0; i<lines().size(); i++) {
		if (!found && strncmp(lines().at(i), kvpair, strlen(kvpair)) == 0) {
			lines().erase(lines().begin()+i);
			found = true;
		}
	}

	lines().push_back(kvpair);
}

bool Config_Section::remove_entry(const char* key)
{
	if (!key || strlen(key)<1) return false;
	bool found=false;

	char *search;
	asprintf(&search,"%s=",twstrim(key));
	
	for (uint i=0; i<lines().size(); i++) {
		if (strncmp(lines().at(i), search, strlen(search)) == 0) {
			lines().erase(lines().begin()+i);
			found=true;
		}
	}
	free(search);
	return found;
}

char* Config_Section::find_entry(const char *key) const
{
	if (!key || strlen(key)<1) return 0;
	bool found=false;
	char *ret;

	char *search;
//	asprintf(&search,"%s=",twstrim(key));
	char *m_key = wstrim(strdup(key));
	asprintf(&search,"%s=",m_key);	
	free(m_key);
	
	for (uint i=0; i<lines().size(); i++) {
		if (!found && strncmp(lines().at(i), search, strlen(search)) == 0) {
			ret = strdup(lines().at(i)+strlen(search));
			found = true;
		}
	}
	
	free(search);	// this sometimes fails, dunno why....
	if (found) { return ret; } else { return 0; }
}

Config_Section *Config_Section::find(const char *name, bool recursive) const
{
	const Config_Sections *list = &sections();
	if (!name) return 0;

	for(uint n=0; n<list->size(); n++) {
		Config_Section *s = (Config_Section*) list->at(n);
		if(strncmp(s->name(), name, strlen(name)) == 0)
			return s;
		if(recursive) {
			s = s->find(name, recursive);
			if(s) return s;
		}
	}
	return 0;
}
