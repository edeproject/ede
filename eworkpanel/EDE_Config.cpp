// EDE_Config.cpp: implementation of the EDE_Config class.
//
//////////////////////////////////////////////////////////////////////

/*#include "fl_internal.h"

#include <efltk/vsnprintf.h>
#include <efltk/Fl_String_List.h>
#include <efltk/Fl_Exception.h>
#include <efltk/Fl_Config.h>
#include <efltk/filename.h>
#include <efltk/fl_utf8.h>*/

#include "EDE_Config.h"
#include <fltk/filename.h>
#include "NLS.h"
#include <ctype.h>
#include <locale.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32_WCE
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

#endif /* _WIN32 */

// This is used for comment inside config files:
#define EDE_VERSION	2.0

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

// From config.h
#define CONFIGDIR "/usr/share/ede"
// End config.h
    
static int is_path_rooted(const char *fn)
{
    /* see if an absolute name was given: */
#ifdef _WIN32
    if (fn[0] == '/' || fn[0] == '.' || fn[0] == '\\' || fn[1]==':')
#else
    if (fn[0] == '/' || fn[0] == '.')
#endif
        return 1;
    return 0;
}

// recursively create a path in the file system
static bool makePath( const char *path ) {
    if(access(path, 0)) {
        const char *s = strrchr( path, slash );
        if ( !s ) return 0;
        int len = s-path;
        char *p = (char*)malloc( len+1 );
        memcpy( p, path, len );
        p[len] = 0;
        makePath( p );
        free( p );
        return ( mkdir( path, 0777 ) == 0 );
    }
    return true;
}

// strip the filename and create a path
static bool makePathForFile( const char *path )
{
    const char *s = strrchr( path, slash );
    if ( !s ) return false;
    int len = s-path;
    char *p = (char*)malloc( len+1 );
    memcpy( p, path, len );
    p[len] = 0;
    bool ret=makePath( p );
    free( p );
    return ret;
}

char *get_sys_dir() {
#ifndef _WIN32
    return CONFIGDIR;
#else
    static char path[PATH_MAX];
    HKEY hKey;
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ, &hKey)==ERROR_SUCCESS)
    {
        DWORD size=4096;
        RegQueryValueExW(hKey, L"CommonFilesDir", NULL, NULL, (LPBYTE)path, &size);
        RegCloseKey(hKey);
        return path;
    }
    return "C:\\EDE\\";
#endif
}

char *get_homedir() {
   char *path = new char[PATH_MAX];
	const char *str1;

    str1=getenv("HOME");
    if (str1) {
         memcpy(path, str1, strlen(str1)+1);
         return path;
    }

    return 0;
}


char *EDE_Config::find_config_file(const char *filename, bool create, int mode)
{
    static char path[4096];

    if(is_path_rooted(filename)) {
        strncpy(path, filename, sizeof(path));
        return (create || !access(path, R_OK)) ? path : 0;
    }
    if(mode==USER) {
        char *cptr = get_homedir();
        char *ret=0;
        if(cptr) {
            snprintf(path, sizeof(path)-1, "%s%c%s%c%s", cptr, slash, ".ede", slash, filename);
            if(create || !access(path, R_OK)) {
                ret = path;
            }
            delete []cptr;
            return ret;
        }
		return 0;
    } else {
        snprintf(path, sizeof(path)-1, "%s%c%s", get_sys_dir(), slash, filename);
        return (create || !access(path, R_OK)) ? path : 0;
    }
}


// Vedran - a few string management functions...

// strdupcat() - it's cool to strcat with implied realloc
//  -- NOTE: due to use of realloc *always* use strdupcat return value:
//            dest = strdupcat(dest,src);
// and *never* use it like:
//            strdupcat(dest,src);
char *strdupcat(char *dest, const char *src)
{
	if (!dest) {
		dest=(char*)malloc(strlen(src));
	} else {
		dest=(char*)realloc (dest, strlen(dest)+strlen(src)+1);
	}
	strcat(dest,src);
	return dest;
}


// wstrim() - for trimming characters (used in parser)
// parts of former fl_trimleft and fl_trimright from Fl_Util.cpp
char* wstrim(char *string)
{
    char *start;

    if(string == NULL )
        return NULL;

    if (*string) {
        int len = strlen(string);
        if (len) {
            char *p = string + len;
            do {
                p--;
                if ( !isspace(*p) ) break;
            } while ( p != string );
            
            if ( !isspace(*p) ) p++;
            *p = 0;
        }
    }
    
    for(start = string; *start && isspace (*start); start++);
    memmove(string, start, strlen(start) + 1);

    return string;
}

// hmmmh?
//char* wstrim(const char *string)
//{
//    char *newstring = strdup(string);
//    return wstrim(newstring);
//}


// from_string() - adapted from Fl_String_List to use vector
std::vector<char*> from_string(const char *str, const char *separator)
{
    if(!str) return std::vector<char*> ();

    const char *ptr = str;
    const char *s = strstr(ptr, separator);
    std::vector<char*> retval;
    if(s) {
        unsigned separator_len = strlen(separator);
        do {
            unsigned len = s - ptr;
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



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define S(item) ((EDE_Config_Section*)item)

EDE_Config::EDE_Config(const char *vendor, const char *application, int mode)
: EDE_Config_Section("","",0)
{
    m_vendor=m_app=m_filename=NULL;
    m_cur_sec = 0;
    m_changed=false;
    m_error = 0;

    if(vendor) m_vendor = strdup(vendor);
    if(application) m_app = strdup(application);

    if(strlen(m_app) > 0) {
        const char *file=0;
        char tmp[PATH_MAX];
#ifdef _WIN32
        if(mode==SYSTEM)
            snprintf(tmp, sizeof(tmp)-1, "%s%c%s.conf", m_app, slash, m_app);
        else
#endif
            snprintf(tmp, sizeof(tmp)-1, "apps%c%s%c%s.conf", slash, m_app, slash, m_app);
        file = find_config_file(tmp, true, mode);
        if(file) {
            bool ret = makePathForFile(file);
            if(ret) {
	        m_filename = strdup(file);
                read_file(true);
            } else
                m_error = CONF_ERR_FILE;
        } else
            m_error = CONF_ERR_FILE;
    } else
        m_error = CONF_ERR_FILE;
}

EDE_Config::EDE_Config(const char *filename, bool read, bool create)
: EDE_Config_Section("","",0)
{
    m_vendor=m_app=m_filename=NULL;
    
    if(filename) m_filename = strdup(filename); else m_filename = strdup("");
// TODO: shouldn't we just return false if there's no filename??
// use case: creating a new file (nonexistant)
    
    m_error = 0;
    m_cur_sec = 0;
    m_changed=false;

    if(create && strlen(m_filename)>0) {
        makePathForFile(m_filename);
    }

    if(read) read_file(create);
}

EDE_Config::~EDE_Config()
{
    flush();
    clear();
    if(m_filename) free(m_filename);
    if(m_vendor) free(m_vendor);
    if(m_app) free(m_app);
}

/* get error string associated with error number */
const char *EDE_Config::strerror(int error)
{
    switch(error)
    {
    case CONF_SUCCESS:     return _("Successful completion");
    case CONF_ERR_FILE:    return _("Could not access config file");
    case CONF_ERR_SECTION: return _("Config file section not found");
    case CONF_ERR_KEY:     return _("Key not found in section");
    case CONF_ERR_MEMORY:  return _("Could not allocate memory");
    case CONF_ERR_NOVALUE: return _("Invalid value associated with key");
    default:               return _("Unknown error");
    }
}

bool EDE_Config::read_file(bool create)
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
    unsigned int size = fileStat.st_size;
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

    unsigned bsize = size*sizeof(char);
    char *buffer = (char*)malloc(bsize+1);
    buffer[bsize] = 0;
    if(!buffer) {
        m_error = CONF_ERR_MEMORY;
        return false;
    }

    unsigned int readed = fread(buffer, 1, size, fp);
    if(readed <= 0) {
        free((char*)buffer);
        fclose(fp);
        m_error = CONF_ERR_FILE;
        return false;
    }
    fclose(fp);

    /* old parser
    EDE_String_List strings(buffer, "\n");

    free((char*)buffer);

    EDE_Config_Section *section = this;
    for(unsigned n=0; n<strings.size(); n++)
    {
        EDE_String line;
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
                EDE_String sec(line.sub_str(1, pos-1));
                section = create_section(sec);
            }
        }
        else if(line[0] != '#')
        {
            int pos = line.pos('=');
            if(pos==-1) pos = line.pos(':');
            if(pos>=0) {
                EDE_String key(line.sub_str(0, pos));
                pos++;
                EDE_String value(line.sub_str(pos, line.length()-pos));
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
    char *key, *value, *sectionname;
    key=strdup(""); value=strdup(""); sectionname=strdup("");
    EDE_Config_Section *section = this;
    do {
    	int c=buffer[pos];
	if ((c == '\n') || (c == '\0')) {
		comment=false; iskey=true; issection=false;
		sectionname = wstrim(sectionname);
		key = wstrim(key);
		value = wstrim(value);
		if (strlen(sectionname) > 0)
			section = create_section(sectionname);
		if (strlen(key) > 0)
			section->add_entry(key,value);
		free(sectionname); free(key); free(value);
		key=strdup(""); value=strdup(""); sectionname=strdup("");
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
			if (issection)
				sectionname = strdupcat(sectionname, (const char*) &c);
			else if (iskey)
				key = strdupcat(key, (const char*) &c);
			else
				value = strdupcat(value,(const char*) &c);
		}
	}
	pos++;
    } while (buffer[pos] != '\0');
    
    free(key); free(value); free(sectionname);
    m_error = 0;
    m_changed=false;
    return true;
}

bool EDE_Config::flush()
{
    if(!m_changed) return true;
    if(strlen(m_filename) < 1) return false;

    FILE *file = fopen(m_filename, "w+");
//    if(!file)
//        fl_throw(::strerror(errno));

    LOCALE_TO_C();

    fprintf(file, "# EDE INI Version %f\n", EDE_VERSION);
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


EDE_Config_Section *EDE_Config::create_section(const char* name)
{
    if(strlen(name)<1) return 0;

    EDE_Config_Section *section = find_section(name, true);
    if(section) return section;

    char *lastptr = strrchr(name,'/'); // int pos = name.rpos('/')+1;
    int pos;
    if(lastptr) {
        pos = lastptr-name + 1;
    } else {
        section = new EDE_Config_Section(name, "", 0);
        sections().push_back(section);
        return section;
    }

    //char* sec_name(name.sub_str(pos, name.length()-pos));
    char *sec_name = strndup(name+pos, strlen(name)-pos);
    //char* sec_path(name.sub_str(0, pos));
    char *sec_path = strndup(name, pos);

    EDE_Config_Section *parent = find_section(sec_path, false);
    EDE_Config_Sections *list = &sections();

    if(!parent) {
	// Fl_String_List sections;
        std::vector<char*> sections = from_string(sec_path, "/");

        char* path = strdup("");
        for(unsigned n=0; n<sections.size(); n++) {
            if(parent) list = &parent->sections();

            parent = new EDE_Config_Section(sections.at(n), path, parent);
            list->push_back(parent);

            path = strdupcat (path, sections.at(n));
            path = strdupcat (path, (char *)'/');
	}
	free(path);
    }

    if(parent) list = &parent->sections();

    section = new EDE_Config_Section(sec_name, sec_path, parent);
    list->push_back(section);

    free(sec_name); free(sec_path);
    m_error = 0;
    return section;
}

EDE_Config_Section *EDE_Config::find_section(const char *path, bool perfect_match) const
{
    if(!path || !*path) return 0;

    std::vector<char*> sections = from_string(path, "/");

    if(sections.size()==0)
        return find(path, false);

    EDE_Config_Section *section = (EDE_Config_Section *)this;
    for(unsigned n=0; n<sections.size(); n++) {
        EDE_Config_Section *tmp = section->find(sections.at(n), false);
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

void EDE_Config::remove_key(const char *section, const char *key)
{
    if(key) {
        EDE_Config_Section *sect = find_section(section, true);
        if(sect->remove_entry(key)) {
            m_error = 0;
            m_changed = true;
            return;
        }
    }
    m_error = CONF_ERR_KEY;
}

// finding and removing stuff from deque
void sectremove(EDE_Config_Sections sects, EDE_Config_Section *sect) {
	for (unsigned int n=0; n<sects.size(); n++) {
		EDE_Config_Section *current = (EDE_Config_Section *)sects.at(n);
		if (current == sect)
			sects.erase(sects.begin()+n);
	}
	return;
}

void EDE_Config::remove_sec(const char *section)
{
    if(!section) return;

    EDE_Config_Section *sect;
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

int EDE_Config::_read_string(EDE_Config_Section *s, const char *key, char *ret, const char *def_value, int size)
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

int EDE_Config::_read_string(EDE_Config_Section *s, const char *key, char *&ret, const char *def_value)
{
    if(!key || !s) {
        ret = def_value?strdup(def_value):0;
        m_error = (!key ? CONF_ERR_KEY : CONF_ERR_SECTION);
        return m_error;
    }

    char *val = s->find_entry(key);
    if(val && strlen(val)>0)
    {
        ret = strdup(val);
        return (m_error = CONF_SUCCESS);
    }
    free(val);

    ret = def_value ? strdup(def_value) : 0;
    m_error = CONF_ERR_KEY;
    return m_error;
}

/*int EDE_Config::_read_string(EDE_Config_Section *s, const char *key, Fl_String &ret, const char *def_value)
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

int EDE_Config::_read_long(EDE_Config_Section *s, const char *key, long &ret, long def_value)
{
    char* tmp;
    if(!_read_string(s, key, tmp, 0)) {
        ret = tmp[0] ? strtol(tmp, NULL, 10) : def_value;
    } else
        ret = def_value;
    return m_error;
}

int EDE_Config::_read_int(EDE_Config_Section *s, const char *key, int &ret, int def_value)
{
    char* tmp;
    if(!_read_string(s, key, tmp, 0)) {
        ret = atoi(tmp);
	if ((errno = ERANGE) || (ret == 0 && strcmp(tmp,"0") != 0)) ret = def_value;
    } else
        ret = def_value;
    return m_error;
}

int EDE_Config::_read_float (EDE_Config_Section *s, const char *key, float &ret, float def_value)
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

int EDE_Config::_read_double(EDE_Config_Section *s, const char *key, double &ret, double def_value)
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

int EDE_Config::_read_bool(EDE_Config_Section *s, const char *key, bool &ret, bool def_value)
{
    char* tmp;
    if(_read_string(s, key, tmp, 0)) {
        ret = def_value;
        return m_error;
    }
    if ((strncasecmp(tmp,"true",4)) ||
        (strncasecmp(tmp,"yes",3)) ||
        (strncasecmp(tmp,"on",2)) ||
	(strcasecmp(tmp,"1"))) {
        ret = true;
    } else if((strncasecmp(tmp,"false",5)) ||
              (strncasecmp(tmp,"no",2)) ||
              (strncasecmp(tmp,"off",3)) ||
              (strcasecmp(tmp,"0"))) {
        ret = false;
    } else {
        m_error = CONF_ERR_NOVALUE;
        ret = def_value;
    }
    return m_error;
}

int EDE_Config::_read_color(EDE_Config_Section *s, const char *key, fltk::Color &ret, fltk::Color def_value)
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
    ret = fltk::color(r,g,b);
    return m_error;
}

/*
 *  Write functions
 */

/*int EDE_Config::_write_string(EDE_Config_Section *s, const char *key, const char *value)
{
    char* val(value);
    return _write_string(s, key, val);
}*/

int EDE_Config::_write_string(EDE_Config_Section *s, const char *key, const char* value)
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

int EDE_Config::_write_long(EDE_Config_Section *s, const char *key, const long value)
{
    char tmp[128]; snprintf(tmp, sizeof(tmp)-1, "%ld", value);
    return _write_string(s, key, tmp);
}

int EDE_Config::_write_int(EDE_Config_Section *s, const char *key, const int value)
{
    char tmp[128]; snprintf(tmp, sizeof(tmp)-1, "%d", value);
    return _write_string(s, key, tmp);
}

int EDE_Config::_write_float(EDE_Config_Section *s, const char *key, const float value)
{
    LOCALE_TO_C();
    char tmp[32]; snprintf(tmp, sizeof(tmp)-1, "%g", value);
    RESTORE_LOCALE();
    return _write_string(s, key, tmp);
}

int EDE_Config::_write_double(EDE_Config_Section *s, const char *key, const double value)
{
    LOCALE_TO_C();
    char tmp[32]; snprintf(tmp, sizeof(tmp)-1, "%g", value);
    RESTORE_LOCALE();
    return _write_string(s, key, tmp);
}

int EDE_Config::_write_bool(EDE_Config_Section *s, const char *key, const bool value)
{
    if(value) return _write_string(s, key, "1");
    return _write_string(s, key, "0");
}

int EDE_Config::_write_color(EDE_Config_Section *s, const char *key, const fltk::Color value)
{
    unsigned char r,g,b;
    fltk::split_color(value, r,g,b);
    char tmp[32];
    snprintf(tmp, sizeof(tmp)-1, "RGB(%d,%d,%d)", r,g,b);
    return _write_string(s, key, tmp);
}

//////////////////////////////////////
//////////////////////////////////////
//////////////////////////////////////

EDE_Config_Section::EDE_Config_Section(const char* name, const char* path, EDE_Config_Section *par)
: m_parent(par)
{
	m_name=strdup(name);
	m_path=strdup(path);
}

EDE_Config_Section::~EDE_Config_Section()
{
    free(m_name);
    free(m_path);
    clear();
}

void EDE_Config_Section::clear()
{
    for(unsigned n=0; n<sections().size(); n++) {
        EDE_Config_Section *s = (EDE_Config_Section *)sections()[n];
        delete s;
    }
    m_lines.clear();
    m_sections.clear();
}

void EDE_Config_Section::write_section(int indent, FILE *fp) const
{
    for(int a=0; a<indent; a++) fprintf(fp, " ");

    if(strlen(name())>0)
        fprintf(fp, "[%s%s]\n", path(), name());

    for(unsigned n=0; n<m_lines.size(); n++) {
        if(strlen(m_lines.at(n)) > 0) {
            for(int a=0; a<indent; a++) fprintf(fp, " ");
//            fprintf(fp, "  %s=%s\n", it.id().c_str(), it.value().c_str());
            fprintf(fp, "  %s\n", m_lines.at(n));
        }
    }

    fprintf(fp, "\n");

    for(unsigned n=0; n<sections().size(); n++) {
        EDE_Config_Section *child = S(sections()[n]);
        child->write_section(indent+2, fp);
    }
}

void EDE_Config_Section::add_entry(const char* key, const char* value)
{
    int keylen = strlen(key);
    if(!key || keylen<1) return;
    if(!value) return;
    
    char *tmp = strdup(key);
    char *tmp2 = strdup(value);
    tmp = wstrim(tmp);
    tmp2 = wstrim(tmp2);
    tmp = strdupcat(tmp, "=");
    tmp = strdupcat(tmp, tmp2);
    free(tmp2);

    // if key already exists, delete
    bool found = false;
    for (unsigned i=0; i<lines().size(); i++) {
        if (!found && strncmp(lines().at(i), tmp, keylen) == 0) {
            lines().erase(lines().begin()+i);
	    found = true;
	}
    }
    
    lines().push_back(tmp);
}

bool EDE_Config_Section::remove_entry(const char* key)
{
    bool found=false;
    char *tmp = strdup(key);
    tmp = strdupcat (tmp, "=");

    for (unsigned i=0; i<lines().size(); i++) {
	if (strncmp(lines().at(i), tmp, strlen(tmp)) == 0) {
		lines().erase(lines().begin()+i);
		found=true;
        }
    }
    free(tmp);
    return found;
}

char* EDE_Config_Section::find_entry(const char *key) const
{
    bool found=false;
    char *ret;
    char *search = strdup(key);
    search = strdupcat(search, "=");

    for (unsigned i=0; i<lines().size(); i++) {
        if (!found && strncmp(lines().at(i), search, strlen(search)) == 0) {
	    ret = strdup(lines().at(i)+strlen(search));
            found = true;
	}
    }

    free(search);	// this sometimes fails, dunno why....
    if (found) { return ret; } else { return 0; }
}

EDE_Config_Section *EDE_Config_Section::find(const char *name, bool recursive) const
{
    const EDE_Config_Sections *list = &sections();
    

    for(uint n=0; n<list->size(); n++) {
        EDE_Config_Section *s = (EDE_Config_Section*) list->at(n);
        if(strcmp(s->name(), name) == 0) {
            return s;
        }
        if(recursive) {
            s = s->find(name, recursive);
            if(s) return s;
        }
    }
    return 0;
}
