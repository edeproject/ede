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


#ifndef _edelib_Config_h_
#define _edelib_Config_h_

/*#include "Enumerations.h"
#include "Fl_Util.h"
#include "Fl_String.h"
#include "Fl_Color.h"
#include "Fl_Ptr_List.h"
#include "Fl_Map.h"*/

#include <fltk/Color.h>
#include <deque>	// TODO: port everything to char**
#include <vector>	// TODO: port everything to char**

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32_WCE
#include <fltk/x.h>
#endif

using namespace fltk;


namespace edelib {


/**
 * \defgroup edelib::Config edelib::Config globals
 */
/*@{*/

/**
 * Error codes for edelib::Config
 */
enum ConfErrors {
    CONF_SUCCESS = 0,   ///< successful operation
    CONF_ERR_FILE,      ///< trouble accessing config file or directory
    CONF_ERR_SECTION,   ///< requested section was not found
    CONF_ERR_KEY,       ///< requested key was not found
    CONF_ERR_MEMORY,    ///< memory allocation error
    CONF_ERR_NOVALUE,   ///< key found, but invalid value associated with it
};

/** List used for sections in Fl_Config_Section */
//FIXME
//typedef Fl_Ptr_List Fl_Config_Sections;
typedef std::deque<void*> Config_Sections;

/** Map used for entries in Fl_Config_Section */
//FIXME
//typedef Fl_String_String_Map Fl_Config_Lines;
//this is not exactly compatible, but that's the best we can do...
typedef std::vector<char*> Config_Lines;

/*@}*/

class Config;

/**
 * The configuration section.
 * Represents one section in config (ini) file.
 * @see edelib::Config
 */
class Config_Section
{
    friend class Config;
public:
    Config_Section(const char* name, const char* path, Config_Section *par);
    virtual ~Config_Section();

    /**
     * Destroys all sections and entries.
     */
    virtual void clear();

    /**
     * Returns pointer to parent section, NULL for Fl_Config (root)
     */
    Config_Section *parent() const { return m_parent; }

    /**
     * Returns name of section, without path.
     * @see path()
     */
    const char* name() const { return m_name; }

    /**
     * Returns path to section, without name.
     * @see name()
     */
    const char* path() const { return m_path; }

    /**
     * Returns const reference to entry map.
     */
    const Config_Lines &lines() const { return m_lines; }

    /**
     * Returns reference to entry map.
     */
    Config_Lines &lines() { return m_lines; }

    /**
     * Returns const reference to section list.
     */
    const Config_Sections &sections() const { return m_sections; }

    /**
     * Returns reference to section list.
     */
    Config_Sections &sections() { return m_sections; }

    /**
     * Find section named 'name'.
     * @param section_name name of section to find
     * @param recursive set true to perform recursive search.
     */
    Config_Section *find(const char *section_name, bool recursive=false) const;

protected:
    Config_Section *m_parent;
    char *m_name, *m_path;

    Config_Lines m_lines; //Line map
    Config_Sections m_sections; //Section list

    void write_section(int indent, FILE *fp) const;

    void add_entry(const char* key, const char* value);
    bool remove_entry(const char* key);
    char* find_entry(const char *key) const;
};

/**
 * The configuration holder. This class maybe used very easily to
 * store application settings to file. Either system wide or user specific,
 * depending on config type. edelib::Config is derived edelib::Config_Section, please
 * take look a look at functions it provides also.
 * @see edelib::Config_Section
 */
class Config : public Config_Section {
public:

    /**
     * Config file modes
     */
    enum ConfigType {
        USER=1, ///< User specific config file
        SYSTEM  ///< System wide config file
    };

    /**
     * Creates/reads/writes app specific config file.
     *
     * LINUX:<br>
     * File is created in ($home)/.ede/apps/($application)/($application).conf
     * Or ($prefix)/share/ede/apps/($application)/($application).conf
     *
     * <br>WIN32:<br>
     * ($home)\Local Settings\.ede\apps\($application)/($application).conf
     * Or ($common files)\($application)\($application).conf
     *
     * Location depends on ConfigType 'mode', USER or SYSTEM
     *
     * @param vendor aplication vendor, written down to file
     * @param application name, written down to file
     * @param mode which mode to use
     */
    Config(const char *vendor, const char *application, int mode=USER);

    /**
     * Access custom file in filesystem.
     *
     * @param filename path to config (ini) file.
     * @param readfile if true, file is readed on constructor. I.e no need for read_file()
     * @param createfile if true, file is created if it doesn't exists.
     */
    Config(const char *filename, bool readfile=true, bool createfile=true);

    /**
     * Destroys config
     */
    virtual ~Config();

    /**
     * Finds config file, depending on mode.
     * NOTE: User MUST NOT free returned pointer!
     *
     * LINUX:<br>
     * File is created in ($home)/.ede/apps/($application)/($application).conf
     * Or ($prefix)/share/ede/apps/($application)/($application).conf
     *
     * <br>WIN32:<br>
     * ($home)\Local Settings\.ede\apps\($application)/($application).conf
     * Or ($common files)\($application)\($application).conf
     *
     * @param filename Relative filename, e.g. "myapp_config.ini"
     * @param create if true, path is returned even if file is not found. Otherwise NULL if path not found.
     * @param mode which mode to use
     */
    static char *find_file(const char *filename, bool create=true, int mode=USER);


    /**
     * (re)read file. NOTE: Deletes current entries from this Fl_Config object.
     * @param create if true, file is created if it doesn't exists.
     * @see filename()
     */
    bool read_file(bool create = true);

    /**
     * Flush entries to file.
     * Returns true on success.
     * @see filename()
     */
    bool flush();

    /** Returns current filename. */
    const char* filename() const { return m_filename; }
    /** Set new filename. You need to call read_file() to get new entries. */
    void filename(const char *filename) { strncpy(m_filename, filename, strlen(filename)); }
    /** Set new filename. You need to call read_file() to get new entries. */
//    void filename(const Fl_String &filename) { m_filename = filename; }

    /** Returns current vendor name. */
    const char* vendor() const { return m_vendor; }
    /** Set new vendor name. */
    void vendor(const char *vendor) { strncpy(m_vendor, vendor, strlen(vendor)); }
    /** Set new vendor name. */
//    void vendor(const Fl_String &vendor) { m_vendor = vendor; }

    /** Returns current application name. */
    const char* application() const { return m_app; }
    /** Set new application name. */
    void application(const char *app) { strncpy(m_app, app, strlen(app)); }
    /** Set new application name. */
//    void application(const Fl_String &app) { m_app = app; }

    /**
     * Returns true, if data changed.
     * call flush() to sync changes to file
     * @see flush()
     */
    bool is_changed() const { return m_changed; }

    /**
     * Set changed, forces flush() to write file.
     * Even if it is not changed.
     */
    void set_changed() { m_changed = true; }

    /**
     * Returns last error happened.
     */
    int error() const { return m_error; }

    /**
     * Reset error, normally you don't need to call this.
     */
    void reset_error() { m_error = 0; }

    /**
     * Return string presentation for last happened error.
     */
    const char *strerror() const { return Config::strerror(m_error); }

    /**
     * Return error string, associated with 'errnum'
     */
    static const char *strerror(int errnum);

    /**
     * Create new section. You can pass full path as section name.
     * For example: create_section("/path/to/my/section");
     * All nested sections are created automatically.
     *
     * Returns pointer to created section, NULL if failed.
     */
//    Config_Section *create_section(const char *path) { char* tmp(path); return create_section(tmp); }

    /**
     * Create new section. You can pass full path as section name.
     * For example: create_section("/path/to/my/section");
     * All nested sections are created automatically.
     *
     * Returns pointer to created section, NULL if failed.
     */
    Config_Section *create_section(const char* path);

    /**
     * Find section. You can pass full path as section name.
     * For example: find_section("/path/to/my/section");
     *
     * Returns pointer to found section, NULL if not found.
     *
     * @param perfect_match is true, it returns NULL if no exact section found. Otherwise it returns last found section in path.
     */
    Config_Section *find_section(const char *path, bool perfect_match=true) const;

    /**
     * Return child sections of section specified 'secpath'
     */
    Config_Sections *section_list(const char *secpath) const { Config_Section *s=find_section(secpath); return s ? (&s->sections()) : 0; }

    /**
     * Return entries of section specified 'secpath'
     */
    Config_Lines *line_list(const char *secpath) const { Config_Section *s=find_section(secpath); return s ? (&s->lines()) : 0; }

    /**
     * Set default section for read/write operations.
     * NOTE: section is created, if it's not found.<BR>
     * NOTE: You can pass path to section e.g "/path/to/my/section"
     */
    void set_section(const char *secpath) { set_section(create_section(secpath)); }

    /**
     * Set default section for read/write operations.
     */
    void set_section(Config_Section *sec) { m_cur_sec = sec; }

    /**
     * Remove entry associated with 'key' from section.
     * NOTE: You can pass path to section e.g "/path/to/my/section"
     */
    void remove_key(const char *section, const char *key);

    /**
     * Remove section specified by 'section'.
     * NOTE: You can pass path to section e.g "/path/to/my/section"
     */
    void remove_sec(const char *section);


    /**
     * Read Fl_String entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
//    int read(const char *key, char* ret, const char *def_value) { return _read_string(m_cur_sec, key, ret, def_value); }

    /**
     * Read char* entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     * @param size of 'ret' char* array.
     */
    int read(const char *key, char *ret, const char *def_value, int size) { return _read_string(m_cur_sec, key, ret, def_value, size); }

    /**
     * Read char* entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: 'ret' is allocated by Fl_Confing, user MUST free 'ret' by calling free() function.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, char *&ret, const char *def_value=0) { return _read_string(m_cur_sec, key, ret, def_value); }

    /**
     * Read long entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, long &ret, long def_value=0)         { return _read_long(m_cur_sec, key, ret, def_value);   }

    /**
     * Read int entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, int &ret, int def_value=0)           { return _read_int(m_cur_sec, key, ret, def_value);    }

    /**
     * Read float entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, float &ret, float def_value=0)       { return _read_float(m_cur_sec, key, ret, def_value);  }

    /**
     * Read double entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, double &ret, double def_value=0)     { return _read_double(m_cur_sec, key, ret, def_value); }

    /**
     * Read bool entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, bool &ret, bool def_value=0)         { return _read_bool(m_cur_sec, key, ret, def_value);   }

    /**
     * Read Fl_Color entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int read(const char *key, Color &ret, Color def_value=0) { return _read_color(m_cur_sec, key, ret, def_value);  }


    /**
     * Write Fl_String entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
//    int write(const char *key, const Fl_String &value) { return _write_string(m_cur_sec, key, value); }

    /**
     * Write const char* entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const char *value)    { return _write_string(m_cur_sec, key, value); }

    /**
     * Write long entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const long value)     { return _write_long(m_cur_sec, key, value); }

    /**
     * Write int entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const int value)      { return _write_int(m_cur_sec, key, value); }

    /**
     * Write float entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const float value)    { return _write_float(m_cur_sec, key, value); }

    /**
     * Write double entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const double value)   { return _write_double(m_cur_sec, key, value); }

    /**
     * Write bool entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const bool value)     { return _write_bool(m_cur_sec, key, value); }

    /**
     * Write Fl_Color entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: This function assumes that current section is set with set_section().
     *
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int write(const char *key, const Color value) { return _write_color(m_cur_sec, key, value); }


    /**
     * Read Fl_String entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
//    int get(const char *section, const char *key, Fl_String &ret, const char *def_value) { return _read_string(find_section(section), key, ret, def_value); }

    /**
     * Read char* entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, char *ret, const char *def_value, int size) { return _read_string(find_section(section), key, ret, def_value, size); }

    /**
     * Read char* entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: 'ret' is allocated by Fl_Confing, user MUST free 'ret' by calling free() function.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, char *&ret, const char *def_value=0) { return _read_string(find_section(section), key, ret, def_value); }

    /**
     * Read long entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, long &ret, long def_value=0)         { return _read_long(find_section(section), key, ret, def_value);            }

    /**
     * Read int entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, int &ret, int def_value=0)           { return _read_int(find_section(section), key, ret, def_value);               }

    /**
     * Read float entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, float &ret, float def_value=0)       { return _read_float(find_section(section), key, ret, def_value);         }

    /**
     * Read double entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, double &ret, double def_value=0)     { return _read_double(find_section(section), key, ret, def_value);      }

    /**
     * Read bool entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, bool &ret, bool def_value=0)         { return _read_bool(find_section(section), key, ret, def_value);            }

    /**
     * Read Fl_Color entry from config.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param ret Result is stored to this.
     * @param def_value Default value for ret, if not found.
     */
    int get(const char *section, const char *key, Color &ret, Color def_value=0) { return _read_color(find_section(section), key, ret, def_value);   }


    /**
     * Write Fl_String entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
//    int set(const char *section, const char *key, const Fl_String &value) { return _write_string(create_section(section), key, value); }

    /**
     * Write const char *entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const char *value) { return _write_string(create_section(section), key, value); }

    /**
     * Write long entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const long value)  { return _write_long(create_section(section), key, value);   }

    /**
     * Write int entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const int value)   { return _write_int(create_section(section), key, value);    }

    /**
     * Write float entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const float value) { return _write_float(create_section(section), key, value);  }

    /**
     * Write bool entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const bool value)  { return _write_double(create_section(section), key, value); }

    /**
     * Write Fl_Color entry to config. You must call flush() to sunc changes to file.
     * Returns CONF_SUCCESS on success, otherwise errorcode.
     * NOTE: section must be set as first parameter!
     *
     * @param section Section for entry
     * @param key Key to entry.
     * @param value value to set. if entry with 'key' exists, value is replaced.
     */
    int set(const char *section, const char *key, const Color value) { return _write_color(create_section(section), key, value); }

private:
    int m_error;
    char* m_filename;
    char *m_vendor, *m_app;

    Config_Section *m_cur_sec;
    bool m_changed;

    //int _read_string(Fl_Config_Section *s, const char *key, Fl_String &ret, const char *def_value);
    int _read_string(Config_Section *s, const char *key, char *ret, const char *def_value, int size);
    int _read_string(Config_Section *s, const char *key, char *&ret, const char *def_value);
    int _read_long  (Config_Section *s, const char *key, long &ret, long def_value);
    int _read_int   (Config_Section *s, const char *key, int &ret, int def_value);
    int _read_float (Config_Section *s, const char *key, float &ret, float def_value);
    int _read_double(Config_Section *s, const char *key, double &ret, double def_value);
    int _read_bool  (Config_Section *s, const char *key, bool &ret, bool def_value);
    int _read_color (Config_Section *s, const char *key, Color &ret, Color def_value);

    //int _write_string(Fl_Config_Section *s, const char *key, const Fl_String &value);
    int _write_string(Config_Section *s, const char *key, const char *value);
    int _write_long  (Config_Section *s, const char *key, const long value);
    int _write_int   (Config_Section *s, const char *key, const int value);
    int _write_float (Config_Section *s, const char *key, const float value);
    int _write_double(Config_Section *s, const char *key, const double value);
    int _write_bool  (Config_Section *s, const char *key, const bool value);
    int _write_color (Config_Section *s, const char *key, const Color value);
};

// Backward compatibility...
static inline const char* find_config_file(const char *filename, bool create=true) {
    return Config::find_file(filename, create, Config::USER);
}

}

#endif
