#ifndef __TRAYWINDOW_H__
#define __TRAYWINDOW_H__

#include <FL/Fl_Window.H>
#include <FL/Fl_Image.H>
#include <edelib/String.h>

EDELIB_NS_USING(String)

/* window dedicated to displaying tray window with icon resized to window size */
class TrayWindow : public Fl_Window {
private:
	Fl_RGB_Image *img;
	String        ttip;
public:
	TrayWindow(int W, int H);
	void set_image(Fl_RGB_Image *im);
	void draw(void);
	void set_tooltip(char *t);
};

#endif
