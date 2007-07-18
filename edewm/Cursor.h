/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __CURSOR_H__
#define __CURSOR_H__

#include <X11/Xlib.h> // Cursor

/* Main existance of this class is
 * to allow using efltk and X cursors
 * (...and maybe bitmapped ones :).
 */

enum CursorType
{
	CURSOR_DEFAULT = 0,
	CURSOR_MOVE,
	CURSOR_WAIT,
	CURSOR_HELP,
	CURSOR_N,
	CURSOR_NE,
	CURSOR_E,
	CURSOR_SE,
	CURSOR_S,
	CURSOR_SW,
	CURSOR_W,
	CURSOR_NW,
	CURSOR_NONE
};

#define CURSOR_LIST_SIZE 13

enum CursorShapeType
{
	FLTK_CURSORS = 0,
	X_CURSORS
};

class Frame;

class CursorHandler
{
	private:
		int cursors[CURSOR_LIST_SIZE];
		CursorShapeType st;
		CursorType curr_cursor_type;
		Cursor root_window_cursor;
		bool locked;
		bool cursors_loaded;
	public:
		CursorHandler();
		~CursorHandler();
		void load(CursorShapeType s);
		void set_cursor(Frame* f, CursorType t);
		void set_root_cursor(void);
		void set_root_cursor(CursorType t);
		Cursor current_cursor(void) const;
		Cursor root_cursor(void)                { return root_window_cursor; }
		CursorShapeType cursor_shape_type(void) { return st; }
};
#endif
