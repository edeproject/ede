/*
 * $Id$
 *
 * EFiler - EDE File Manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */


// References:
// [1] Mutt documentation, chapter on mailcap
// http://www.mutt.org/doc/manual/manual-5.html
// [2] RFC 1524
// http://www.faqs.org/rfcs/rfc1524.html


#include "mailcap.h"

#include <edelib/String.h>
#include <edelib/List.h>
#include <edelib/Util.h>
#include <edelib/Directory.h>
#include <edelib/File.h>
#include <edelib/StrUtil.h>

#include <stdio.h>

// See: [2] Appendix A
#define MAILCAPS "/etc/mailcap:/usr/etc/mailcap:/usr/local/etc/mailcap"

// FIXME: use favourite aplications to find xterm
#define TERM "xterm"

// Maximum length of a mailcap spec
// We could've used edelib::String, but that would make the whole code a bit 
// more complex and a bit slower. There's no reason for mailcap to be longer
// (very long mailcaps are usually application specific codes /e.g. mutt/ which
// isn't useful to us)
#define BUFLEN 1000


using namespace edelib; // this drastically shortened the code...


struct mime_db_struct {
	String type;
	String viewcmd;
	String createcmd;
	String editcmd;
	String printcmd;
	bool copiousoutput;
	bool needsterminal;
};


// database of parsed mimetypes
list<mime_db_struct> mime_db;

// flag to know if we should call read_files()
bool files_read=false;


// Expand various macros in mailcap command spec and prevent
// certain security issues

String mailcap_sanitize(char* cmd, String type) {
	String s(cmd);

	// no %s, file should be piped to stdin
	unsigned int k = s.find("%s");
	if (k==String::npos) { 
		s = String("cat '%s' | ") + cmd;
	} else {
		// replace %s with '%s' (see: [1])
		while (k!=String::npos) {
			s = s.substr(0,k)+ "'%s'" + s.substr(k+2);
			k = s.find("%s",k+2);
		}
	}

	// replace %t with mimetype
	k = s.find("%t");
	while (k!=String::npos) {
		s = s.substr(0,k) + type + s.substr(k+2);
		k = s.find("%t",k+1);
	}
	// we don't know the charset
	k = s.find("%{charset}");
	while (k!=String::npos) {
		s = s.substr(0,k) + "us-ascii" + s.substr(k+10);
		k = s.find("%{charset}",k+1);
	}
	// ignore other mutt specific codes
	k = s.find("%{");
	while (k!=String::npos) {
		s = s.substr(0,k) + s.substr(k+2);
		k = s.find("}",k); // there has to be a closing brace...
		if (k!=String::npos) s= s.substr(0,k) + s.substr(k+1);
		k = s.find("%{",k+1);
	}
	return s;
}


// Read contents of mailcap file

void read_files() {
	list<String> locations;

	// RFC compliant list of mailcap locations
	stringtok(locations, String(MAILCAPS), ":");
	// Add ~/.mailcap
	locations.push_front(dir_home() + E_DIR_SEPARATOR_STR".mailcap");
	// Add EDE specific configuration
	if (user_config_dir() != "") 
		locations.push_front(user_config_dir() + E_DIR_SEPARATOR_STR"ede"E_DIR_SEPARATOR_STR"mailcap");

	// Test all these locations
	const char* mailcap = NULL;
	{
		list<String>::iterator it = locations.begin(), it_end = locations.end();
	
		for(; it != it_end; ++it) {
			if (file_exists((*it).c_str())) {
				mailcap=(*it).c_str();
				break;
			}
		}
	}
	fprintf (stderr, " ------ mailcap: %s\n",mailcap);

	// Read data into list
	FILE *fp = fopen(mailcap,"r");
	char buffer[BUFLEN];
	while (fgets(buffer,BUFLEN,fp)) {
		str_trim(buffer);
		if (buffer[0]=='#') continue; //comment
		if (strlen(buffer)<2) continue; //empty line
		
		// Multiline
		if (buffer[strlen(buffer)-1]=='\\') {
			String line(buffer); //Use string for dynamic growing
			while (fgets(buffer,BUFLEN,fp)) {
				str_trim(buffer);
				line = line.substr(0, line.length()-2);
				line += buffer;
				if (buffer[strlen(buffer)-1]!='\\') break;
			}
			// Use just the first BUFLEN chars
			strncpy(buffer, line.c_str(), BUFLEN);
			buffer[BUFLEN-1]='\0';
		}

		// Parse line
		mime_db_struct parsed;
		char *tmp = strtok(buffer, ";");
		str_trim(tmp);
		parsed.type=tmp;

		// Command for viewing file
		tmp = strtok(NULL, ";");
		if (tmp==NULL) continue; // no view cmd in line!?
		str_trim(tmp);
		parsed.viewcmd = mailcap_sanitize(tmp, parsed.type);

		// Other mailcap parameters...
		parsed.copiousoutput = parsed.needsterminal = false;
		while ((tmp = strtok(NULL, ";")) != NULL) {
			str_trim(tmp);
			if (strcmp(tmp,"copiousoutput")==0)
				parsed.copiousoutput = true;
			else if (strcmp(tmp,"needsterminal")==0)
				parsed.needsterminal = true;
			else if (strncmp(tmp,"compose=",8)==0)
				parsed.createcmd = mailcap_sanitize(tmp+8, parsed.type);
			else if (strncmp(tmp,"print=",6)==0)
				parsed.printcmd = mailcap_sanitize(tmp+6, parsed.type);
			else if (strncmp(tmp,"edit=",5)==0)
				parsed.editcmd = mailcap_sanitize(tmp+5, parsed.type);
			// We ignore other parameters
		}
		mime_db.push_back(parsed);
	}
	fclose(fp);

	files_read=true;
}


// Helper function for searching mime_db

list<mime_db_struct>::iterator find_type(const char* type) {
	list<mime_db_struct>::iterator it = mime_db.begin(), it_end = mime_db.end();
	for(; it != it_end; ++it)
		if ((*it).type==type) return it;

	it = mime_db.begin();
	for(; it != it_end; ++it) {
		String s((*it).type);
		if (s[s.length()-1]=='*' && strncmp(type, s.c_str(), s.length()-1)==0) 
			return it;
	}
	return 0;
}


// Command for performing given action on files of given type

const char* mailcap_opener(const char* type, MailcapAction action) { 
	static char buffer[BUFLEN];
	if (!files_read) read_files(); 
	list<mime_db_struct>::iterator it = find_type(type);
	if (it==0) return 0;

	if (action==MAILCAP_VIEW) {
		const char *c = (*it).viewcmd.c_str();
		if ((*it).copiousoutput) {
			// copiousoutput and needsterminal are mutually exclusive [2]
			// TODO: use enotepad? - when it supports stdin
			snprintf(buffer, BUFLEN-1, TERM" -e \"%s\" | less", c);
		} else if ((*it).needsterminal) {
			snprintf(buffer, BUFLEN-1, TERM" -e \"%s\"", c);
		} else return c;
		return buffer;
	}

	else if (action==MAILCAP_CREATE) return (*it).createcmd.c_str();
	else if (action==MAILCAP_EDIT) return (*it).editcmd.c_str();
	else if (action==MAILCAP_PRINT) return (*it).printcmd.c_str();
	else return 0;
}


// List of actions available for given type

int mailcap_actions(const char* type) {
	if (!files_read) read_files(); 
	list<mime_db_struct>::iterator it = find_type(type);
	if (it==0) return 0;
	int result=0;
	if ((*it).viewcmd != "") result += MAILCAP_VIEW;
	if ((*it).createcmd != "") result += MAILCAP_CREATE;
	if ((*it).editcmd != "") result += MAILCAP_EDIT;
	if ((*it).printcmd != "") result += MAILCAP_PRINT;
	return result;
}


// Add a new opener for given type
// NOTE: This will be written in EDE-specific location. If EDE-specific
// mailcap file doesn't exist, it will be created and existing mimetypes
// will be added. Thus, users of EDE will get a new type in addition to
// old ones, however other applications will not.

void mailcap_add_type(const char* type, const char* opener) {
	// Find old type, edit if it exists
	list<mime_db_struct>::iterator it = find_type(type);
	if (it != 0) {
		(*it).viewcmd = opener;
		(*it).viewcmd += " %s";
	} else {
		mime_db_struct newtype;
		newtype.type = type;
		newtype.viewcmd = opener;
		newtype.viewcmd += " %s";
		newtype.createcmd = "";
		newtype.editcmd = "";
		newtype.printcmd = "";
		newtype.copiousoutput = false;
		newtype.needsterminal = false;
		mime_db.push_back(newtype);
	}

	// Write mime_db contents to file
	String ede_mailcap = user_config_dir() + E_DIR_SEPARATOR_STR"ede"E_DIR_SEPARATOR_STR"mailcap";
	FILE *fp = fopen(ede_mailcap.c_str(),"w");

	it = mime_db.begin();
	list<mime_db_struct>::iterator it_end = mime_db.end();
	while (it != it_end) {
		char buffer[BUFLEN];
		snprintf(buffer, BUFLEN-1, "%s; %s", (*it).type.c_str(), (*it).viewcmd.c_str());
		if ((*it).createcmd != "") {
			strncat(buffer, "; compose=", BUFLEN-1-strlen(buffer));
			strncat(buffer, (*it).createcmd.c_str(), BUFLEN-1-strlen(buffer));
		}
		if ((*it).editcmd != "") {
			strncat(buffer, "; edit=", BUFLEN-1-strlen(buffer));
			strncat(buffer, (*it).editcmd.c_str(), BUFLEN-1-strlen(buffer));
		}
		if ((*it).printcmd != "") {
			strncat(buffer, "; print=", BUFLEN-1-strlen(buffer));
			strncat(buffer, (*it).printcmd.c_str(), BUFLEN-1-strlen(buffer));
		}
		if (strlen(buffer)>BUFLEN-2) 
			// Even if line is too long, we don't want to mangle the next one
			buffer[strlen(buffer)-1]='\n';
		else
			strcat(buffer, "\n");
		fputs(buffer, fp);
		++it;
	}
	fclose(fp);

}
