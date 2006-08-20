/*
 * $Id: Sound.h 1697 2006-07-21 15:01:05Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __SOUND_H__
#define __SOUND_H__

#ifdef SOUND
	#include <ao/ao.h>
#endif

enum
{
	SOUND_MINIMIZE = 0,
	SOUND_MAXIMIZE,
	SOUND_RESTORE,
	SOUND_SHADE,
	SOUND_CLOSE
};
#define KNOWN_SOUNDS 5

struct EventSound
{
	bool allocated;
	bool loaded;
	short event;
	char* file_to_play;
};

#define PCM_BUF_SIZE 4096

class SoundSystem
{
#ifdef SOUND
	private:
		ao_device* device;
		ao_sample_format format;
		int default_driver;
		char pcm_out[PCM_BUF_SIZE];
		bool inited;
		bool down;
		EventSound event_sound[KNOWN_SOUNDS];
#endif 
	public:
		SoundSystem();
		~SoundSystem();
		void init(void);
		void shutdown(void);

		void add(short event, const char* file);
		int play(const char* fname);
		int play(short event);
};

#endif
