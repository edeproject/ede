#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>

#include "Panel.h"
#include "AppletManager.h"

int main(int argc, char **argv) {
	Panel* panel = new Panel();
	panel->load_applets();
	panel->show();
	return Fl::run();
}
