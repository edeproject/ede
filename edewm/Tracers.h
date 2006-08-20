/*
 * $Id: Tracers.h 1671 2006-07-11 14:07:43Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __TRACERS_H__
#define __TRACERS_H__

#include "debug.h"

#ifdef _DEBUG
	class FunctionTracer
	{
		private:
			const char* func;
		public:
			FunctionTracer(const char* f) : func(f)
			{
				if(func)
					EPRINTF("Function [+]: \"%s\"\n", func);
			}

			~FunctionTracer()
			{
				if(func)
					EPRINTF("Function [-]: \"%s\"\n", func);
			}
	};

	#define TRACE_FUNCTION(name) FunctionTracer foo(name)
#else
	#define TRACE_FUNCTION(name) (void)0
#endif // _DEBUG

#endif
