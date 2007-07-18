/*
 * $Id$
 *
 * Volume control application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef EVOLUME_H_
#define EVOLUME_H_

extern "C" {
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
}

/*#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Slider.h>
#include <efltk/Fl_Menu_Bar.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Button.h>
#include <efltk/Fl_Item.h>
#include <efltk/Fl_Item_Group.h>
#include <efltk/Fl_Divider.h>
#include <efltk/Fl_Check_Button.h>
#include <efltk/x.h>
#include <efltk/fl_ask.h>
#include <efltk/Fl_Config.h>
#include <efltk/Fl_Locale.h>
#include <efltk/Fl_Util.h>
#include <efltk/Fl_Divider.h>*/

#include <fltk/Window.h>
#include <fltk/Slider.h>
#include <fltk/MenuBar.h>
#include <fltk/Box.h>
#include <fltk/Button.h>
#include <fltk/Item.h>
#include <fltk/ItemGroup.h>
#include <fltk/Divider.h>
#include <fltk/CheckButton.h>
#include <fltk/x.h>
#include <fltk/ask.h>
#include <fltk/Divider.h>

#include "../edelib2/Config.h"
#include "../edelib2/NLS.h"

typedef struct 
volume
{
    unsigned char left;
    unsigned char right;

} volume;

int mixer_device;
  
fltk::Slider *volume_slider, *cd_slider, *pcm_slider, *synth_slider,
 *line_slider, *bass_slider, *treble_slider, *mic_slider,
 *speaker_slider, *imix_slider, *igain_slider, *ogain_slider;
   
fltk::Slider *volume_balance, *cd_balance, *pcm_balance, *synth_balance,
 *line_balance, *bass_balance, *treble_balance, *mic_balance,
 *speaker_balance, *imix_balance, *igain_balance, *ogain_balance;
   
fltk::CheckButton *volume_mute, *cd_mute, *pcm_mute, *synth_mute,
 *line_mute, *bass_mute, *treble_mute, *mic_mute,
 *speaker_mute, *imix_mute, *igain_mute, *ogain_mute;

fltk::CheckButton *volume_rec, *cd_rec, *pcm_rec, *synth_rec,
 *line_rec, *bass_rec, *treble_rec, *mic_rec,
 *speaker_rec, *imix_rec, *igain_rec, *ogain_rec;

void get_device_info(int mixer_dev, fltk::Slider *sl, fltk::Slider *bal, fltk::CheckButton *ck, int device);
void set_device(int mixer_fd, int device, fltk::Slider *device_sl, fltk::Slider *balance);
void set_mute(int mixer_fd, int device, fltk::Slider *device_sl, fltk::Slider *balance, fltk::CheckButton *check_button);
void set_rec(int mixer_fd, int device, fltk::CheckButton *ck);
void update_info();

#endif

