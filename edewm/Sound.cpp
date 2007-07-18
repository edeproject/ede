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

#include "Sound.h"
#include "debug.h"

#ifdef SOUND
	#include <vorbis/vorbisfile.h>
	#include <vorbis/codec.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	// free
#include <string.h>	// strdup

SoundSystem::SoundSystem()
{
#ifdef SOUND
	device = NULL;
	inited = false;
	down = false;
#endif
}

SoundSystem::~SoundSystem()
{
#ifdef SOUND
	if(!down)
		shutdown();		// just in case
#endif
}

void SoundSystem::init(void)
{
#ifdef SOUND
	EPRINTF("Loading sound system\n");

	ao_initialize();
	default_driver = ao_default_driver_id();

	for(int i = 0; i < KNOWN_SOUNDS; i++)
	{
		event_sound[i].allocated = false;
		event_sound[i].loaded = false;
		event_sound[i].file_to_play = NULL;
	}

	inited = true;
#endif
}

void SoundSystem::shutdown(void)
{
#ifdef SOUND
	EPRINTF("Shutting down sound system\n");

	ao_shutdown();

	for(int i = 0; i < KNOWN_SOUNDS; i++)
	{
		if(event_sound[i].allocated)
			free(event_sound[i].file_to_play);
	}

	down = true;
#endif
}

void SoundSystem::add(short event, const char* file)
{
#ifdef SOUND
	assert(event < KNOWN_SOUNDS);
	if(event_sound[event].allocated)
		free(event_sound[event].file_to_play);

	event_sound[event].file_to_play = strdup(file);
	event_sound[event].allocated = true;
	event_sound[event].loaded = true;
#endif
}

int SoundSystem::play(short event)
{
#ifdef SOUND
	assert(event < KNOWN_SOUNDS);
	if(event_sound[event].loaded)
		play(event_sound[event].file_to_play);
	else
		ELOG("Skipping this sound, no file for it");
#endif
	return 1;
}

int SoundSystem::play(const char* fname)
{
#ifdef SOUND
	assert(inited != false);
	assert(fname != NULL);

	FILE* f = fopen(fname, "rb");
	if(f == NULL)
	{
		ELOG("Can't open %s\n", fname);
		return 0;
	}

	OggVorbis_File vf;
	vorbis_info* vi;

	if(ov_open(f, &vf, NULL, 0) < 0)
	{
		ELOG("%s does not appear to be ogg file\n");
		fclose(f);
		return 0;
	}

	// read and print comments
	char** comm = ov_comment(&vf, -1)->user_comments;
	vi = ov_info(&vf, -1);

	while(*comm)
	{
		ELOG("%s", *comm);
		comm++;
	}

	assert(vi != NULL);

	format.bits = 4 * 8;	// TODO: should be word_size * 8
	format.channels = vi->channels;
	format.rate = vi->rate;
	format.byte_format = AO_FMT_NATIVE;

	// now open device
	device = ao_open_live(default_driver, &format, NULL);
	if(device == NULL)
	{
		ELOG("Can't open device");
		ov_clear(&vf);
		return 0;
	}

	int current_section;
	while(1)
	{
		long ret = ov_read(&vf, pcm_out, sizeof(pcm_out), 0, 2, 1, &current_section);
		if(ret == 0)
			break;
		else if(ret < 0)
			ELOG("Error in the stream, continuing...");
		else
			ao_play(device, pcm_out, ret);
	}

	ao_close(device);
	device = NULL;

	// NOTE: fclose(f) is not needed, since ov_clear() will close file
	ov_clear(&vf);
#endif	// SOUND

	return 1;
}
