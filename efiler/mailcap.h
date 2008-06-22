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


#ifndef mailcap_h
#define mailcap_h

enum MailcapAction {
	MAILCAP_VIEW   = 1,
	MAILCAP_CREATE = 2,
	MAILCAP_EDIT   = 4,
	MAILCAP_PRINT  = 8,
	MAILCAP_EXEC   = 16 // execute
};


// Returns a shell command to be used for opening files of given type.
// The command returned will *always* contain %s which should be replaced 
// with filename. Parameter MAILCAP_TYPE indicates the type of action to 
// be performed

const char* mailcap_opener(const char* type, MailcapAction action=MAILCAP_VIEW);


// Returns a list of actions available for a given type

int mailcap_actions(const char* type);


// Add new type to the list of actions

void mailcap_add_type(const char* type, const char* opener);


#endif
