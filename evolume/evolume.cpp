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


// TODO:
// At the moment evolume is ALSA only - patches for OSS support
// are welcome

#include "prefs.h"
#include "evolume.h"
#include "../edeconf.h"

#include "../edelib2/about_dialog.h"

#include <fltk/events.h>
#include <fltk/run.h>


using namespace fltk;
using namespace edelib;




// Global variables

char 	  	device[1024]={0};
Window 	*main_window=0;

Config globalConfig("EDE Team", "evolume");
bool simplemode = true;


// Main ALSA device functions

void set_device(int mixer_fd, int device, Slider *device_sl, Slider *balance)
{
    int l = (unsigned int) ((1.0-(balance->value()) ) * device_sl->value() );	
    int r = (unsigned int) ( (balance->value()) * device_sl->value());
    int v = (r << 8) | l;
    if (ioctl (mixer_fd, MIXER_WRITE (device), &v) < 0)    
	alert(_("Cannot setup device, sorry."));
}

void get_device_info(int mixer_dev, Slider *sl, Slider *bal, 
		     CheckButton *ck, int device)
{
    unsigned int devmask, recmask, recsrc, stereo;
    volume real_volume;
    
    real_volume.left = real_volume.right = 0;
    devmask = recmask = recsrc = stereo = 0;
    
    if (ioctl(mixer_dev, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) 
	fprintf(stderr, "Read devmask failed.\n");
    if (devmask & (1 << (device))) 
	sl->activate();
    else 
	sl->deactivate();

    if (ioctl(mixer_dev, SOUND_MIXER_READ_STEREODEVS, &stereo) == -1) 
	fprintf(stderr, "Read recsrc failed.\n"); 
    if ( stereo & (1 << (device) ) ) bal->activate();		    
    else ck->deactivate();		    
		    
    if (ioctl(mixer_dev, SOUND_MIXER_READ_RECMASK, &recmask) == -1) 
	fprintf(stderr, "Read recmask failed.\n");
    if ( recmask & (1 << (device) ) ) ck->activate();		    
    else ck->deactivate();
    
    if (ioctl(mixer_dev, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) 
	fprintf(stderr, "Read recsrc failed.\n"); 
    if ( recsrc & (1 << (device) ) ) ck->set();		    
    else ck->clear();		    
    
    if ( ioctl(mixer_dev, MIXER_READ(device), &real_volume) < 0 ) {
        fprintf(stderr, "Can't obtain current volume settings.\n");  
    }

    float volume = real_volume.left + real_volume.right;

    float balance = 0; 
    balance = ( (1.0 * (unsigned char)real_volume.right ) / 
		(1.0 * ((unsigned char)real_volume.left + (unsigned char)real_volume.right)) );

    if (volume == 0)
	volume=1;
    if (balance < 0)
	balance=0.5;
    sl->value(volume);
    bal->value(balance);
}

void set_mute(int mixer_fd, int device, Slider *device_sl, Slider *balance, CheckButton *check_button)
{
    int vol = 0;
    
    if ( check_button->value() ) 
    {
	if (ioctl(mixer_fd, MIXER_WRITE(device), &vol) < 0 ) 
	 fprintf(stderr, "Cannot set mute.\n");  
    }
    else {
	volume real_volume;
        double old_volume = device_sl->value();
        double old_balance = balance->value();
        real_volume.left  = (unsigned char) ( (1.0 - (old_balance)) * old_volume );	
        real_volume.right = (unsigned char) ( (old_balance) * old_volume);
        if ( ioctl(mixer_fd, MIXER_WRITE(device), &real_volume) < 0 )
        {
    	    fprintf(stderr, "Cannot setup volume, sorry.\n");
	}
    }
}

void set_rec(int mixer_fd, int device, CheckButton *ck)
{
    unsigned int recsrc;
    
    if (ioctl(mixer_fd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) 
	printf("read recsrc failed\n");
    unsigned int new_recsrc = recsrc ^ ( 1 << device );
    
    if (ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &new_recsrc) == -1) 
	printf("oh no\n");				    
}

void update_info()
{
    mixer_info minfo;

    if (ioctl(mixer_device, SOUND_MIXER_INFO, &minfo) < 0)    
	fprintf(stderr, "Read device info failed.\n");
    else  
    {
	char *title = (char*)malloc(strlen(_("Volume control: [%s]"))+strlen(minfo.name));
	sprintf(title,_("Volume control: [%s]"), minfo.name);
	main_window->label(title);
    }
}


// These functions set parameters for default look of sliders

void default_look(Slider* slider) 
{
    slider->type(Slider::TICK_BOTH);
    slider->set_vertical();
    slider->minimum(-100); 
    slider->maximum(100);
    slider->value(1);
    slider->step(1); 
    slider->align(ALIGN_TOP);
}

void default_look_b(Slider* balance_slider) 
{
    balance_slider->type(1); 
    balance_slider->minimum(0.00);
    balance_slider->maximum(1.00); 
    balance_slider->step(0.01);
    balance_slider->value(0.01);
}


// Functions for various control groups - this is mostly copy-paste

void cb_volume(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_VOLUME, volume_slider, volume_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_VOLUME, volume_slider, volume_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_VOLUME, volume_slider, volume_balance, volume_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_VOLUME, volume_rec);
}

void cb_cd(Slider* o, void *i) {
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_CD, cd_slider, cd_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_CD, cd_slider, cd_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_CD, cd_slider, cd_balance, cd_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_CD, cd_rec);
}

void cb_pcm(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_PCM, pcm_slider, pcm_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_PCM, pcm_slider, pcm_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_PCM, pcm_slider, pcm_balance, pcm_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_PCM, pcm_rec);
}

void cb_synth(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_SYNTH, synth_slider, synth_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_SYNTH, synth_slider, synth_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_SYNTH, synth_slider, synth_balance, synth_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_SYNTH, synth_rec);
}

void cb_line(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_LINE, line_slider, line_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_LINE, line_slider, line_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_LINE, line_slider, line_balance, line_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_LINE, line_rec);
}

void cb_bass(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_BASS, bass_slider, bass_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_BASS, bass_slider, bass_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_BASS, bass_slider, bass_balance, bass_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_BASS, bass_rec);
}

void cb_treble(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_TREBLE, treble_slider, treble_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_TREBLE, treble_slider, treble_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_TREBLE, treble_slider, treble_balance, treble_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_TREBLE, treble_rec);
}

void cb_mic(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_MIC, mic_slider, mic_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_MIC, mic_slider, mic_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_MIC, mic_slider, mic_balance, mic_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_MIC, mic_rec);
}

void cb_speaker(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_SPEAKER, speaker_slider, speaker_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_SPEAKER, speaker_slider, speaker_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_SPEAKER, speaker_slider, speaker_balance, speaker_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_SPEAKER, speaker_rec);
}

void cb_imix(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_IMIX, imix_slider, imix_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_IMIX, imix_slider, imix_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_IMIX, imix_slider, imix_balance, imix_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_IMIX, imix_rec);
}

void cb_igain(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_IGAIN, igain_slider, igain_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_IGAIN, igain_slider, igain_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_IGAIN, igain_slider, igain_balance, igain_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_IGAIN, igain_rec);
}

void cb_ogain(Slider* o, void *i) 
{
    int x = (int) i;
    if (x == 1) set_device(mixer_device, SOUND_MIXER_OGAIN, ogain_slider, ogain_balance);
    if (x == 2) set_device(mixer_device, SOUND_MIXER_OGAIN, ogain_slider, ogain_balance);
    if (x == 3) set_mute(mixer_device, SOUND_MIXER_OGAIN, ogain_slider, ogain_balance, ogain_mute);
    if (x == 4) set_rec(mixer_device, SOUND_MIXER_OGAIN, ogain_rec);
}


// Menu callback functions

void cb_Quit(Widget*, void*) 
{
    main_window->hide();
}

static void cb_About(Item*, void*) {
  about_dialog("Volume Control","1.0","");
}

void cb_SimpleMode(Widget*, void*) {
	if (!simplemode) {
		simplemode = true;
		synth_slider->hide();
		synth_balance->hide();
		synth_mute->hide();
		synth_rec->hide();
		bass_slider->hide();
		bass_balance->hide();
		bass_mute->hide();
		bass_rec->hide();
		treble_slider->hide();
		treble_balance->hide();
		treble_mute->hide();
		treble_rec->hide();
		mic_slider->hide();
		mic_balance->hide();
		mic_mute->hide();
		mic_rec->hide();
		speaker_slider->hide();
		speaker_balance->hide();
		speaker_mute->hide();
		speaker_rec->hide();
		imix_slider->hide();
		imix_balance->hide();
		imix_mute->hide();
		imix_rec->hide();
		igain_slider->hide();
		igain_balance->hide();
		igain_mute->hide();
		igain_rec->hide();
		ogain_slider->hide();
		ogain_balance->hide();
		ogain_mute->hide();
		ogain_rec->hide();
		main_window->resize(250,205);
	} else {
		simplemode = false;
		synth_slider->show();
		synth_balance->show();
		synth_mute->show();
		synth_rec->show();
		bass_slider->show();
		bass_balance->show();
		bass_mute->show();
		bass_rec->show();
		treble_slider->show();
		treble_balance->show();
		treble_mute->show();
		treble_rec->show();
		mic_slider->show();
		mic_balance->show();
		mic_mute->show();
		mic_rec->show();
		speaker_slider->show();
		speaker_balance->show();
		speaker_mute->show();
		speaker_rec->show();
		imix_slider->show();
		imix_balance->show();
		imix_mute->show();
		imix_rec->show();
		igain_slider->show();
		igain_balance->show();
		igain_mute->show();
		igain_rec->show();
		ogain_slider->show();
		ogain_balance->show();
		ogain_mute->show();
		ogain_rec->show();
		main_window->resize(720,205);
	}
	globalConfig.set("Sound mixer", "Simplemode", simplemode);
}


// Main window design

int main (int argc, char **argv) 
{
//    fl_init_locale_support("evolume", PREFIX"/share/locale");

    globalConfig.get("Sound mixer", "Device", device, "/dev/mixer", sizeof(device));
    globalConfig.get("Sound mixer", "Simplemode", simplemode, true);

    main_window = new Window(720, 205, _("Volume control"));
    if (simplemode) main_window->resize(250,205);
    main_window->begin();
    
    MenuBar *vc_menubar = new MenuBar(0, 0, 724, 25);
    vc_menubar->begin();
    
    ItemGroup file(_("&File"));
    file.begin();
          Item* pref_item = new Item(_("Preferences"));
                   pref_item->shortcut(CTRL+'p');
		   pref_item->callback(PreferencesDialog);
    
          Item* quit_item = new Item(_("Quit"));
                   quit_item->shortcut(CTRL+'q');
		   quit_item->callback(cb_Quit);
		   
    file.end();

    ItemGroup view(_("&View"));
    view.begin();
          Item* mode_item = new Item(_("Simple mode"));
                   mode_item->shortcut(CTRL+'s');
		   mode_item->type(Item::TOGGLE);
		   mode_item->callback(cb_SimpleMode);
		   if (simplemode) mode_item->set();
    view.end();

    ItemGroup help(_("&Help"));
    help.begin();
	  Item* about_item = new Item(_("About"));
                   about_item->shortcut(CTRL+'a');
		   about_item->callback((Callback*)cb_About);
    help.end();
    vc_menubar->end();

    {Divider* o = new Divider();
     o->resize(0, 24, 724, 3);
    }

    volume_slider = new Slider(20, 50, 20, 80, "VOL");
                default_look(volume_slider);
        volume_balance = new Slider(10, 135, 40, 15, "Balance");
		default_look_b(volume_balance);
        volume_mute = new CheckButton(5, 165, 20, 20, "Mute");
		volume_mute->align(ALIGN_BOTTOM);
        volume_rec = new CheckButton(35, 165, 20, 20, "Rec");
    	        volume_rec->align(ALIGN_BOTTOM);
		    
    cd_slider = new Slider(80, 50, 20, 80, "CD");
                default_look(cd_slider);
	cd_balance = new Slider(70, 135, 40, 15, "Balance");
		default_look_b(cd_balance);
	cd_mute = new CheckButton(65, 165, 20, 20, "Mute");
		cd_mute->align(ALIGN_BOTTOM);
	cd_rec = new CheckButton(95, 165, 20, 20, "Rec");
	        cd_rec->align(ALIGN_BOTTOM);
     
    pcm_slider = new Slider(140, 50, 20, 80, "PCM");
	         default_look(pcm_slider);
    pcm_balance = new Slider(130, 135, 40, 15, "Balance");
		default_look_b(pcm_balance);
    pcm_mute = new CheckButton(125, 165, 20, 20, "Mute");
      pcm_mute->align(ALIGN_BOTTOM);
    pcm_rec = new CheckButton(155, 165, 20, 20, "Rec");
     pcm_rec->align(ALIGN_BOTTOM);
      
    line_slider = new Slider(200, 50, 20, 80, "LINE");
                  default_look(line_slider);
    line_balance = new Slider(190, 135, 40, 15, "Balance");
		default_look_b(line_balance);
    line_mute = new CheckButton(185, 165, 20, 20, "Mute");
                   line_mute->align(ALIGN_BOTTOM);
    line_rec = new CheckButton(215, 165, 20, 20, "Rec");
                   line_rec->align(ALIGN_BOTTOM);
		  
    synth_slider = new Slider(260, 50, 20, 80, "SYNTH");
                   default_look(synth_slider);
    synth_balance = new Slider(250, 135, 40, 15, "Balance");
		   default_look_b(synth_balance);
    synth_mute = new CheckButton(245, 165, 20, 20, "Mute");
                   synth_mute->align(ALIGN_BOTTOM);
    synth_rec = new CheckButton(275, 165, 20, 20, "Rec");
                   synth_rec->align(ALIGN_BOTTOM);
   
    bass_slider = new Slider(320, 50, 20, 80, "BASS");
                  default_look(bass_slider);
    bass_balance = new Slider(310, 135, 40, 15, "Balance");
		  default_look_b(bass_balance);
    bass_mute = new CheckButton(305, 165, 20, 20, "Mute");
                  bass_mute->align(ALIGN_BOTTOM);
    bass_rec = new CheckButton(335, 165, 20, 20, "Rec");
                  bass_rec->align(ALIGN_BOTTOM);
      
    treble_slider = new Slider(380, 50, 20, 80, "TREBLE");
                    default_look(treble_slider);
    treble_balance = new Slider(370, 135, 40, 15, "Balance");
                  default_look_b(treble_balance);
    treble_mute = new CheckButton(365, 165, 20, 20, "Mute");
      treble_mute->align(ALIGN_BOTTOM);
    treble_rec = new CheckButton(395, 165, 20, 20, "Rec");
      treble_rec->align(ALIGN_BOTTOM);
		    
    mic_slider = new Slider(440, 50, 20, 80, "MIC");
                 default_look(mic_slider);
    mic_balance = new Slider(430, 135, 40, 15, "Balance");
		default_look_b(mic_balance);
    mic_mute = new CheckButton(425, 165, 20, 20, "Mute");
                 mic_mute->align(ALIGN_BOTTOM);
    mic_rec = new CheckButton(455, 165, 20, 20, "Rec");
                 mic_rec->align(ALIGN_BOTTOM);
		 
    speaker_slider = new Slider(500, 50, 20, 80, "SPK");
                     default_look(speaker_slider);
    speaker_balance = new Slider(490, 135, 40, 15, "Balance");
         	     default_look_b(speaker_balance);
    speaker_mute = new CheckButton(485, 165, 20, 20, "Mute");
                     speaker_mute->align(ALIGN_BOTTOM);
    speaker_rec = new CheckButton(515, 165, 20, 20, "Rec");
                speaker_rec->align(ALIGN_BOTTOM);
     
    imix_slider = new Slider(560, 50, 20, 80, "IMIX");
	          default_look(imix_slider);
    imix_balance = new Slider(550, 135, 40, 15, "Balance");
                  default_look_b(imix_balance);
    imix_mute = new CheckButton(545, 165, 20, 20, "Mute");
                  imix_mute->align(ALIGN_BOTTOM);
    imix_rec = new CheckButton(575, 165, 20, 20, "Rec");
                  imix_rec->align(ALIGN_BOTTOM);
    
    igain_slider = new Slider(620, 50, 20, 80, "IGAIN");
    	           default_look(igain_slider);
    igain_balance = new Slider(610, 135, 40, 15, "Balance");
    		   default_look_b(igain_balance);
    igain_mute = new CheckButton(605, 165, 20, 20, "Mute");
                   igain_mute->align(ALIGN_BOTTOM);
    igain_rec = new CheckButton(635, 165, 20, 20, "Rec");
                   igain_rec->align(ALIGN_BOTTOM);
    
    ogain_slider = new Slider(680, 50, 20, 80, "OGAIN");
    	           default_look(ogain_slider);
    ogain_balance = new Slider(670, 135, 40, 15, "Balance");
		   default_look_b(ogain_balance);
    ogain_mute = new CheckButton(665, 165, 20, 20, "Mute");
	           ogain_mute->align(ALIGN_BOTTOM);
    ogain_rec = new CheckButton(695, 165, 20, 20, "Rec");
		   ogain_rec->align(ALIGN_BOTTOM);
		   
    mixer_device = open(device, O_RDWR);
    
    if (mixer_device == -1) 
    { 
    	    alert(_("Opening mixer device %s failed. Setup correct device in configuration dialog."), device);
	    volume_slider->deactivate(); cd_slider->deactivate();
	    pcm_slider->deactivate(); synth_slider->deactivate();
	    line_slider->deactivate(); bass_slider->deactivate();
	    treble_slider->deactivate(); mic_slider->deactivate();
    	    speaker_slider->deactivate(); imix_slider->deactivate();
	    igain_slider->deactivate(); ogain_slider->deactivate();
    }

    update_info();
    
    volume_slider->callback( (Callback*) cb_volume, (void*) 1 );
    volume_balance->callback( (Callback*) cb_volume,(void *) 2 );    
    volume_mute->callback( (Callback*) cb_volume,(void *) 3 );    
    volume_rec->callback( (Callback*) cb_volume,(void *) 4 );
    get_device_info(mixer_device, volume_slider, volume_balance, volume_rec, SOUND_MIXER_VOLUME);    
    
    cd_slider->callback( (Callback*) cb_cd, (void *) 1 );
    cd_balance->callback( (Callback*) cb_cd,(void *) 2 );    
    cd_mute->callback( (Callback*) cb_cd,(void *) 3 );    
    cd_rec->callback( (Callback*) cb_cd,(void *) 4 );
    get_device_info(mixer_device, cd_slider, cd_balance, cd_rec, SOUND_MIXER_CD);

    pcm_slider->callback( (Callback*) cb_pcm, (void *) 1 );
    pcm_balance->callback( (Callback*) cb_pcm,(void *) 2 );    
    pcm_mute->callback( (Callback*) cb_pcm,(void *) 3 );    
    pcm_rec->callback( (Callback*) cb_pcm,(void *) 4 );
    get_device_info(mixer_device, pcm_slider, pcm_balance, pcm_rec, SOUND_MIXER_PCM);

    synth_slider->callback( (Callback*) cb_synth, (void *) 1 );
    synth_balance->callback( (Callback*) cb_synth,(void *) 2 );    
    synth_mute->callback( (Callback*) cb_synth,(void *) 3 );    
    synth_rec->callback( (Callback*) cb_synth,(void *) 4 );
    get_device_info(mixer_device, synth_slider, synth_balance, synth_rec, SOUND_MIXER_SYNTH);    
    
    line_slider->callback( (Callback*) cb_line, (void *) 1 );
    line_balance->callback( (Callback*) cb_line,(void *) 2 );    
    line_mute->callback( (Callback*) cb_line,(void *) 3 );    
    line_rec->callback( (Callback*) cb_line,(void *) 4 );
    get_device_info(mixer_device, line_slider, line_balance, line_rec, SOUND_MIXER_LINE);    
    
    bass_slider->callback( (Callback*) cb_bass, (void *) 1 );
    bass_balance->callback( (Callback*) cb_bass,(void *) 2 );    
    bass_mute->callback( (Callback*) cb_bass,(void *) 3 );    
    bass_rec->callback( (Callback*) cb_bass,(void *) 4 );
    get_device_info(mixer_device, bass_slider, bass_balance, bass_rec, SOUND_MIXER_BASS);    
     
    treble_slider->callback( (Callback*) cb_treble, (void *) 1 );
    treble_balance->callback( (Callback*) cb_treble,(void *) 2 );    
    treble_mute->callback( (Callback*) cb_treble,(void *) 3 );    
    treble_rec->callback( (Callback*) cb_treble,(void *) 4 );
    get_device_info(mixer_device, treble_slider, treble_balance, treble_rec, SOUND_MIXER_TREBLE);    
    
    mic_slider->callback( (Callback*) cb_mic, (void *) 1 );
    mic_balance->callback( (Callback*) cb_mic,(void *) 2 );    
    mic_mute->callback( (Callback*) cb_mic,(void *) 3 );    
    mic_rec->callback( (Callback*) cb_mic,(void *) 4 );
    get_device_info(mixer_device, mic_slider, mic_balance, mic_rec, SOUND_MIXER_MIC);    
     
    speaker_slider->callback( (Callback*) cb_speaker, (void *) 1 );
    speaker_balance->callback( (Callback*) cb_speaker,(void *) 2 );    
    speaker_mute->callback( (Callback*) cb_speaker,(void *) 3 );    
    speaker_rec->callback( (Callback*) cb_speaker,(void *) 4 );
    get_device_info(mixer_device, speaker_slider, speaker_balance, speaker_rec, SOUND_MIXER_SPEAKER);    
     
    imix_slider->callback( (Callback*) cb_imix, (void *) 1 );
    imix_balance->callback( (Callback*) cb_imix,(void *) 2 );    
    imix_mute->callback( (Callback*) cb_imix,(void *) 3 );    
    imix_rec->callback( (Callback*) cb_imix,(void *) 4 );
    get_device_info(mixer_device, imix_slider, imix_balance, imix_rec, SOUND_MIXER_IMIX);    
    
    igain_slider->callback( (Callback*) cb_igain, (void *) 1 );
    igain_balance->callback( (Callback*) cb_igain,(void *) 2 );    
    igain_mute->callback( (Callback*) cb_igain,(void *) 3 );    
    igain_rec->callback( (Callback*) cb_igain,(void *) 4 );
    get_device_info(mixer_device, igain_slider, igain_balance, igain_rec, SOUND_MIXER_IGAIN);    
	       
    ogain_slider->callback( (Callback*) cb_ogain, (void *) 1 );
    ogain_balance->callback( (Callback*) cb_ogain,(void *) 2 );    
    ogain_mute->callback( (Callback*) cb_ogain,(void *) 3 );    
    ogain_rec->callback( (Callback*) cb_ogain,(void *) 4 );
    get_device_info(mixer_device, ogain_slider, ogain_balance, ogain_rec, SOUND_MIXER_OGAIN);    

    main_window->end();
    main_window->show(argc, argv);
    
    simplemode = !simplemode;	// cb_SimpleMode inverts meaning
    cb_SimpleMode(0,0);

    return run();
}

