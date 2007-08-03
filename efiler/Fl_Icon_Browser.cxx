
// Icon Browser - modified from Fl_Browser.cxx by Vedran Ljubovic

// "$Id: Fl_Icon_Browser.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Browser widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "Fl_Icon_Browser.h"

// I modified this from the original Forms data to use a linked list
// so that the number of items in the browser and size of those items
// is unlimited.  The only problem is that the old browser used an
// index number to identify a line, and it is slow to convert from/to
// a pointer.  I use a cache of the last match to try to speed this
// up.

// Also added the ability to "hide" a line.  This set's it's height to
// zero, so the Fl_Browser_ cannot pick it.

#define SELECTED 1
#define NOTDISPLAYED 2

struct FL_BLINE {	// data is in a linked list of these
  FL_BLINE* prev;
  FL_BLINE* next;
  void* data;
  Fl_Image* icon;
  short length;		// sizeof(txt)-1, may be longer than string
  char flags;		// selected, displayed
  int level;		// indentation level for tree
  char txt[1];		// start of allocated array
};

void* Fl_Icon_Browser::item_first() const {return first;}

void* Fl_Icon_Browser::item_next(void* l) const {return ((FL_BLINE*)l)->next;}

void* Fl_Icon_Browser::item_prev(void* l) const {return ((FL_BLINE*)l)->prev;}

int Fl_Icon_Browser::item_selected(void* l) const {
  return ((FL_BLINE*)l)->flags&SELECTED;}

void Fl_Icon_Browser::item_select(void* l, int v) {
  if (v) ((FL_BLINE*)l)->flags |= SELECTED;
  else ((FL_BLINE*)l)->flags &= ~SELECTED;
}

FL_BLINE* Fl_Icon_Browser::find_line(int line) const {
  int n; FL_BLINE* l;
  if (line == cacheline) return cache;
  if (cacheline && line > (cacheline/2) && line < ((cacheline+lines)/2)) {
    n = cacheline; l = cache;
  } else if (line <= (lines/2)) {
    n = 1; l = first;
  } else {
    n = lines; l = last;
  }
  for (; n < line && l; n++) l = l->next;
  for (; n > line && l; n--) l = l->prev;
  ((Fl_Icon_Browser*)this)->cacheline = line;
  ((Fl_Icon_Browser*)this)->cache = l;
  return l;
}

int Fl_Icon_Browser::lineno(void* v) const {
  FL_BLINE* l = (FL_BLINE*)v;
  if (!l) return 0;
  if (l == cache) return cacheline;
  if (l == first) return 1;
  if (l == last) return lines;
  if (!cache) {
    ((Fl_Icon_Browser*)this)->cache = first;
    ((Fl_Icon_Browser*)this)->cacheline = 1;
  }
  // assumme it is near cache, search both directions:
  FL_BLINE* b = cache->prev;
  int bnum = cacheline-1;
  FL_BLINE* f = cache->next;
  int fnum = cacheline+1;
  int n = 0;
  for (;;) {
    if (b == l) {n = bnum; break;}
    if (f == l) {n = fnum; break;}
    if (b) {b = b->prev; bnum--;}
    if (f) {f = f->next; fnum++;}
  }
  ((Fl_Icon_Browser*)this)->cache = l;
  ((Fl_Icon_Browser*)this)->cacheline = n;
  return n;
}

FL_BLINE* Fl_Icon_Browser::_remove(int line) {
  FL_BLINE* ttt = find_line(line);
  deleting(ttt);

  cacheline = line-1;
  cache = ttt->prev;
  lines--;
  full_height_ -= item_height(ttt);
  if (ttt->prev) ttt->prev->next = ttt->next;
  else first = ttt->next;
  if (ttt->next) ttt->next->prev = ttt->prev;
  else last = ttt->prev;

  return(ttt);
}

void Fl_Icon_Browser::remove(int line) {
  if (line < 1 || line > lines) return;
  free(_remove(line));
}

void Fl_Icon_Browser::insert(int line, FL_BLINE* t) {
  if (!first) {
    t->prev = t->next = 0;
    first = last = t;
  } else if (line <= 1) {
    inserting(first, t);
    t->prev = 0;
    t->next = first;
    t->next->prev = t;
    first = t;
  } else if (line > lines) {
    t->prev = last;
    t->prev->next = t;
    t->next = 0;
    last = t;
  } else {
    FL_BLINE* n = find_line(line);
    inserting(n, t);
    t->next = n;
    t->prev = n->prev;
    t->prev->next = t;
    n->prev = t;
  }
  cacheline = line;
  cache = t;
  lines++;
  full_height_ += item_height(t);
  redraw_line(t);
}

void Fl_Icon_Browser::insert(int line, const char* newtext, void* d) {
  int l = strlen(newtext);
  FL_BLINE* t = (FL_BLINE*)malloc(sizeof(FL_BLINE)+l);
  t->length = (short)l;
  t->flags = 0;
  strcpy(t->txt, newtext);
  t->data = d;
  t->icon = 0;
  t->level = 0;
  insert(line, t);
}

void Fl_Icon_Browser::move(int to, int from) {
  if (from < 1 || from > lines) return;
  insert(to, _remove(from));
}

void Fl_Icon_Browser::text(int line, const char* newtext) {
  if (line < 1 || line > lines) return;
  FL_BLINE* t = find_line(line);
  int l = strlen(newtext);
  if (l > t->length) {
    FL_BLINE* n = (FL_BLINE*)malloc(sizeof(FL_BLINE)+l);
    replacing(t, n);
    cache = n;
    n->data = t->data;
    n->length = (short)l;
    n->flags = t->flags;
    n->icon = t->icon;
    n->level = t->level;
    n->prev = t->prev;
    if (n->prev) n->prev->next = n; else first = n;
    n->next = t->next;
    if (n->next) n->next->prev = n; else last = n;
    free(t);
    t = n;
  }
  strcpy(t->txt, newtext);
  redraw_line(t);
}

void Fl_Icon_Browser::data(int line, void* d) {
  if (line < 1 || line > lines) return;
  find_line(line)->data = d;
}

int Fl_Icon_Browser::item_height(void* lv) const {
  FL_BLINE* l = (FL_BLINE*)lv;
  if (l->flags & NOTDISPLAYED) return 0;

  int hmax = 2; // use 2 to insure we don't return a zero!

  if (!l->txt[0]) {
    // For blank lines set the height to exactly 1 line!
    fl_font(textfont(), textsize());
    int hh = fl_height();
    if (hh > hmax) hmax = hh;
  }
  else {
    const int* i = column_widths();
    // do each column separately as they may all set different fonts:
    for (char* str = l->txt; str && *str; str++) {
      Fl_Font font = textfont(); // default font
      int tsize = textsize(); // default size
      while (*str==format_char()) {
	str++;
	switch (*str++) {
	case 'l': case 'L': tsize = 24; break;
	case 'm': case 'M': tsize = 18; break;
	case 's': tsize = 11; break;
	case 'b': font = (Fl_Font)(font|FL_BOLD); break;
	case 'i': font = (Fl_Font)(font|FL_ITALIC); break;
	case 'f': case 't': font = FL_COURIER; break;
	case 'B':
	case 'C': strtol(str, &str, 10); break;// skip a color number
	case 'F': font = (Fl_Font)strtol(str,&str,10); break;
	case 'S': tsize = strtol(str,&str,10); break;
	case 0: case '@': str--;
	case '.': goto END_FORMAT;
	}
      }
      END_FORMAT:
      char* ptr = str;
      if (ptr && *i++) str = strchr(str, column_char());
      else str = NULL;
      if((!str && *ptr) || (str && ptr < str)) {
	fl_font(font, tsize); int hh = fl_height();
	if (hh > hmax) hmax = hh;
      }
      if (!str || !*str) break;
    }
  }

  if (l->icon && (l->icon->h()+2)>hmax) hmax=l->icon->h()+2; // leave 2px above/below

  return hmax; // previous version returned hmax+2!
}

int Fl_Icon_Browser::item_width(void* v) const {
  char* str = ((FL_BLINE*)v)->txt;
  const int* i = column_widths();
  int ww = 0;

  while (*i) { // add up all tab-seperated fields
    char* e;
    e = strchr(str, column_char());
    if (!e) break; // last one occupied by text
    str = e+1;
    ww += *i++;
  }

  // OK, we gotta parse the string and find the string width...
  int tsize = textsize();
  Fl_Font font = textfont();
  int done = 0;

  while (*str == format_char_ && str[1] && str[1] != format_char_) {
    str ++;
    switch (*str++) {
    case 'l': case 'L': tsize = 24; break;
    case 'm': case 'M': tsize = 18; break;
    case 's': tsize = 11; break;
    case 'b': font = (Fl_Font)(font|FL_BOLD); break;
    case 'i': font = (Fl_Font)(font|FL_ITALIC); break;
    case 'f': case 't': font = FL_COURIER; break;
    case 'B':
    case 'C': strtol(str, &str, 10); break;// skip a color number
    case 'F': font = (Fl_Font)strtol(str, &str, 10); break;
    case 'S': tsize = strtol(str, &str, 10); break;
    case '.':
      done = 1;
      break;
    case '@':
      str--;
      done = 1;
    }

    if (done)
      break;
  }

  if (*str == format_char_ && str[1])
    str ++;

  FL_BLINE* blind = (FL_BLINE*)v;
  if (ww==0 && blind->icon) ww = blind->icon->w();

  fl_font(font, tsize);
  return ww + int(fl_width(str)) + 6;
}

int Fl_Icon_Browser::full_height() const {
  return full_height_;
}

int Fl_Icon_Browser::incr_height() const {
  return textsize()+2;
}

void Fl_Icon_Browser::item_draw(void* v, int X, int Y, int W, int H) const {
  char* str = ((FL_BLINE*)v)->txt;
  const int* i = column_widths();
  bool first=true; // for icon
  FL_BLINE*blind = (FL_BLINE*)v; 

  while (W > 6) {	// do each tab-seperated field
    int w1 = W;	// width for this field
    char* e = 0; // pointer to end of field or null if none
    if (*i) { // find end of field and temporarily replace with 0
      e = strchr(str, column_char());
      if (e) {*e = 0; w1 = *i++;}
    }


    // Tree drawing code
    // TODO: try to avoid using item_height...
    #define TREE_GLYPH_WIDTH 10
    // [+] and [-] box size
    #define TREE_BOX_SIZE 6 
    if (first && blind->level>0) {
       if (blind->flags & SELECTED)
          fl_color(fl_contrast(textcolor(), selection_color()));
       else
          fl_color(textcolor());
       fl_line_style(FL_DOT);
       int ih = item_height(blind);
       int hspace = (TREE_GLYPH_WIDTH-TREE_BOX_SIZE)/2;
       int vspace = (ih-TREE_BOX_SIZE)/2;

       // Draw vertical lines for parents
       for(int i=1; i<blind->level; i++) {
          // is there anything below to connect?
          FL_BLINE* k = blind->next;
          while (k && k->level > i)
             k = k->next;
          if (k && k->level==i) 
             fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+ih);
          X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       }

       if (blind->next && blind->next->level > blind->level) {
          // draw dotted lines before and after
          if (blind->prev)
             fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+vspace);
          // is there another item with same level?
          FL_BLINE* p = blind->next;
          bool collapsed=true;
          while (p && p->level > blind->level) {
             if (!(p->flags&NOTDISPLAYED)) collapsed=false;
             p = p->next;
          }
          if (p && p->level == blind->level)
             fl_line(X+TREE_GLYPH_WIDTH/2, Y+ih-vspace, X+TREE_GLYPH_WIDTH/2, Y+ih);

          // draw [+] glyph
          fl_color(fl_lighter(fl_color()));
          fl_line_style(0);
          fl_line(X+hspace, Y+vspace, X+TREE_GLYPH_WIDTH-hspace, Y+vspace);
          fl_line(X+TREE_GLYPH_WIDTH-hspace, Y+vspace, X+TREE_GLYPH_WIDTH-hspace, Y+ih-vspace-(ih%2));
          fl_line(X+TREE_GLYPH_WIDTH-hspace, Y+ih-vspace-(ih%2), X+hspace, Y+ih-vspace-(ih%2));
          fl_line(X+hspace, Y+ih-vspace-(ih%2), X+hspace, Y+vspace);

          fl_line(X+hspace+2, Y+ih/2, X+TREE_GLYPH_WIDTH-hspace-2, Y+ih/2); // minus
          if (collapsed)
            fl_line(X+TREE_GLYPH_WIDTH/2, Y+vspace+2, X+TREE_GLYPH_WIDTH/2, Y+ih-vspace-2-(ih%2)); // plus

          fl_color(fl_darker(fl_color()));
       } 
       else if (blind->next && blind->next->level == blind->level) {
          // draw |- glyph
          fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+ih);
          fl_line(X+TREE_GLYPH_WIDTH/2+2, Y+ih/2, X+TREE_GLYPH_WIDTH-1, Y+ih/2);
       }
       else  {
          // draw L glyph
          fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+ih/2);
          fl_line(X+TREE_GLYPH_WIDTH/2+2, Y+ih/2, X+TREE_GLYPH_WIDTH-1, Y+ih/2);
       } 
       X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       fl_line_style(0);
    }


	// NOTE: This version is drawing continuous lines, but the problem is,
	// if parent item is invisible (e.g. scrolled out) it's line isn't drawn :(
/*    #define TREE_GLYPH_WIDTH 10
    #define TREE_BOX_SIZE 6 // box size
    if (first && blind->level>0) {
       if (blind->flags & SELECTED)
          fl_color(fl_contrast(textcolor(), selection_color()));
       else
          fl_color(textcolor());
       fl_line_style(FL_DOT);
       int ih = item_height(blind);
       int hspace = (TREE_GLYPH_WIDTH-TREE_BOX_SIZE)/2;
       int vspace = (ih-TREE_BOX_SIZE)/2;

       for(int i=1; i<blind->level; i++) {
          // leave space for the vertical line
          X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       }

       if (blind->next && blind->next->level > blind->level) {

          // draw dotted lines before and after
          if (blind->prev)
             fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+vspace);
          int lineend = Y+ih;
          FL_BLINE* p = blind->next;
          bool collapsed=true;
          while (p && p->level > blind->level) {
             lineend += item_height(p);
             if (!(p->flags&NOTDISPLAYED)) collapsed=false;
             p = p->next;
          }
          if (p && p->level == blind->level)
             fl_line(X+TREE_GLYPH_WIDTH/2, Y+ih-vspace, X+TREE_GLYPH_WIDTH/2, lineend);

          // draw [+] glyph
          fl_color(fl_lighter(fl_color()));
          fl_line_style(0);
          fl_line(X+hspace, Y+vspace, X+TREE_GLYPH_WIDTH-hspace, Y+vspace);
          fl_line(X+TREE_GLYPH_WIDTH-hspace, Y+vspace, X+TREE_GLYPH_WIDTH-hspace, Y+ih-vspace-(ih%2));
          fl_line(X+TREE_GLYPH_WIDTH-hspace, Y+ih-vspace-(ih%2), X+hspace, Y+ih-vspace-(ih%2));
          fl_line(X+hspace, Y+ih-vspace-(ih%2), X+hspace, Y+vspace);

          fl_line(X+hspace+2, Y+ih/2, X+TREE_GLYPH_WIDTH-hspace-2, Y+ih/2); // minus
          if (collapsed)
            fl_line(X+TREE_GLYPH_WIDTH/2, Y+vspace+2, X+TREE_GLYPH_WIDTH/2, Y+ih-vspace-2-(ih%2)); // plus

          fl_color(fl_darker(fl_color()));
          fl_line_style(FL_DOT);
          X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       } 
       else if (blind->next && blind->next->level == blind->level) {
          // draw |- glyph
          int lineend = Y+ih;
          FL_BLINE* p = blind->next;
          FL_BLINE* q = blind;
          while (p && p->level == blind->level) {
             lineend += item_height(p);
             q = p;
             p = p->next;
          }
          lineend -= item_height(q);
          fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, lineend);
          fl_line(X+TREE_GLYPH_WIDTH/2+2, Y+ih/2, X+TREE_GLYPH_WIDTH-1, Y+ih/2);
          X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       }
       else  {
          // draw L glyph
          fl_line(X+TREE_GLYPH_WIDTH/2, Y, X+TREE_GLYPH_WIDTH/2, Y+ih/2);
          fl_line(X+TREE_GLYPH_WIDTH/2+2, Y+ih/2, X+TREE_GLYPH_WIDTH-1, Y+ih/2);
          X += TREE_GLYPH_WIDTH; W -= TREE_GLYPH_WIDTH; w1 -= TREE_GLYPH_WIDTH;
       } 
       fl_line_style(0);
    }*/

    // Icon drawing code
    if (first) {
       first=false;
       if (blind->icon != 0) {
          blind->icon->draw(X+2,Y+1); // leave 2px left, 1px above
          int iconw = blind->icon->w() + 2; 
          X += iconw; W -= iconw; w1 -= iconw;
       }
    }

    int tsize = textsize();
    Fl_Font font = textfont();
    Fl_Color lcol = textcolor();
    Fl_Align talign = FL_ALIGN_LEFT;
    // check for all the @-lines recognized by XForms:
    while (*str == format_char() && *++str && *str != format_char()) {
      switch (*str++) {
      case 'l': case 'L': tsize = 24; break;
      case 'm': case 'M': tsize = 18; break;
      case 's': tsize = 11; break;
      case 'b': font = (Fl_Font)(font|FL_BOLD); break;
      case 'i': font = (Fl_Font)(font|FL_ITALIC); break;
      case 'f': case 't': font = FL_COURIER; break;
      case 'c': talign = FL_ALIGN_CENTER; break;
      case 'r': talign = FL_ALIGN_RIGHT; break;
      case 'B': 
	if (!(((FL_BLINE*)v)->flags & SELECTED)) {
	  fl_color((Fl_Color)strtol(str, &str, 10));
	  fl_rectf(X, Y, w1, H);
	} else strtol(str, &str, 10);
        break;
      case 'C':
	lcol = (Fl_Color)strtol(str, &str, 10);
	break;
      case 'F':
	font = (Fl_Font)strtol(str, &str, 10);
	break;
      case 'N':
	lcol = FL_INACTIVE_COLOR;
	break;
      case 'S':
	tsize = strtol(str, &str, 10);
	break;
      case '-':
	fl_color(FL_DARK3);
	fl_line(X+3, Y+H/2, X+w1-3, Y+H/2);
	fl_color(FL_LIGHT3);
	fl_line(X+3, Y+H/2+1, X+w1-3, Y+H/2+1);
	break;
      case 'u':
      case '_':
	fl_color(lcol);
	fl_line(X+3, Y+H-1, X+w1-3, Y+H-1);
	break;
      case '.':
	goto BREAK;
      case '@':
	str--; goto BREAK;
      }
    }
  BREAK:
    fl_font(font, tsize);
    if (((FL_BLINE*)v)->flags & SELECTED)
      lcol = fl_contrast(lcol, selection_color());
    if (!active_r()) lcol = fl_inactive(lcol);
    fl_color(lcol);
    fl_draw(str, X+3, Y, w1-6, H, e ? Fl_Align(talign|FL_ALIGN_CLIP) : talign, 0, 0);
    if (!e) break; // no more fields...
    *e = column_char(); // put the seperator back
    X += w1;
    W -= w1;
    str = e+1;
  }
}

static const int no_columns[1] = {0};

Fl_Icon_Browser::Fl_Icon_Browser(int X, int Y, int W, int H, const char*l)
  : Fl_Browser_(X, Y, W, H, l) {
  column_widths_ = no_columns;
  lines = 0;
  full_height_ = 0;
  cacheline = 0;
  format_char_ = '@';
  column_char_ = '\t';
  first = last = cache = 0;
}

void Fl_Icon_Browser::lineposition(int line, Fl_Line_Position pos) {
  if (line<1) line = 1;
  if (line>lines) line = lines;
  int p = 0;

  FL_BLINE* l;
  for (l=first; l && line>1; l = l->next) {
    line--; p += item_height(l);
  }
  if (l && (pos == BOTTOM)) p += item_height (l);

  int final = p, X, Y, W, H;
  bbox(X, Y, W, H);

  switch(pos) {
    case TOP: break;
    case BOTTOM: final -= H; break;
    case MIDDLE: final -= H/2; break;
  }
  
  if (final > (full_height() - H)) final = full_height() -H;
  position(final);
}

int Fl_Icon_Browser::topline() const {
  return lineno(top());
}

void Fl_Icon_Browser::clear() {
  for (FL_BLINE* l = first; l;) {
    FL_BLINE* n = l->next;
    free(l);
    l = n;
  }
  full_height_ = 0;
  first = 0;
  lines = 0;
  new_list();
}

void Fl_Icon_Browser::add(const char* newtext, void* d) {
  insert(lines+1, newtext, d);
  //Fl_Icon_Browser_::display(last);
}

const char* Fl_Icon_Browser::text(int line) const {
  if (line < 1 || line > lines) return 0;
  return find_line(line)->txt;
}

void* Fl_Icon_Browser::data(int line) const {
  if (line < 1 || line > lines) return 0;
  return find_line(line)->data;
}

int Fl_Icon_Browser::select(int line, int v) {
  if (line < 1 || line > lines) return 0;
  return Fl_Browser_::select(find_line(line), v);
}

int Fl_Icon_Browser::selected(int line) const {
  if (line < 1 || line > lines) return 0;
  return find_line(line)->flags & SELECTED;
}

void Fl_Icon_Browser::show(int line) {
  FL_BLINE* t = find_line(line);
  if (t->flags & NOTDISPLAYED) {
    t->flags &= ~NOTDISPLAYED;
    full_height_ += item_height(t);
    if (Fl_Browser_::displayed(t)) redraw_lines();
  }
}

void Fl_Icon_Browser::hide(int line) {
  FL_BLINE* t = find_line(line);
  if (!(t->flags & NOTDISPLAYED)) {
    full_height_ -= item_height(t);
    t->flags |= NOTDISPLAYED;
    if (Fl_Browser_::displayed(t)) redraw_lines();
  }
}

void Fl_Icon_Browser::display(int line, int v) {
  if (line < 1 || line > lines) return;
  if (v) show(line); else hide(line);
}

int Fl_Icon_Browser::visible(int line) const {
  if (line < 1 || line > lines) return 0;
  return !(find_line(line)->flags&NOTDISPLAYED);
}

int Fl_Icon_Browser::value() const {
  return lineno(selection());
}

// SWAP TWO LINES
void Fl_Icon_Browser::swap(FL_BLINE *a, FL_BLINE *b) {

  if ( a == b || !a || !b) return;          // nothing to do
  swapping(a, b);
  FL_BLINE *aprev  = a->prev;
  FL_BLINE *anext  = a->next;
  FL_BLINE *bprev  = b->prev;
  FL_BLINE *bnext  = b->next;
  if ( b->prev == a ) { 		// A ADJACENT TO B
     if ( aprev ) aprev->next = b; else first = b;
     b->next = a;
     a->next = bnext;
     b->prev = aprev;
     a->prev = b;
     if ( bnext ) bnext->prev = a; else last = a;
  } else if ( a->prev == b ) {		// B ADJACENT TO A
     if ( bprev ) bprev->next = a; else first = a;
     a->next = b;
     b->next = anext;
     a->prev = bprev;
     b->prev = a;
     if ( anext ) anext->prev = b; else last = b;
  } else {				// A AND B NOT ADJACENT
     // handle prev's
     b->prev = aprev;
     if ( anext ) anext->prev = b; else last = b;
     a->prev = bprev;
     if ( bnext ) bnext->prev = a; else last = a;
     // handle next's
     if ( aprev ) aprev->next = b; else first = b;
     b->next = anext;
     if ( bprev ) bprev->next = a; else first = a;
     a->next = bnext;
  }
  // Disable cache -- we played around with positions
  cacheline = 0;
  cache = 0;
}

void Fl_Icon_Browser::swap(int ai, int bi) {
  if (ai < 1 || ai > lines || bi < 1 || bi > lines) return;
  FL_BLINE* a = find_line(ai);
  FL_BLINE* b = find_line(bi);
  swap(a,b);
}

void Fl_Icon_Browser::set_icon(int line, Fl_Image* icon) {
	if (icon && line>0 && line<=lines) {
		// update full_height_
		FL_BLINE* bl = find_line(line);
		int dh = icon->h() - item_height(bl) + 2; // leave 2px above/below
		if (dh>0) full_height_ += dh; 
		bl->icon=icon;
		redraw_line(bl); 
	} 
}
void Fl_Icon_Browser::remove_icon(int line) { 
	if (line>0 && line<=lines) {
		FL_BLINE* bl = find_line(line);
		if (!bl->icon) return;
		int dh = bl->icon->h()+2; // leave 2px above/below
		bl->icon=0; 
		redraw_line(bl); 
		// update_full_height_
		dh -= item_height(bl);
		if (dh>0) full_height_ -= dh; 
	}
}

Fl_Image* Fl_Icon_Browser::get_icon(int line) { 
	if (line < 1 || line > lines) return 0;
	return find_line(line)->icon; 
}


// Tree support

void Fl_Icon_Browser::indent(int line, int level) {
	if (line < 1 || line > lines) return;
	if (level<0) return;
	find_line(line)->level = level;
}

int Fl_Icon_Browser::indent(int line) {
	if (line < 1 || line > lines) return 0;
	return find_line(line)->level;
}

void Fl_Icon_Browser::collapse(void* l) {
fprintf(stderr, "collapse(l)\n");
	FL_BLINE* ll = (FL_BLINE*)l;
	FL_BLINE* p = ll->next;
	while (p && p->level > ll->level) {
		if (!(p->flags & NOTDISPLAYED)) {
			full_height_ -= item_height(p);
			p->flags |= NOTDISPLAYED;
		}
		p = p->next;
	}
}

void Fl_Icon_Browser::expand(void* l) {
fprintf(stderr, "expand(l)\n");
	FL_BLINE* ll = (FL_BLINE*)l;
	FL_BLINE* p = ll->prev;
	// if invisible, expand parent
	if (ll->flags & NOTDISPLAYED) {
		while (p && p->level != ll->level-1)
			p = p->prev;
		if (p) expand(p); // if not found, do nothing
	}
	p = ll->next;
	while (p && p->level > ll->level) {
		// expand only the level below!
		if ((p->flags & NOTDISPLAYED) && (p->level==ll->level+1)) {
			p->flags &= ~NOTDISPLAYED;
			full_height_ += item_height(p);
		}
		p = p->next;
	}
}


void Fl_Icon_Browser::collapse(int line) {
	if (line < 1 || line > lines) return;
	collapse(find_line(line));
	redraw_lines();
}

void Fl_Icon_Browser::expand(int line) {
	if (line < 1 || line > lines) return;
	expand(find_line(line));
	redraw();
}

void Fl_Icon_Browser::collapse_all() {
	FL_BLINE* l = first;
	if (!l) return;
	do {
		collapse(l);
	} while (l=l->next);
	redraw_lines();
}

void Fl_Icon_Browser::expand_all() {
	FL_BLINE* l = first;
	if (!l) return;
	do {
		collapse(l);
	} while (l=l->next);
	redraw();
}

void Fl_Icon_Browser::toggle_collapse(int line) {
	if (line < 1 || line > lines) return;
	toggle_collapse(find_line(line));
	redraw();
}

void Fl_Icon_Browser::toggle_collapse(void* lp) {
	FL_BLINE* l = (FL_BLINE*)lp;
	if (!l->next || l->next->level <= l->level) return;

	bool invisibles=false;
	FL_BLINE* p = l->next;
	do {
		// expand only one level below!
		if (p->level <=l->level) break;
		if ((p->flags & NOTDISPLAYED) && (p->level==l->level+1))
			invisibles=true;
	} while (p=p->next);

	if (invisibles)
		expand(l);
	else
		collapse(l);
}

int Fl_Icon_Browser::handle(int e) {
	if (e==FL_PUSH) {
		FL_BLINE* l = first;
		int itemy=y()-position();
		while (l) {
			itemy+=item_height(l);
			if (itemy>Fl::event_y()) break;
			l = l->next;
		}
		if (l && (l->level > 0) && (Fl::event_x()>x()+(l->level-1)*TREE_GLYPH_WIDTH) && (Fl::event_x()<x()+l->level*TREE_GLYPH_WIDTH)) {
			toggle_collapse(l);
			redraw();
			Fl::event_is_click(0); // prevent the next click from becoming callback
			return 1;
		}
	}
	return Fl_Browser_::handle(e);
}



//
// End of "$Id: Fl_Icon_Browser.cxx 5190 2006-06-09 16:16:34Z mike $".
//
