/*
 * $Id$
 *
 * Desktop configuration tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <stdio.h>
#include <FL/Fl_RGB_Image.H>
#include "BoxBlur.h"

typedef unsigned char uchar;
typedef unsigned int  uint;

static int pixel_address(int x, int y, int w) {
	return y * w + x;
}

static uint get_pixel(uint* data, int x, int y, int w) {
	int p = pixel_address(x, y, w);
	if(p < 0)
		p = 0;
	if(p > w - 1)
		p = w - 1;
	return data[p];
}

Fl_RGB_Image* box_blur(Fl_RGB_Image* src) {
	int radius = 4;

	uint* orig = (uint*)src->data()[0];
	uchar* buf = new uchar[src->w() * src->h() * src->d()];
	memcpy(buf, src->array, src->w() * src->h() * src->d());
	uint*  bufptr = (uint*)buf;

	for(int x = 4; x < src->w() - 1; x++) {
		for(int y = 4; y < src->h() - 1; y+= src->d()) {
			int count = 0;
			uint rsum = 0, gsum = 0, bsum = 0, asum = 0;

			for(int nx = x - radius; nx < x + radius; nx++) {
				for(int ny = y - radius; ny < y + radius; ny++) {
					/* get pixel from x,y */
					uint pix = get_pixel(bufptr, nx, ny, src->w()/src->d());
					uchar* ptr = (uchar*)&pix;

					rsum += *ptr++;
					gsum += *ptr++;
					bsum += *ptr++;
					asum += *ptr;
					
					count += 1;
				}
			}

			/* set pixel at x,y */
			uint tmp;
			uchar *tc = (uchar*)&tmp;
			*tc++ = rsum / count;
			*tc++ = gsum / count;
			*tc++ = bsum / count;
			*tc =   asum / count;

			bufptr[pixel_address(x, y, src->w()/src->d())] = tmp;
		}
	}

	if(src->alloc_array)
		delete [] (uchar*)src->array;
	src->array = buf;
	src->alloc_array = 1;

	return src;
}
