/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Round_Button.h>
#include <FL/Fl_RGB_Image.h>
#include <FL/Fl.h>
#include <FL/x.h>
#include <string.h> // memset

#include <edelib/Nls.h>
#include "Logout.h"

static int logout_ret;
static Fl_Double_Window* win;
static Fl_Round_Button*  rb1;
static Fl_Round_Button*  rb2;
static Fl_Round_Button*  rb3;

unsigned char* take_x11_screenshot(unsigned char *p, int X, int Y, int w, int h, int alpha);
unsigned char* make_darker(unsigned char *p, int X, int Y, int w, int h);

static void rb_cb(Fl_Widget*, void* r) {
	Fl_Round_Button* rb = (Fl_Round_Button*)r;
	if(rb == rb2) {
		rb1->value(0);
		rb3->value(0);
	} else if(rb == rb3) {
		rb1->value(0);
		rb2->value(0);
	} else {
		rb2->value(0);
		rb3->value(0);
	}

	rb->value(1);
}

static void ok_cb(Fl_Widget*, void*) {
	if(rb1->value())
		logout_ret = LOGOUT_LOGOUT;
	else if(rb2->value())
		logout_ret = LOGOUT_RESTART;
	else
		logout_ret = LOGOUT_SHUTDOWN;
	win->hide();
}

static void cancel_cb(Fl_Widget*, void*) {
	logout_ret = LOGOUT_CANCEL;
	win->hide();
}

int logout_dialog(int screen_w, int screen_h, bool disable_restart, bool disable_shutdown) {
	logout_ret = 0;
	unsigned char* imgdata = NULL;
	imgdata = take_x11_screenshot(imgdata, 0, 0, screen_w, screen_h, 0);
	if(imgdata)
		imgdata = make_darker(imgdata, 0, 0, screen_w, screen_h);

	//win = new Fl_Double_Window(365, 265, 325, 185, _("Logout, restart or shutdown"));
	win = new Fl_Double_Window(0, 0, screen_w, screen_h, _("Logout, restart or shutdown"));
	win->begin();
		Fl_Box* bb = new Fl_Box(0, 0, win->w(), win->h());
		Fl_RGB_Image* img = new Fl_RGB_Image(imgdata, win->w(), win->h());
		img->alloc_array = 1;
		bb->image(img);

		//Fl_Group* g = new Fl_Group(365, 265, 325, 185);
		Fl_Group* g = new Fl_Group(0, 0, 325, 185);
		g->box(FL_THIN_UP_BOX);
		g->begin();
			Fl_Box* b = new Fl_Box(10, 9, 305, 39, _("Logout, restart or shut down the computer ?"));
			b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
			b->labelfont(FL_HELVETICA_BOLD);

			rb1 = new Fl_Round_Button(25, 60, 275, 20, _("Logout from the current session"));
			rb1->down_box(FL_ROUND_DOWN_BOX);
			rb1->value(1);
			rb1->callback(rb_cb, rb1);

			rb2 = new Fl_Round_Button(25, 85, 275, 20, _("Restart the computer"));
			rb2->down_box(FL_ROUND_DOWN_BOX);
			rb2->value(0);
			rb2->callback(rb_cb, rb2);
			if(disable_restart)
				rb2->deactivate();

			rb3 = new Fl_Round_Button(25, 110, 275, 20, _("Shut down the computer"));
			rb3->down_box(FL_ROUND_DOWN_BOX);
			rb3->value(0);
			rb3->callback(rb_cb, rb3);
			if(disable_shutdown)
				rb3->deactivate();

			Fl_Button* ok = new Fl_Button(130, 150, 90, 25, _("&OK"));
			ok->callback(ok_cb);

			Fl_Button* cancel = new Fl_Button(225, 150, 90, 25, _("&Cancel"));
			cancel->callback(cancel_cb);
		g->end();


	g->position(screen_w/2 - g->w()/2, screen_h/2 - g->h()/2);
	//win->position(screen_w/2 - win->w()/2, screen_h/2 - win->h()/2);

	win->end();
	win->clear_border();
	win->set_override();
	win->show();

	while(win->shown())
		Fl::wait();

	return logout_ret;
}

unsigned char* make_darker(unsigned char *p, int X, int Y, int w, int h) {
	if(!p)
		return 0;
	unsigned char* pdata = p;
	int step = 100;

	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			// red
			if(*pdata > step)
				*pdata -= step;
			else
				*pdata = 0;
			pdata++;

			// green
			if(*pdata > step)
				*pdata -= step;
			else
				*pdata = 0;
			pdata++;

			// blue
			if(*pdata > step)
				*pdata -= step;
			else
				*pdata = 0;
			pdata++;
		}
	}

	return p;
}

// stolen from fl_read_image.cxx
unsigned char* take_x11_screenshot(unsigned char *p, int X, int Y, int w, int h, int alpha) {
  XImage	*image;
  int		i, maxindex;
  int           x, y;		// Current X & Y in image
  int		d;		// Depth of image
  unsigned char *line,		// Array to hold image row
		*line_ptr;	// Pointer to current line image
  unsigned char	*pixel;		// Current color value
  XColor	colors[4096];	// Colors from the colormap...
  unsigned char	cvals[4096][3];	// Color values from the colormap...
  unsigned	index_mask,
		index_shift,
		red_mask,
		red_shift,
		green_mask,
		green_shift,
		blue_mask,
		blue_shift;


  //
  // Under X11 we have the option of the XGetImage() interface or SGI's
  // ReadDisplay extension which does all of the really hard work for
  // us...
  //

  image = 0;

  if (!image) {
    image = XGetImage(fl_display, RootWindow(fl_display, fl_screen), X, Y, w, h, AllPlanes, ZPixmap);
  }

  if (!image) return 0;

  d = alpha ? 4 : 3;

  // Allocate the image data array as needed...
  if (!p) p = new unsigned char[w * h * d];

  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);

  // Check that we have valid mask/shift values...
  if (!image->red_mask && image->bits_per_pixel > 12) {
    // Greater than 12 bits must be TrueColor...
    image->red_mask   = fl_visual->visual->red_mask;
    image->green_mask = fl_visual->visual->green_mask;
    image->blue_mask  = fl_visual->visual->blue_mask;
  }

  // Check if we have colormap image...
  if (!image->red_mask) {
    // Get the colormap entries for this window...
    maxindex = fl_visual->visual->map_entries;

    for (i = 0; i < maxindex; i ++) colors[i].pixel = i;

    XQueryColors(fl_display, fl_colormap, colors, maxindex);

    for (i = 0; i < maxindex; i ++) {
      cvals[i][0] = colors[i].red >> 8;
      cvals[i][1] = colors[i].green >> 8;
      cvals[i][2] = colors[i].blue >> 8;
    }

    // Read the pixels and output an RGB image...
    for (y = 0; y < image->height; y ++) {
      pixel = (unsigned char *)(image->data + y * image->bytes_per_line);
      line  = p + y * w * d;

      switch (image->bits_per_pixel) {
        case 1 :
	  for (x = image->width, line_ptr = line, index_mask = 128;
	       x > 0;
	       x --, line_ptr += d) {
	    if (*pixel & index_mask) {
	      line_ptr[0] = cvals[1][0];
	      line_ptr[1] = cvals[1][1];
	      line_ptr[2] = cvals[1][2];
            } else {
	      line_ptr[0] = cvals[0][0];
	      line_ptr[1] = cvals[0][1];
	      line_ptr[2] = cvals[0][2];
            }

            if (index_mask > 1) {
	      index_mask >>= 1;
	    } else {
              index_mask = 128;
              pixel ++;
            }
	  }
          break;

        case 2 :
	  for (x = image->width, line_ptr = line, index_shift = 6;
	       x > 0;
	       x --, line_ptr += d) {
	    i = (*pixel >> index_shift) & 3;

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift > 0) {
              index_mask >>= 2;
              index_shift -= 2;
            } else {
              index_mask  = 192;
              index_shift = 6;
              pixel ++;
            }
	  }
          break;

        case 4 :
	  for (x = image->width, line_ptr = line, index_shift = 4;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 4) i = (*pixel >> 4) & 15;
	    else i = *pixel & 15;

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift > 0) {
              index_shift = 0;
	    } else {
              index_shift = 4;
              pixel ++;
            }
	  }
          break;

        case 8 :
	  for (x = image->width, line_ptr = line;
	       x > 0;
	       x --, line_ptr += d, pixel ++) {
	    line_ptr[0] = cvals[*pixel][0];
	    line_ptr[1] = cvals[*pixel][1];
	    line_ptr[2] = cvals[*pixel][2];
	  }
          break;

        case 12 :
	  for (x = image->width, line_ptr = line, index_shift = 0;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 0) {
	      i = ((pixel[0] << 4) | (pixel[1] >> 4)) & 4095;
	    } else {
	      i = ((pixel[1] << 8) | pixel[2]) & 4095;
	    }

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift == 0) {
              index_shift = 4;
            } else {
              index_shift = 0;
              pixel += 3;
            }
	  }
          break;
      }
    }
  } else {
    // RGB(A) image, so figure out the shifts & masks...
    red_mask  = image->red_mask;
    red_shift = 0;

    while ((red_mask & 1) == 0) {
      red_mask >>= 1;
      red_shift ++;
    }

    green_mask  = image->green_mask;
    green_shift = 0;

    while ((green_mask & 1) == 0) {
      green_mask >>= 1;
      green_shift ++;
    }

    blue_mask  = image->blue_mask;
    blue_shift = 0;

    while ((blue_mask & 1) == 0) {
      blue_mask >>= 1;
      blue_shift ++;
    }

    // Read the pixels and output an RGB image...
    for (y = 0; y < image->height; y ++) {
      pixel = (unsigned char *)(image->data + y * image->bytes_per_line);
      line  = p + y * w * d;

      switch (image->bits_per_pixel) {
        case 8 :
	  for (x = image->width, line_ptr = line;
	       x > 0;
	       x --, line_ptr += d, pixel ++) {
	    i = *pixel;

	    line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	    line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	    line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	  }
          break;

        case 12 :
	  for (x = image->width, line_ptr = line, index_shift = 0;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 0) {
	      i = ((pixel[0] << 4) | (pixel[1] >> 4)) & 4095;
	    } else {
	      i = ((pixel[1] << 8) | pixel[2]) & 4095;
            }

	    line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	    line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	    line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;

            if (index_shift == 0) {
              index_shift = 4;
            } else {
              index_shift = 0;
              pixel += 3;
            }
	  }
          break;

        case 16 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 2) {
	      i = (pixel[1] << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 2) {
	      i = (pixel[0] << 8) | pixel[1];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;

        case 24 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 3) {
	      i = (((pixel[2] << 8) | pixel[1]) << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 3) {
	      i = (((pixel[0] << 8) | pixel[1]) << 8) | pixel[2];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;

        case 32 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 4) {
	      i = (((((pixel[3] << 8) | pixel[2]) << 8) | pixel[1]) << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 4) {
	      i = (((((pixel[0] << 8) | pixel[1]) << 8) | pixel[2]) << 8) | pixel[3];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;
      }
    }
  }

  // Destroy the X image we've read and return the RGB(A) image...
  XDestroyImage(image);

  return p;
}
