/*
 * $Id$
 *
 * Copyright (C) 2006-2013 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>

#include "Wallpaper.h"

#define CALC_PIXEL(tmp, rshift, rmask, gshift, gmask, bshift, bmask)	\
	tmp = 0;															\
	if(rshift >= 0)														\
		tmp |= (((int)r << rshift) & rmask);							\
	else																\
		tmp |= (((int)r >> (-rshift)) & rmask);							\
																		\
	if(gshift >= 0)														\
		tmp |= (((int)g << gshift) & gmask);							\
	else																\
		tmp |= (((int)g >> (-gshift)) & gmask);							\
																		\
	if(bshift >= 0)														\
		tmp |= (((int)b << bshift) & bmask);							\
	else																\
		tmp |= (((int)b >> (-bshift)) & bmask)


static Pixmap create_xpixmap(Fl_Image* img, XImage** xim, Pixmap pix, int wp_w, int wp_h) {
	if(!img) return 0;
	if(pix) XFreePixmap(fl_display, pix);

	unsigned long rmask = fl_visual->visual->red_mask;
	unsigned long gmask = fl_visual->visual->green_mask;
	unsigned long bmask = fl_visual->visual->blue_mask;
	unsigned long start_mask; 
	int           start_shift;
	int           rshift = 0;
	int           gshift = 0;
	int           bshift = 0;

	if(fl_visual->depth == 24 || fl_visual->depth == 16) {
		unsigned long n;
		if(fl_visual->depth == 24) {
			start_shift = 24;
			start_mask  = 0x80000000;
		} else {
			start_shift = 8;
			start_mask  = 0x8000;
		}

		rshift = start_shift;
		n = start_mask;
		while(!(n & rmask)) {
			n >>= 1;
			rshift--;
		}

		gshift = start_shift;
		n = start_mask;
		while(!(n & gmask)) {
			n >>= 1;
			gshift--;
		}

		bshift = start_shift;
		n = start_mask;
		while(!(n & bmask)) {
			n >>= 1;
			bshift--;
		}
	}

	/*
	 * Figure out bitmap_pad and create image coresponding to the current 
	 * display depth except for 8 bpp display
	 */
	int bitmap_pad = 0;
	if(fl_visual->depth > 16)
		bitmap_pad = 32;
	else if(fl_visual->depth > 8)
		bitmap_pad = 16;
	else {
		E_WARNING(E_STRLOC ": Visual %i not supported\n", fl_visual->depth);
		return 0;
	}

	*xim = XCreateImage(fl_display, fl_visual->visual, fl_visual->depth, ZPixmap, 0, 0, img->w(), img->h(), bitmap_pad, 0);

	int iw = img->w();
	int ih = img->h();
	int id = img->d();
	bool msb = (ImageByteOrder(fl_display) == MSBFirst) ? true : false;

	unsigned int r, g, b, tmp;
	unsigned char* dest = (unsigned char*)malloc(ih * (*xim)->bytes_per_line);
	unsigned char* destptr = dest;
	unsigned char* src = (unsigned char*)img->data()[0];

	if((*xim)->bits_per_pixel == 32) {
		if(id == 3 || id == 4) {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src++;
					g = *src++;
					b = *src++;

					if(id == 4)
						src++;

					CALC_PIXEL(tmp, rshift, rmask, gshift, gmask, bshift, bmask);

					if(msb) {
						*destptr++ = (tmp & 0xff000000) >> 24;
						*destptr++ = (tmp & 0xff0000) >> 16;
						*destptr++ = (tmp & 0xff00) >> 8;
						*destptr++ = (tmp & 0xff);
					} else {
						*destptr++ = (tmp & 0xff);
						*destptr++ = (tmp & 0xff00) >> 8;
						*destptr++ = (tmp & 0xff0000) >> 16;
						*destptr++ = (tmp & 0xff000000) >> 24;
					}
				}
			}
		} else {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src++;					
					g = *src++;					
					b = *src++;					

					if(msb) {
						*destptr++ = 0;
						*destptr++ = b;
						*destptr++ = g;
						*destptr++ = r;
					} else {
						*destptr++ = r;
						*destptr++ = g;
						*destptr++ = b;
						*destptr++ = 0;
					}
				}
			}
		}
	} else if((*xim)->bits_per_pixel == 24) {
		if(id == 3 || id == 4) {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src++;
					g = *src++;
					b = *src++;

					if(id == 4)
						src++;

					CALC_PIXEL(tmp, rshift, rmask, gshift, gmask, bshift, bmask);

					if(msb) {
						*destptr++ = (tmp & 0xff0000) >> 16;
						*destptr++ = (tmp & 0xff00) >> 8;
						*destptr++ = (tmp & 0xff);

					} else {
						*destptr++ = (tmp & 0xff);
						*destptr++ = (tmp & 0xff00) >> 8;
						*destptr++ = (tmp & 0xff0000) >> 16;
					}
				}
			}
		} else {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src++;
					g = *src++;
					b = *src++;

					if(msb) {
						*destptr++ = b;
						*destptr++ = g;
						*destptr++ = r;
					} else {
						*destptr++ = r;
						*destptr++ = g;
						*destptr++ = b;
					}
				}
			}
		} 
	} else if((*xim)->bits_per_pixel == 16) {
		if(id == 3 || id == 4) {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src++;
					g = *src++;
					b = *src++;

					if(id == 4)
						src++;

					CALC_PIXEL(tmp, rshift, rmask, gshift, gmask, bshift, bmask);

					if(msb) {
						*destptr++ = (tmp >> 8) & 0xff;
						*destptr++ = (tmp & 0xff);
					} else {
						*destptr++ = (tmp & 0xff);
						*destptr++ = (tmp >> 8) & 0xff;
					}
				}
			}
		} else {
			for(int j = 0; j < ih; j++) {
				for(int i = 0; i < iw; i++) {
					r = *src >> 3; src++;
					g = *src >> 2; src++;
					b = *src >> 3; src++;

					*destptr++ = r << 11 | g << 5 | b;
				}
			}
		}
	}

	(*xim)->data = (char*)dest;

	/*
	 * Creating another window as drawable is needed since fl_window (as drawable) can't be
	 * used here (valid only in draw()). Drawable must be size as wallpaper area or clients who
	 * query _XA_XROOTPMAP_ID will get BadWindow when goes out of drawable area (but not out of
	 * wallpaper area).
	 *
	 * FIXME: drawable background should be the same color as wallpaper background
	 */
	Window drawable = XCreateSimpleWindow(fl_display, RootWindow(fl_display, fl_screen), 0, 0, wp_w, wp_h,
										  0, 0, BlackPixel(fl_display, fl_screen));

	pix = XCreatePixmap(fl_display, drawable, wp_w, wp_h, fl_visual->depth);

	/* The same applies as above; fl_gc can't be used here */
	XGCValues gcv;
	gcv.graphics_exposures = False;
	GC dgc = XCreateGC(fl_display, pix, GCGraphicsExposures, &gcv);

	XPutImage(fl_display, pix, dgc, *xim, 0, 0, 0, 0, iw, ih);

	XDestroyWindow(fl_display, drawable);
	XFreeGC(fl_display, dgc);

	return pix;
}

#define PIXEL_POS(x, y, w, d) ((((y) * (w)) + (x)) * (d))

static void create_tile(Fl_Image* orig, Fl_RGB_Image** copied, int X, int Y, int W, int H) {
	/* don't tile large image */
	if(orig->w() >= W && orig->h() >= H) {
		*copied = (Fl_RGB_Image*) orig;
		return;
	}

	int iw = orig->w();
	int ih = orig->h();
	int idepth = orig->d();
	int tx = X - (X % iw);
	int ty = Y - (Y % ih);
	int tw = W + tx;
	int th = H + ty;

	unsigned char* dest = new unsigned char[tw * th * orig->d()];
	unsigned char* destptr = dest;
	unsigned char* src = (unsigned char*)orig->data()[0];
	int ppos = 0;
	/* for bounds checks */
	int imax = iw * ih * idepth;

	if(idepth == 3 || idepth == 4) {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
				*destptr++ = src[ppos + 1];
				*destptr++ = src[ppos + 2];

				if(idepth == 4)
					*destptr++ = src[ppos + 3];
			}
		}
	} else if(idepth == 2) {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
				*destptr++ = src[ppos + 1];
			}
		}
	} else {
		for(int j = 0, cj = 0; j < th; j++, cj++) {
			if(cj > ih) cj = 0;

			for(int i = 0, ci = 0; i < tw; i++, ci++) {
				if(ci > iw) ci = 0;
				ppos = PIXEL_POS(ci, cj, iw, idepth);
				if(ppos > imax) ppos = imax;

				*destptr++ = src[ppos];
			}
		}
	}

	Fl_RGB_Image* c = new Fl_RGB_Image(dest, tw, th, idepth, orig->ld());
	c->alloc_array = 1;
	*copied = c;
}

Wallpaper::~Wallpaper() { 
	if(rootpmap_pixmap) XFreePixmap(fl_display, rootpmap_pixmap);
	delete stretched_alloc;
}

void Wallpaper::set_rootpmap(void) {
	E_RETURN_IF_FAIL(image() != NULL);

	XImage *rootpmap_image = 0;
	Atom    _XA_XROOTPMAP_ID;
	
	rootpmap_pixmap = create_xpixmap(image(), &rootpmap_image, rootpmap_pixmap, w(), h());
	E_RETURN_IF_FAIL(rootpmap_pixmap != 0);

	/* XDestroyImage function calls frees both the image structure and the data pointed to by the image structure */
	if(rootpmap_image)
		XDestroyImage(rootpmap_image);

	_XA_XROOTPMAP_ID = XInternAtom(fl_display, "_XROOTPMAP_ID", False);

	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen), 
					_XA_XROOTPMAP_ID, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&rootpmap_pixmap, 1);	
}

bool Wallpaper::load(const char *path, int s, bool rootpmap) {
	E_ASSERT(path != NULL);
	
	/* in case this function gets multiple calls */
	if(wpath == path && state == s && rootpmap == use_rootpmap)
		return true;

	Fl_Shared_Image *img = Fl_Shared_Image::get(path);
	E_RETURN_VAL_IF_FAIL(img != NULL, false);
	
	if(s != WALLPAPER_CENTER && s != WALLPAPER_TILE && s != WALLPAPER_STRETCH)
		s = WALLPAPER_CENTER;
	
	if(s == WALLPAPER_TILE) {
		Fl_RGB_Image *tiled;

		create_tile((Fl_Image*)img, &tiled, x(), y(), w(), h());
		image(tiled);
	} else if(s == WALLPAPER_STRETCH) {
		Fl_Image *stretched = NULL;

		if(img->w() == w() && img->h() == h())
			stretched = img;
		else {
			/* valgrind reports it as possible lost, but FLTK should free it */
			delete stretched_alloc;

			stretched = img->copy(w(), h());
			img->release();
			stretched_alloc = stretched;
		}

		image(stretched);
	} else {
		image(img);
	}

	/* set root pixmap for pseudo transparency */
	if(rootpmap) set_rootpmap();
	
	state = s;
	use_rootpmap = rootpmap;
	/* prevent self assignment or bad things will happen */
	if(wpath != path) wpath = path;

	return true;
}

void Wallpaper::draw(void) {
	E_RETURN_IF_FAIL(image() != NULL);

	int ix, iy, iw, ih;
	Fl_Image* im = image();

	iw = im->w();
	ih = im->h();

	E_RETURN_IF_FAIL(iw > 0 && ih > 0);

	if(state == WALLPAPER_CENTER) {
		ix = (w()/2) - (iw/2);
		iy = (h()/2) - (ih/2);
		ix += x();
		iy += y();
	} else {
		ix = x();
		iy = y();
	}

	im->draw(ix, iy);

#if 0
	/*
	 * For debugging purposes :)
	 * Uncommenting this (and removing GC/Window creation in create_xpixmap will draw _XA_XROOTPMAP_ID 
	 * Pixmap directly in Wallpaper widget. Used to check Fl_Image->Image conversion.
	 */
	if(global_xim) {
		Pixmap pix = fl_create_offscreen(image()->w(), image()->h());
		fl_begin_offscreen(pix);
			XPutImage(fl_display, pix, fl_gc, global_xim, 0, 0, 0, 0, image()->w(), image()->h());
		fl_end_offscreen();

		fl_copy_offscreen(ix, iy, image()->w(), image()->h(), pix, 0, 0);

		XChangeProperty(fl_display, RootWindow(fl_display, fl_screen), 
				_XA_XROOTPMAP_ID, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pix, 1);	

	}
#endif
}

int Wallpaper::handle(int event) {
	switch(event) {
		/* Route all DND events to parent (desktop), otherwise desktop will not get them if Wallpaper is visible */
		case FL_DND_ENTER:
		case FL_DND_DRAG:
		case FL_DND_LEAVE:
		case FL_DND_RELEASE:
		case FL_PASTE:
			return parent()->handle(event);
	}

	return 0;
}

void Wallpaper::resize(int X, int Y, int W, int H) {
	if(X == x() && Y == y() && W == w() && H == h())
		return;

	Fl_Box::resize(X, Y, W, H);
	if(image()) {
		/*
		 * It is safe to call 'load()' again, as Fl_Shared_Image::get() will cache successfully loaded image.
		 * Also benefit is that image transormations (scaling, tiling) will be done again on original image, so
		 * there will be no lost data.
		 *
		 * TODO: Fl_Shared_Image will eat a memory; this needs some investigation.
		 */
		load(wpath.c_str(), state, use_rootpmap);
	}
}
