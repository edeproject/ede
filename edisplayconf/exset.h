#ifndef exset_h
#define exset_h

#include <fltk/x.h> //#include <efltk/x.h>
#include "../edelib2/Run.h" //#include <efltk/Fl_Util.h>

#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>

using namespace edelib;

class Exset {

public:
    Exset(void) {}
    ~Exset() {}

  char str[80];

  void set_pattern(int delay, int pattern = -1){
    if( pattern !=  -1 ){
      sprintf(str,"xset s %d %d",delay*60,pattern*60);
    	run_program(str);
    }else{
      sprintf(str,"xset s %d",delay*60);
      run_program(str);
    }
  }
  void set_check_blank(int blank){
   	sprintf(str,"xset s %s",blank ? "on" : "off");
  	run_program(str);
  }
  void set_blank(int blank){
    sprintf(str,"xset s %s",blank ? "blank" : "noblank");
    run_program(str);
  }

  void test_blank() { run_program("xset s activate"); }
  void test_bell() {  run_program("xkbbell"); }

  void set_mouse(int accel, int thresh){
      XChangePointerControl(fltk::xdisplay, true, true, accel,
                            1, thresh);
  }


  void set_bell(int volume, int pitch, int duration, int sound = 0){
      XKeyboardControl _ctrl;
      unsigned long mask = KBBellPercent | KBBellPitch | KBBellDuration;

      _ctrl.bell_percent = volume;
      _ctrl.bell_pitch   = pitch;
      _ctrl.bell_duration = duration;

      set_xset(&_ctrl,mask);
  }

  void set_keybd( int repeat, int clicks) {
      XKeyboardControl _ctrl;
      unsigned long mask = KBKeyClickPercent | KBAutoRepeatMode;

      _ctrl.key_click_percent = clicks;
      _ctrl.auto_repeat_mode = (repeat ? AutoRepeatModeOn : AutoRepeatModeOff);

      set_xset(&_ctrl,mask);
  }

  void set_xset(XKeyboardControl * ctrl, unsigned long mask){
      XChangeKeyboardControl(fltk::xdisplay, mask, ctrl);
  }
};
#endif

