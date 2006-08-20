#include <efltk/Fl.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Group.h>
#include <efltk/Fl_Button.h>
#include <stdio.h>

#define MRSIZE 2

void cb_move_left(Fl_Widget*, void* ww)
{
	Fl_Window* win = (Fl_Window*) ww;
	printf("move left(before) x: %i y: %i\n", win->x(), win->y());
	win->position(win->x() - MRSIZE, win->y());
	printf("move left(after)  x: %i y: %i\n", win->x(), win->y());
}

void cb_move_right(Fl_Widget*, void* ww)
{
	Fl_Window* win = (Fl_Window*) ww;
	printf("move right(before) x: %i y: %i\n", win->x(), win->y());
	win->position(win->x() + MRSIZE, win->y());
	printf("move right(after)  x: %i y: %i\n", win->x(), win->y());
}

void cb_move_up(Fl_Widget*, void* ww)
{
	Fl_Window* win = (Fl_Window*) ww;
	printf("move up(before) x: %i y: %i\n", win->x(), win->y());
	win->position(win->x(), win->y() - MRSIZE);
	printf("move up(after)  x: %i y: %i\n", win->x(), win->y());
}

void cb_move_down(Fl_Widget*, void* ww)
{
	Fl_Window* win = (Fl_Window*) ww;
	printf("move down(before) x: %i y: %i\n", win->x(), win->y());
	win->position(win->x(), win->y() + MRSIZE);
	printf("move down(after)  x: %i y: %i\n", win->x(), win->y());
}


int main (int argc, char **argv) 
{
	Fl_Window* win = new Fl_Window(300, 290, "Basic Window Operations");
	win->shortcut(0xff1b);

	Fl_Box* bbb = new Fl_Box(10, 10, 280, 50, "Below buttons should apply changes to this window");
	bbb->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);

	Fl_Group* move_group = new Fl_Group(10, 85, 280, 75, "Move");
	move_group->box(FL_ENGRAVED_BOX);
	move_group->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

	Fl_Button* move_up = new Fl_Button(130, 10, 25, 25, "^");
	move_up->callback(cb_move_up, win);
	//move_left->label_type(FL_SYMBOL_LABEL);
	
	Fl_Button* move_down = new Fl_Button(130, 35, 25, 25, "v");
	move_down->callback(cb_move_down, win);
	//move_right->label_type(FL_SYMBOL_LABEL);
      
	Fl_Button* move_left = new Fl_Button(105, 35, 25, 25, "<");
	move_left->callback(cb_move_left, win);
	//move_up->label_type(FL_SYMBOL_LABEL);
      
	Fl_Button* move_right = new Fl_Button(155, 35, 25, 25, ">");
	move_right->callback(cb_move_right, win);
	//move_down->label_type(FL_SYMBOL_LABEL);

	move_group->end();


	Fl_Group* resize_group = new Fl_Group(10, 190, 280, 75, "Resize");
	resize_group->box(FL_ENGRAVED_BOX);
	resize_group->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

	Fl_Button* resize_up = new Fl_Button(130, 10, 25, 25, "^");
	//resize_left->label_type(FL_SYMBOL_LABEL);

	Fl_Button* resize_down = new Fl_Button(130, 35, 25, 25, "v");
	//resize_right->label_type(FL_SYMBOL_LABEL);

	Fl_Button* resize_left = new Fl_Button(105, 35, 25, 25, "<");
	//resize_up->label_type(FL_SYMBOL_LABEL);

	Fl_Button* resize_right = new Fl_Button(155, 35, 25, 25, ">");
	//resize_down->label_type(FL_SYMBOL_LABEL);
      
	resize_group->end();
    
	win->end();
	win->show(argc, argv);
	return Fl::run();
}
