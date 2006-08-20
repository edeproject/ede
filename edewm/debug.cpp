/*
 * $Id: debug.cpp 1671 2006-07-11 14:07:43Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

#define OUT_TYPE stdout

void edewm_log(const char* str, ...)
{
#ifdef _DEBUG
	fprintf(OUT_TYPE, "wm: ");
	va_list args;
	va_start(args, str);
	vfprintf(OUT_TYPE, str, args);
	fprintf(OUT_TYPE, "\n");
	fflush(OUT_TYPE);
#endif
}
void edewm_warning(const char* str, ...)
{
#ifdef _DEBUG
	fprintf(OUT_TYPE, "wm (warning): ");
	va_list args;
	va_start(args, str);
	vfprintf(OUT_TYPE, str, args);
	fprintf(OUT_TYPE, "\n");
	fflush(OUT_TYPE);
#endif
}

void edewm_fatal(const char* str, ...)
{
#ifdef _DEBUG
	fprintf(OUT_TYPE, "!!! wm (fatal): ");
	va_list args;
	va_start(args, str);
	vfprintf(OUT_TYPE, str, args);
	fprintf(OUT_TYPE, "\n");
	fflush(OUT_TYPE);
#endif
}

void edewm_printf(const char* str, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, str);
	vfprintf(OUT_TYPE, str, args);
	fflush(OUT_TYPE);
#endif
}
