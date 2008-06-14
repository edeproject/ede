#ifndef _OpenWith_h_
#define _OpenWith_h_

#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/String.h>
#include <edelib/List.h>

#include <FL/Fl_Input.H>

class OpenWith : public edelib::Window {
private:
	const char* _file;
	edelib::list<edelib::String> programs;
	Fl_Input *inpt;
public:
	OpenWith();
	const char* file() { return _file; }
	void show(const char* pfile) { 
		_file=pfile;
		inpt->value("");
		edelib::Window::show();
	}
};


#endif
