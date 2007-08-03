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

// This is a replacement for fl_ask that uses system icons.
// TODO: also add support for EDE sound events instead of the ugly beep
// ede_choice_alert() is just ede_choice() that uses alert icon.



void ede_alert(const char*fmt, ...);
void ede_message(const char*fmt, ...);
int ede_ask(const char*fmt, ...);
int ede_choice(const char*fmt,const char *b0,const char *b1,const char *b2,...);
int ede_choice_alert(const char*fmt,const char *b0,const char *b1,const char *b2,...);
const char* ede_password(const char*fmt,const char *defstr,...);
const char* ede_input(const char*fmt,const char *defstr,...);
