// config.h
// You can edit these symbols to change the behavior & appearance of flwm.
// Turning off features will make flwm smaller too!

////////////////////////////////////////////////////////////////
// BEHAVIOR:

// Turn this on for click-to-type (rather than point-to-type).
// On: clicking on the window gives it focus
// Off: pointing at the window gives it the focus.
#define CLICK_TO_TYPE 1

// For point-to-type, sticky focus means you don't lose the focus
// until you move the cursor to another window that wants focus.
// If this is off you lose focus as soon as the cursor goes outside
// the window (such as to the desktop):
#define STICKY_FOCUS 0

// For point-to-type, after this many seconds the window is raised,
// nothing is done if this is not defined:
//#define AUTO_RAISE 0.5

// set this to zero to remove the multiple-desktop code.  This will
// make flwm about 20K smaller
#define DESKTOPS 1

////////////////////////////////////////////////////////////////
// APPEARANCE:

// how many pixels from edge for resize handle:
#define RESIZE_EDGE 6

// must drag window this far off screen to snap the border off the screen:
#define EDGE_SNAP 20
// must drag window this far off screen to actually move it off screen:
#define SCREEN_SNAP 40

#define SHAPE 1


