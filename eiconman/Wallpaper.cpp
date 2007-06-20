/*
 * $Id$
 *
 * Eiconman, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Wallpaper.h"
#include "Utils.h"

#include <edelib/Debug.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/fl_draw.h>
#include <FL/x.h>

Pixmap create_xpixmap(Fl_Image* img, XImage* xim, Pixmap pix) {
	if(!img)
		return 0;

	if(xim) {
		if(xim->data)
			delete [] xim->data;
		XDestroyImage(xim);
	}

	if(pix)
		XFreePixmap(fl_display, pix);

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
	} else {
		EWARNING("Depth %i not supported, for now\n", fl_visual->depth);
		return 0;
	}

	// assume display == 16 depth
	xim = XCreateImage(fl_display, fl_visual->visual, 16, ZPixmap, 0, 0, img->w(), img->h(), 16, 0);

	if(xim->bits_per_pixel != 16) {
		EWARNING("Visual %i not implemented yet\n", xim->bits_per_pixel);
		XDestroyImage(xim);
		return 0;
	}

	if(img->d() < 3) {
		EWARNING("Only RGB(A) images is supported for now\n");
		XDestroyImage(xim);
		return 0;
	}

	int iw = img->w();
	int ih = img->h();
	int id = img->d();

	bool msb = false;
	if(ImageByteOrder(fl_display) == MSBFirst)
		msb = true;
	else
		msb = false;

	unsigned int r, g, b, tmp;
	unsigned char* dest = new unsigned char[iw * ih * id];
	unsigned char* destptr = dest;
	unsigned char* src = (unsigned char*)img->data()[0];

	for(int j = 0; j < ih; j++) {
		for(int i = 0; i < iw; i++) {
			r = *src++;
			g = *src++;
			b = *src++;

			if(id == 4)
				src++;

			tmp = 0;
			if(rshift >= 0)
				tmp |= (((int)r << rshift) & rmask);
			else
				tmp |= (((int)r >> (-rshift)) & rmask);

			if(gshift >= 0)
				tmp |= (((int)g << gshift) & gmask);
			else
				tmp |= (((int)g >> (-gshift)) & gmask);

			if(bshift >= 0)
				tmp |= (((int)b << bshift) & bmask);
			else
				tmp |= (((int)b >> (-bshift)) & bmask);

			if(msb) {
				// big endian
				*destptr++ = (tmp >> 8) & 0xff;
				*destptr++ = (tmp & 0xff);
			} else {
				// little endian
				*destptr++ = (tmp & 0xff);
				*destptr++ = (tmp >> 8) & 0xff;
			}
		}
	}

	xim->data = (char*)dest;

	/*
	 * Creating another window as drawable is needed since fl_window (as drawable) can't be
	 * used here (valid only in draw()).
	 */
	Window drawable = XCreateSimpleWindow(fl_display, RootWindow(fl_display, fl_screen), 0, 0, iw,
              ih, 0, 0, BlackPixel(fl_display, fl_screen));

	pix = XCreatePixmap(fl_display, drawable, iw, ih, fl_visual->depth);

	/*
	 * The same applies as above;
	 * fl_gc can't be used here.
	 */
	XGCValues gcv;
	gcv.graphics_exposures = False;
	GC dgc = XCreateGC(fl_display, pix, GCGraphicsExposures, &gcv);

	XPutImage(fl_display, pix, dgc, xim, 0, 0, 0, 0, iw, ih);

	XDestroyWindow(fl_display, drawable);
	XFreeGC(fl_display, dgc);

	return pix;
}

Wallpaper::Wallpaper(int X, int Y, int W, int H) : 
	Fl_Box(X, Y, W, H, 0), rootpmap_image(NULL), rootpmap_pixmap(0), img(NULL), tiled(false) { 
}

Wallpaper::~Wallpaper() { 
	if(rootpmap_image) {
		if(rootpmap_image->data)
			delete [] rootpmap_image->data;
		XDestroyImage(rootpmap_image);
	}

	if(rootpmap_pixmap)
		XFreePixmap(fl_display, rootpmap_pixmap);
}

bool Wallpaper::set(const char* path) {
	EASSERT(path != NULL);

	tiled = false;

	Fl_Image* i = Fl_Shared_Image::get(path);
	if(!i)
		return false;
	img = i;
	image(img);

	set_rootpmap();

	return true;
}

bool Wallpaper::set_tiled(const char* path) {
	bool ret = set(path);
	if(ret)
		tiled = true;
	return ret;
}

void Wallpaper::set_rootpmap(void) {
	if(!image())
		return;

	rootpmap_pixmap = create_xpixmap(image(), rootpmap_image, rootpmap_pixmap);

	if(!rootpmap_pixmap)
		return;

	XChangeProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_XROOTPMAP_ID, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&rootpmap_pixmap, 1);	

#if 0
	XGCValues gcv;
	gcv.graphics_exposures = False;
	GC dgc = XCreateGC(fl_display, pix, GCGraphicsExposures, &gcv);

	XImage img;
	img.byte_order = LSBFirst; // TODO: check
	img.format = ZPixmap;
	img.depth = fl_visual->depth; // depth of screen or depth of image() ?

	// find out bits_per_pixel field
	int num_pfv;
	XPixmapFormatValues* pfv = 0;
	XPixmapFormatValues* pfvlst = 0;
	pfvlst = XListPixmapFormats(fl_display, &num_pfv);
	for(pfv = pfvlst; pfv < pfvlst + num_pfv; pfv++) {
		if(pfv->depth == fl_visual->depth)
			break;
	}

	img.bits_per_pixel = pfv->bits_per_pixel;
	if(img.bits_per_pixel & 7) {
		EWARNING("Can't work with %i bpp !!!\n", img.bits_per_pixel);
		return;
	}
#endif
}

void Wallpaper::draw(void) {
	if(!image())
		return;

	int ix, iy, iw, ih;
	Fl_Image* im = image();

	iw = im->w();
	ih = im->h();

	if(iw == 0 || ih == 0)
		return;

	if(ih < h() || iw < w()) {
		if(tiled) { 
			// tile image across the box
			fl_push_clip(x(), y(), w(), h());
				int tx = x() - (x() % iw);
				int ty = y() - (y() % ih);
				int tw = w() + tx;
				int th = h() + ty;

				for(int j = 0; j < th; j += ih)
					for(int i = 0; i < tw; i += iw)
						im->draw(i, j);

			fl_pop_clip();
			return;
		} else {
			// center image in the box
			ix = (w()/2) - (iw/2);
			iy = (h()/2) - (ih/2);
			ix += x();
			iy += y();
		}
	} else {
		ix = x();
		iy = y();
	}

	im->draw(ix, iy);

	/*
	 * For debugging purposes :)
	 * Uncommenting this (and removing GC/Window creation in create_xpixmap
	 * will draw _XA_XROOTPMAP_ID Pixmap directly in Wallpaper widget. 
	 * It is used to check Fl_Image->Image conversion.
	 */
#if 0
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
