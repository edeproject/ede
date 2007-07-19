/*
 * $Id$
 *
 * Library of useful functions
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Util.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>


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


// From Enumerations.h
#ifdef _WIN32
# undef slash
# define slash '\\'
#else
# undef slash
# define slash '/'
#endif
// End Enumerations.h


//using namespace fltk;
//using namespace edelib;


// Test if path is absolute or relative
int is_path_rooted(const char *fn)
{
#ifdef _WIN32
	if (fn[0] == '/' || fn[0] == '.' || fn[0] == '\\' || fn[1]==':')
#else
	if (fn[0] == '/' || fn[0] == '.')
#endif
		return 1;
	return 0;
}


// recursively create a path in the file system
bool make_path( const char *path ) 
{
	if(access(path, 0)) 
	{
		const char *s = strrchr( path, slash );
		if ( !s ) return 0;
		int len = s-path;
		char *p = (char*)malloc( len+1 );
		memcpy( p, path, len );
		p[len] = 0;
		make_path( (const char*)p );
		free( p );
		return ( mkdir( path, 0777 ) == 0 );
	}
	return true;
}


// create the path needed for file using make_path
bool make_path_for_file( const char *path )
{
	const char *s = strrchr( path, slash );
	if ( !s ) return false;
	int len = s-path;
	char *p = (char*)malloc( len+1 );
	memcpy( p, path, len );
	p[len] = 0;
	bool ret=make_path( (const char*)p );
	free( p );
	return ret;
}


// Cross-platform function for system path
char* get_sys_dir() 
{
#ifndef _WIN32
	return SYSTEM_PATH;
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



// Cross-platform function for home directory...
// I don't see the purpose since getenv("HOME") works just fine
/*char* get_homedir() {
	char *path = new char[PATH_MAX];
	const char *str1;

	str1=getenv("HOME");
	if (str1) {
		memcpy(path, str1, strlen(str1)+1);
		return path;
	}

	return 0;
}*/


// strdupcat() - it's cool to strcat with implied realloc
//  -- NOTE: due to use of realloc *always* use strdupcat return value:
//            dest = strdupcat(dest,src);
// and *never* use it like:
//            strdupcat(dest,src);
char* strdupcat(char *dest, const char *src)
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
	if(!string)
		return NULL;
	
	char *start;
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
	
	for(start = string; *start && isspace (*start); start++);
		memmove(string, start, strlen(start) + 1);
	
	return string;
}

const char* twstrim(const char* string)
{
	static char buffer[4096];
	if (strlen(string)>4095) {
		strncpy(buffer,string,4095);
		buffer[4095]='\0';
	} else
		strcpy(buffer,string);
	wstrim((char*)buffer);
	return (const char*)buffer;
}

// hmmmh?
/*
char* wstrim(const char *string)
{
	char *newstring = strdup(string);
	return wstrim(newstring);
}*/

// Returns nicely formatted string for byte sizes
const char* nice_size(double size) {
	static char buffer[256];
	if (size<1024) {
		snprintf(buffer,255,"%.0f B",size);
	} else if (size<1024*10) {
		snprintf(buffer,255,"%.2f kB",size/1024);
	} else if (size<1024*100) {
		snprintf(buffer,255,"%.1f kB",size/1024);
	} else if (size<1024*1024) {
		snprintf(buffer,255,"%.0f kB",size/1024);
	} else if (size<1024*1024*10) {
		snprintf(buffer,255,"%.2f MB",size/(1024*1024));
	} else if (size<1024*1024*10) {
		snprintf(buffer,255,"%.1f MB",size/(1024*1024));
	} else if (size<1024*1024*1024) {
		snprintf(buffer,255,"%.0f MB",size/(1024*1024));
	} else if (size<1024*1024*1024*10) {
		snprintf(buffer,255,"%.2f GB",size/(1024*1024*1024));
	} else {
		snprintf(buffer,255,"%.1f GB",size/(1024*1024*1024));
	}
	return (const char*) buffer;
}


const char* nice_time(long int epoch) {
	static char buffer[256];

	const time_t k = (const time_t)epoch;
	const struct tm *timeptr = localtime(&k);
	// Date/time format should be moved to configuration
	snprintf(buffer,255,"%.2d.%.2d.%.4d. %.2d:%.2d", timeptr->tm_mday, timeptr->tm_mon+1, 1900+timeptr->tm_year, timeptr->tm_hour, timeptr->tm_min);
	
	return buffer;
}



// Find in haystack any of needles (divided with separator)
char* strstrmulti(const char *haystack, const char *needles, const char *separator) {
	if (!haystack || !needles || (strlen(haystack)==0) || (strlen(needles)==0)) 
		return (char*)haystack; // this means that empty search returns true
	char *copy = strdup(needles);
	char *token = strtok(copy, separator);
	char *result = 0;
	do {
		if ((result = strstr(haystack,token))) break;
	} while ((token = strtok(NULL, separator)));
	free (copy);
	if (!result && (strcmp(separator,needles+strlen(needles)-strlen(separator))==0))
		return (char*)haystack; // again
	return result;
}



// vec_from_string() - similar to explode() in PHP or split() in Perl
// adapted from Fl_String_List to use vector
/*std::vector<char*> vec_from_string(const char *str, const char *separator)
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
}*/




// Print to a static char[] and return pointer
const char* tsprintf(char *format, ...)
{
	static char buffer[4096];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 4095, format, args);
	va_end(args);
	return (const char*)buffer;
}

char* tasprintf(char *format, ...)
{
	char buffer[4096];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 4095, format, args);
	va_end(args);
	return strdup(buffer);
}


// This function exists on some OSes and is mentioned in C textbooks
// However, we can just use sprintf instead

/*
char *
itoa(int value, char *string, int radix)
{
	char tmp[33];
	char *tp = tmp;
	int i;
	unsigned v;
	int sign;
	char *sp;
	
	if (radix > 36 || radix <= 1)
	{
		return 0;
	}
	
	sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;
	while (v || tp == tmp)
	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'a' - 10;
	}
	
	if (string == 0)
		string = (char *)malloc((tp-tmp)+sign+1);
	sp = string;
	
	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
	return string;
}*/
