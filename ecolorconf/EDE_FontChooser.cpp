/*
 * $Id$
 *
 * Font chooser widget
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * Based on:
 * Font demo program for the Fast Light Tool Kit (FLTK).
 * Copyright 1998-2006 by Bill Spitzak and others.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "EDE_FontChooser.h"

#define MAXSIZE 64

static fltk::Window *form = (fltk::Window *)0;
static fltk::Widget* id_box;


// class for font preview window

class FontDisplay : public fltk::Widget {
	void draw();
public:
	fltk::Font* font; 
	unsigned size; 
	const char* encoding;
	FontDisplay(fltk::Box* B, int X, int Y, int W, int H, const char* L = 0) :
		fltk::Widget(X,Y,W,H,L) {box(B); font = 0; size = 14;}
};

void FontDisplay::draw() {
  draw_box();
  fltk::push_clip(2,2,w()-2,h()-2);
  const char* saved_encoding = fltk::get_encoding();
  fltk::set_encoding(encoding);
  fltk::setfont(font, (float) size);
  id_box->label(fltk::Font::current_name());
  id_box->redraw();
  fltk::setcolor(fltk::BLACK);
  char buffer[32];
  for (int Y = 1; Y < 8; Y++) {
    for (int X = 0; X < 32; X++) buffer[X] = (32*Y+X);
    fltk::drawtext(buffer, 32, 3, 3+(size+leading())*Y);
  }
  fltk::set_encoding(saved_encoding);
  fltk::pop_clip();
}



// other variables

static FontDisplay *textobj;

static fltk::Browser *fontobj, *sizeobj, *encobj;

static fltk::Font** all_fonts; // list returned by fltk
static int numfonts=0;

static fltk::Group *button_group;
static fltk::CheckButton* bold_button, *italic_button;
static fltk::Button *ok_button, *cancel_button;

bool return_value = false;



// callback functions


// callback for list of fonts

void font_cb(fltk::Widget *, long) 
{
	int fn = fontobj->value();
//DEBUG
//printf("font: %d    name: %s   bigname: %s\n", fn, fonts[fn]->name(), fonts[fn]->system_name());
	fltk::Font* f = all_fonts[fn];

	// are bold and italic available?
	if (f->bold() == f) 
		bold_button->deactivate();
	else 
		bold_button->activate();
	if (f->italic() == f) 
		italic_button->deactivate();
	else 
		italic_button->activate();
	if (bold_button->value()) f = f->bold();
	if (italic_button->value()) f = f->italic();

	textobj->font = f;

	// Populate the encobj (browser for font encodings)
	char saved[30];
	if (textobj->encoding)
		strncpy(saved, textobj->encoding, 29);
	else
		saved[0] = 0;
	encobj->clear();

	const char** encodings;
	int ne = f->encodings(encodings);
	int picked = -1;
	int iso8859 = 0;

	// On XFT encoding is always Unicode, so encodings() will return 0
	if (ne==0) 
	{ 
		encobj->add("Unicode"); 
		encobj->deselect();
		encobj->deactivate(); 
		textobj->encoding=0; 
	}
	else 
	{
		encobj->activate();
		for (int i = 0; i < ne; i++) {
			encobj->add(encodings[i]);
			if (!strcmp(encodings[i], saved)) picked = i;
			if (!strcmp(encodings[i], fltk::get_encoding())) iso8859 = i;
		}
		if (picked < 0) picked = iso8859;
		textobj->encoding = encodings[picked];
		encobj->value(picked);
	}

	// Populate the sizeobj (browser for font sizes)
	int pickedsize;
	if (sizeobj->value() > 0) {
		pickedsize = atoi(sizeobj->child(sizeobj->value())->label());
	} else {
		pickedsize = 14;
	}
	sizeobj->clear();

	int *s;
	int n = f->sizes(s);
	if(!n) {
		// no sizes (this only happens on X)
		for (int i = 1; i<MAXSIZE; i++) {
			char buf[20];
			sprintf(buf,"%d",i);
			sizeobj->add(buf);
		}
		sizeobj->value((int)fltk::getsize()-1); //pickedsize
		textobj->size = (int)fltk::getsize();
		
		// fl_font(f, pickedsize); lets fix this...
	} else if (s[0] == 0) {
		// many sizes;
		int j = 1;
		for (int i = 1; i<MAXSIZE || i<s[n-1]; i++) {
			char buf[20];
			sprintf(buf,"%d",i);
			fltk::Widget *w = sizeobj->add(buf);
			if (j < n && i==s[j]) {
				w->labelfont(w->labelfont()->bold());
				w->labelcolor(fltk::RED);
				j++;
			}
			//if (j < n && i==s[j]) {sprintf(buf,"@b;%d",i); j++;}
		}
		sizeobj->value(pickedsize-1);
		textobj->size = pickedsize;
	} else {
		// some sizes -- when is this used?
		int w = 0;
		for (int i = 0; i < n; i++) {
			if (s[i]<=pickedsize) w = i;
			char buf[20];
			sprintf(buf,"%d",s[i]);
			fltk::Widget *w = sizeobj->add(buf);
			w->labelfont(w->labelfont()->bold());
			//sprintf(buf,"@b;%d",s[i]);
		}
		sizeobj->value(w);
		textobj->size = s[w];
	}

	encobj->redraw();
	sizeobj->redraw();
	textobj->redraw();
//	encobj->relayout();
//	sizeobj->relayout();
//	textobj->relayout();

//  id_box->label(textobj->font->system_name());
//  id_box->redraw();
	button_group->redraw();	// needed?
}

void encoding_cb(fltk::Widget *, long) {
  if (encobj->children() < 2) return; // XFT
  int i = encobj->value();
//  textobj->encoding = encobj->text(i);
  textobj->encoding = encobj->child(i)->label();
  textobj->redraw();
  id_box->redraw();
}

void size_cb(fltk::Widget *, long) {
  int i = sizeobj->value();
  //const char *c = sizeobj->text(i);
  const char *c = sizeobj->child(i)->label();
  while (*c < '0' || *c > '9') c++;
  textobj->size = atoi(c);
  textobj->redraw();
//  id_box->redraw();
}

void return_cb(fltk::Widget *, long ret) {
    return_value = ret;
    form->hide();
}


// TODO: rewrite this in fluid...
void create_the_forms() 
{
  if(form) return;
  form = new fltk::Window(550, 420, _("Select font..."));
  form->set_double_buffer();
  form->begin();
  
  textobj = new FontDisplay(fltk::ENGRAVED_BOX,10,10,530,160);
  textobj->clear_flag(fltk::ALIGN_MASK);
  textobj->set_flag(fltk::ALIGN_TOP|fltk::ALIGN_LEFT|fltk::ALIGN_INSIDE|fltk::ALIGN_CLIP);
  id_box = new fltk::Widget(10, 172, 530, 15);
  id_box->box(fltk::ENGRAVED_BOX);
  id_box->labelsize(10);
  id_box->set_flag(fltk::ALIGN_INSIDE|fltk::ALIGN_CLIP);
  button_group = new fltk::Group(10, 190, 140, 20);
  button_group->begin();
  bold_button = new fltk::CheckButton(0, 0, 70, 20, "Bold");
  bold_button->labelfont(bold_button->labelfont()->bold());
  bold_button->callback(font_cb, 1);
  italic_button = new fltk::CheckButton(70, 0, 70, 20, "Italic");
  italic_button->labelfont(italic_button->labelfont()->italic());
  italic_button->callback(font_cb, 1);
  button_group->end();
  fontobj = new fltk::Browser(10, 210, 280, 170);
  fontobj->when(fltk::WHEN_CHANGED);
  fontobj->callback(font_cb);
  form->resizable(fontobj);
  encobj = new fltk::Browser(300, 210, 100, 170);
  encobj->when(fltk::WHEN_CHANGED);
  encobj->callback(encoding_cb, 1);
  sizeobj = new fltk::Browser(410, 210, 130, 170);
  sizeobj->when(fltk::WHEN_CHANGED);
  sizeobj->callback(size_cb);
  
  ok_button = new fltk::Button(380, 390, 80, 25, _("&OK"));
  ok_button->callback(return_cb, 1);

  cancel_button = new fltk::Button(465, 390, 80, 25, _("&Cancel"));
  cancel_button->callback(return_cb, 0);
  
  form->end();
}


// search for largest <= selected size:
int find_best_size(fltk::Font* font, int selected)
{
	int *allsizes;
	int numsizes = font->sizes(allsizes);

//	This is a bug in efltk
	if (numsizes <= 1) return selected;

	for (int i=1; i<numsizes; i++) {
		if (allsizes[i] > selected)
			return allsizes[i-1];
	}

	return allsizes[numsizes-1];
}


EDEFont font_chooser(EDEFont current_font) 
{
	EDEFont return_font;
	create_the_forms();
	if(!numfonts) numfonts = fltk::list_fonts(all_fonts);

	// populate list of fonts
	fontobj->clear();
	for(int i = 0; i < numfonts; i++) {
		fontobj->add(all_fonts[i]->name());
 		if (current_font.font && (strcasecmp(current_font.font->name(),all_fonts[i]->name())==0)) 
			// it's a substring
			fontobj->value(i);
	}
	/*char* currentname = strdup(current_font.font->name());
	fsor(int i = 0; i < numfonts; i++) {
		char* fontname = strdup(all_fonts[i]->name());
		fontobj->add(fontname);
 		if (currentname.lower_case().pos(fontname.lower_case())==0) // it's a substring
			fontobj->value(i);
	}*/

	// set bold, italic
	/*if (currentname.pos(" bold italic") == currentname.length()-12) {
		bold_button->value(true);
		italic_button->value(true);
	} else if (currentname.pos(" italic") == currentname.length()-7) {
		italic_button->value(true);
	} else if (currentname.pos(" bold") == currentname.length()-5) {
		bold_button->value(true);
	}*/

	// populate other lists
	textobj->encoding = current_font.encoding; // TODO: what if we're using XFT?
	font_cb(fontobj,0);
	for (int i=0; i < sizeobj->children(); i++) {
		if (atoi(sizeobj->child(i)->label()) == current_font.size) {
			sizeobj->value(i);
			size_cb(sizeobj,0);
		}
	}

	//
	form->show();
	form->exec();

	// we have to construct a proper EDEFont to return
	return_font.defined = false;
	if (return_value)
	{
		return_font.font = fltk::font(fontobj->child(fontobj->value())->label()); //Style.h
		if (bold_button->value()) return_font.font = return_font.font->bold();
		if (italic_button->value()) return_font.font = return_font.font->italic();

		int size = atoi(sizeobj->child(sizeobj->value())->label());
		return_font.size = find_best_size(return_font.font, size);

		// on XFT encoding is always Unicode, so this field can be blank
		if (encobj->children() > 1) 
			return_font.encoding = strdup(encobj->child(encobj->value())->label());
		else
			return_font.encoding = 0;
		
		return_font.defined = true;
	}
	return return_font;
}

//
// End of "$Id: efontdialog.cpp,v 1.3 2005/03/20 15:53:58 vljubovic Exp $".
//
